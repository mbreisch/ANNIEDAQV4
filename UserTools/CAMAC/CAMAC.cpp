#include "CAMAC.h"
#include "Algorithms.h"

CAMAC_args::CAMAC_args():Thread_args(){

  m_trigger_pub=0;
  m_data_receive=0;
  m_data_send=0;
  m_utils=0;
  m_logger=0;
  ref_time=0;
  data_counter=0;
  ma="";
  mb="";

  MRDdata=0;
  trg_pos=0;
  perc=0;
  DC="";
  MRDout=0;
  CCData=0;

  polltimeout=0;
  statsperiod=0;
  ramdiskpath="";
  storesendpolltimeout=10000;
}

CAMAC_args::~CAMAC_args(){

  delete m_trigger_pub;
  delete m_data_receive;
  delete m_data_send;

  m_trigger_pub=0;
  m_data_receive=0;
  m_data_send=0;

  in.clear();                            
  out.clear();

  m_utils=0;
  m_logger=0;

  delete ref_time;
  ref_time=0;

  MRDdata=0;
  trg_pos=0;
  perc=0;
 

  for(int i=0; i<data_buffer.size(); i++){
    delete data_buffer.at(i);
    data_buffer.at(i)=0;
  }
  data_buffer.clear();


  Data.ch.clear();
  delete MRDout;
  MRDout=0;
  delete CCData;
  CCData=0;

}


CAMAC::CAMAC():Tool(){}


bool CAMAC::Initialise(std::string configfile, DataModel &data){
  
  m_data= &data;
  m_log= m_data->Log;

  if(m_tool_name=="") m_tool_name="CAMAC";
  
  // get tool config from database
  std::string configtext;
  bool get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
  if(!get_ok){
    Log(m_tool_name+" Failed to get Tool config from database!",0,0);
    return false;
  }
  // parse the configuration to populate the m_variables Store.
  std::stringstream configstream(configtext);
  if(configtext!="") m_variables.Initialise(configstream);
  
  // allow overrides from local config file
  if(configfile!="")  m_variables.Initialise(configfile);

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
  
  //m_variables.Print();
  m_variables.Get("configcc",configcc);           // name of parent config file
  m_variables.Get("percent", perc);               //firing probability                                
  m_variables.Get("trg_mode", MRDdata.trg_mode);  //Trigger mode
  m_variables.Get("StartTime",StartTime);         // 


  // the 'configcc' option is a reference to another set of configuration options
  // that provide settings for configuring the CAMAC cards. 
  // It may be a DB entry name or a local file. The type should be given by
  // the 'configcctype' variable, which is either 'local' (file) or 'remote' (DB)
  configcctype="local";
  if(!m_variables.Get("configcctype",configcctype)){
  	// if the option is not present, we will default to local IF the file exists.
  	Log("CAMAC: no configcctype in m_variables, checking if "+configcc
  	   +" is a locally accessible file",3,m_verbose);
  	if((configcc=="") || (checkfileexists(configcc)==false)) configcctype="remote";
  }
  Log("CAMAC: configcc '"+configcc+"' is a "+configcctype+" file",3,m_verbose);
  
  m_util=new DAQUtilities(m_data->context);

  if(!SetupCards()){
    Log("ERROR: CAMAC Tool error connecting and settitg up cards",0,m_verbose);
    return false;
  }

  srand(time(0)); 
  MRDdata.triggernum=0;
 
  //setting up main thread socket

  trigger_sub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  trigger_sub->bind("inproc://CAMACtrigger_status");

  items[0].socket=*trigger_sub;
  items[0].fd=0;
  items[0].events=ZMQ_POLLIN;
  items[0].revents=0;

  
  if(!CAMAC_Thread_Setup(m_data, m_args, m_util)){
    Log("ERROR: CAMAC Tool error setting up CAMAC thread",0,m_verbose);
    return false;  // Setting up CAMAC thread (BEN error on retur)
  }
  
    
  if(!Store_Thread_Setup(m_data, m_args, m_util)){
    Log("ERROR: CAMAC Tool error setting up Store thread",0,m_verbose);
    return false;// Setting UP Store Thread (BEN error on return)
  }
  

  return true;
}


bool CAMAC::Execute(){

  zmq::poll(&items[0],1,0);
  
  if ((items[0].revents & ZMQ_POLLIN)){
    
    Store* tmp=0;
    
    if(!m_util->ReceivePointer(trigger_sub, tmp) || !tmp) Log("ERROR: receiving CAMAC trigger status",0,m_verbose);
    
    std::string status="";
    *tmp>>status;
    
    Log(status,1,m_verbose);
    
    unsigned long CAMAC_readouts;
    tmp->Get("data_counter",CAMAC_readouts);
    
    m_data->triggers["CAMAC"]= CAMAC_readouts;
    
    unsigned long CAMAC_buffer;
    int get_ok = tmp->Get("data_buffer",CAMAC_buffer);
    if(get_ok) m_data->buffers["CAMAC"]= CAMAC_buffer;

    delete tmp;
    tmp=0;

  }


  return true;
}


bool CAMAC::Finalise(){

  for(int i=0; i<m_args.size(); i++){
    m_util->KillThread(m_args.at(i));
      delete m_args.at(i);
    m_args.at(i)=0;
  }

  m_args.clear();

  delete trigger_sub;
  trigger_sub=0;

  delete m_util;
  m_util=0;

  Lcard.clear();
  Ncard.clear();

  // BEN new maybe not do if want to restart subrun
  Ccard.clear();
  Ncrate.clear();

    //clear disck maybe also

  return true;
}

void CAMAC::CAMACThread(Thread_args* arg){

  CAMAC_args* args=reinterpret_cast<CAMAC_args*>(arg);

    
  Get_Data(args); //new data comming in buffer it and send akn

  zmq::poll(args->out, args->polltimeout);
  
  if( (args->out.at(0).revents & ZMQ_POLLOUT) && args->data_buffer.size()  ) CAMAC_To_Store(args);         /// data in buffer so send it to store writer 
  
  if(difftime(time(NULL),*args->ref_time) >args->statsperiod){
    CAMAC_Stats_Send(args);   // on timer stats pub to main thread for triggering
    *args->ref_time=time(NULL);
  }

  ///////////////////////
}


bool CAMAC::Get_Data(CAMAC_args* args){
  
  args->MRDdata->TRG = false;
  switch (args->MRDdata->trg_mode){ //0 is real trg, 1 is soft trg, 2 is with test
    
  case 0:
    if(args->MRDdata->List.CC["TDC"].at(*args->trg_pos)->TestEvent() == 1)
      args->MRDdata->TRG = true;
    break;
    
  case 1:
    args->MRDdata->TRG = true;
    break;
    
  case 2:
    args->MRDdata->TRG = rand() % int(100/(*args->perc)) == 0;
    break;
    
  default:
    args->m_logger->Log("WARNING: Trigger mode unknown");
  }

    
  ////////////////////////////////////////////
  ///////////////////////////////////////////////
  ////////////////////////////////////////////////

  if(args->MRDdata->TRG){
    
    args->MRDdata->triggernum++;
    args->data_counter++;
    
    //	  std::cout << "TRG on!\n" << std::endl;
    args->MRDdata->LocalTime = boost::posix_time::microsec_clock::local_time();
    //		++count;
    //		Log("TRG!\n", 2, verb);
    //std::cout<<"L3"<<std::endl;
    
    for (int i = 0; i < args->MRDdata->List.CC[args->DC].size(); i++)
      {
	//std::cout<<"L4"<<std::endl;
	//std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;

	args->Data.ch.clear();
	//std::cout<<"L5"<<std::endl;
	//std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;
	
	if (args->MRDdata->trg_mode >0){
	  //std::cout<<"L5.4 i="<<i<<" size="<<args->MRDdata->List.CC[args->DC].size()<<std::endl;
	  //std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;
	  args->MRDdata->List.CC[args->DC].at(i)->InitTest();
	  
	}
	//std::cout<<"L5.5"<<std::endl;
	args->MRDdata->List.CC[args->DC].at(i)->GetData(args->Data.ch);
	//std::cout<<"L6"<<std::endl;
	//std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;
	
	if (args->Data.ch.size() != 0)
	  {
	    //std::cout<<"L7"<<std::endl;
	    //std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;
	    
	    args->MRDdata->List.Data[args->DC].Slot.push_back(args->MRDdata->List.CC[args->DC].at(i)->GetSlot());
	    //	std::cout<<"L8 slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<std::endl;
	    //	std::cout<<"i="<<i<<" , size="<<args->MRDdata->List.CC[args->DC].size()<<std::endl;
	    //	std::cout<<"crate is="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;
	    args->MRDdata->List.Data[args->DC].Crate.push_back(args->MRDdata->List.CC[args->DC].at(i)->GetCrate());
	    args->MRDdata->List.Data[args->DC].Num.push_back(args->Data);
	    args->MRDdata->List.CC[args->DC].at(i)->ClearAll();
	    //std::cout<<"L8"<<std::endl;
	    //std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;
	    
	  }
	//std::cout<<"L9"<<std::endl;
	//std::cout<<"slot="<<args->MRDdata->List.CC[args->DC].at(i)->GetSlot()<<" : crate="<<args->MRDdata->List.CC[args->DC].at(i)->GetCrate()<<std::endl;

      }
    //std::cout<<"L10"<<std::endl;
   
    //std::cout<<"d20"<<std::endl;   
    ///////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //      std::cout<<"positive trigger"<<std::endl;
    //	  std::cout<<"E2"<<std::endl;
    boost::posix_time::time_duration Time = args->MRDdata->LocalTime - (*(args->Epoch));
    //    boost::posix_time::time_duration T1=args->MRDdata->LocalTime - args->MRDdata->LocalTime - args->MRDdata->LocalTime;
    // boost::posix_time::time_duration T2=(*(args->Epoch)) - (*(args->Epoch)) - (*(args->Epoch));
    //std::cout<<"d21 "<<Time<<" : "<<Time.total_milliseconds()<<std::endl;    
    //std::cout<<"local time="<<  boost::posix_time::to_iso_string(args->MRDdata->LocalTime)<<std::endl;
    //std::cout<<"epoch="<<  boost::posix_time::to_iso_string(*(args->Epoch))<<std::endl;  
    //std::cout<<"diff="<< (args->MRDdata->LocalTime - (*(args->Epoch))).total_milliseconds()<<std::endl;
    //std::cout<<"test="<< args->MRDout->TimeStamp<<std::endl;
  args->MRDout->TimeStamp = Time.total_milliseconds();
  //std::cout<<"d22"<<std::endl;
    //		std::cout << "TimeStamp " << TimeStamp << std::endl; 	
    //		TOutN = 0, AOutN = 0;
    //		std::fill(TDC, TDC+512, 0);
    //		std::fill(ADC, ADC+512, 0);
    //	std::cout<<"E3"<<std::endl;
    args->MRDout->Type.clear();
    //std::cout<<"d23"<<std::endl;
    args->MRDout->Value.clear();
    //std::cout<<"d24"<<std::endl;
    args->MRDout->Slot.clear();
    //std::cout<<"d25"<<std::endl;
    args->MRDout->Crate.clear();
    //std::cout<<"d26"<<std::endl;
    args->MRDout->Channel.clear();
    //std::cout<<"d27"<<std::endl;
    args->MRDout->OutN = 0;
    //std::cout<<"d28"<<std::endl;
    //std::cout<<"E4"<<std::endl;
    args->MRDout->Trigger= args->MRDdata->triggernum;
    //std::cout<<"d29"<<std::endl;
    // std::cout<<"E5 size="<<args->MRDdata.List.Data.size()<<std::endl;
    
    if (args->MRDdata->List.Data.size() > 0)	//There is something to be saved
      {
	//std::cout<<"d30"<<std::endl;
	//  std::cout<<"E6 "<<std::endl;
	args->inn = args->MRDdata->List.Data.begin();		//iterates over Module.Data map<type, Cards>
        
	//loop on Module.Data types, either TDC or ADC
	for (; args->inn != args->MRDdata->List.Data.end(); ++(args->inn))
	  {
	    //std::cout<<"d31"<<std::endl;
	    //is = args->MRDdata.List.Data["TDC"].Num.begin();
	    args->is = args->inn->second.Num.begin();
	    
	    //loop on active Module.Card.Num vector
	    for (int i = 0; args->is != args->inn->second.Num.end(); ++(args->is), ++i)
	      {
		//std::cout<<"d32"<<std::endl;
		args->MRDout->OutN += args->is->ch.size();	//number of channels on
		args->it = args->is->ch.begin();
		
		//loop on active channels
		for (; args->it != args->is->ch.end(); ++(args->it))
		  {
		    //std::cout<<"d33"<<std::endl;
		    args->MRDout->Type.push_back(args->inn->first);
		    args->MRDout->Value.push_back(args->it->second);
		    args->MRDout->Channel.push_back((args->it->first % 100));
		    args->MRDout->Slot.push_back(args->inn->second.Slot.at(i));
		    args->MRDout->Crate.push_back(args->inn->second.Crate.at(i));
		  }
	      }
	  }
      }
    
    //std::cout<<"d34"<<std::endl;
    args->data_buffer.push_back(args->MRDout);
    args->MRDout=new MRDOut;
    //std::cout<<"d34"<<std::endl;
  }
  //std::cout<<"d36"<<std::endl;   
  return true;   //ben this needs to be made useful
}

inline CamacCrate* CAMAC::Create(std::string cardname, std::istream* config, int cardslot, int crate){
	CamacCrate* ccp;
	if (cardname == "TDC")
	{
	  std::cout<<"TDC"<<std::endl;
	  ccp = new Lecroy3377(cardslot, config, crate);
	}
	if (cardname == "ADC")
	{
	  std::cout<<"ADC"<<std::endl;
	  ccp = new Lecroy4300b(cardslot, config, crate);
	}
	return ccp;	
}

inline CamacCrate* CAMAC::Create(std::string cardname, std::string config, int cardslot, int crate)
{
	CamacCrate* ccp;
	if (cardname == "TDC")
	{
	  std::cout<<"TDC"<<std::endl;
	  ccp = new Lecroy3377(cardslot, config, crate);
	}
	if (cardname == "ADC")
	{
	  std::cout<<"ADC"<<std::endl;
	  ccp = new Lecroy4300b(cardslot, config, crate);
	}
	return ccp;
}


bool CAMAC::CAMAC_To_Store(CAMAC_args* args){ 
 
  if(args->data_buffer.size()){
    //send data pointer

    if(args->m_utils->SendPointer(args->m_data_send, args->data_buffer.at(0))){
      args->data_buffer.at(0)=0;      
      args->data_buffer.pop_front();
    }
    else args->m_logger->Log("ERROR:Failed to send data pointer to CAMAC Store Thread");
    
  }

  return true; //BEN do this better
 
}

bool CAMAC::CAMAC_Stats_Send(CAMAC_args* args){

  Store* tmp= new Store;
  tmp->Set("data_counter",args->data_counter);
  tmp->Set("data_buffer",args->data_buffer.size());
  
  if(!args->m_utils->SendPointer(args->m_trigger_pub, tmp)){
    args->m_logger->Log("ERROR:Failed to send camac status data");  
    delete tmp;
    tmp=0;    
    return false;
  }  //    send store pointer possbile mem leak here as pub socket and not garenteed that main thread will receive, maybe use string //its PAR not pub sub
  
  return true;
}


bool CAMAC::SetupCards(){

  // loading card types and locations from config file
  std::istream* configstream=nullptr;

  // local file if present will take precedence
  if(configcctype=="local"){
  	configstream = new std::ifstream(configcc.c_str());
  	if(!dynamic_cast<ifstream*>(configstream)->is_open()){
  		Log("CAMAC::SetupCards failed to open local configuration file "+configcc,0,m_verbose);
  		delete configstream;
  		configstream=nullptr;
  		return false;
  	}
  }
  
  // if we have a database entry name, get the contents from that
  if(configstream==nullptr){
  	std::string parentconfigcontents;
  	bool get_ok = m_data->postgres_helper.GetToolConfig(configcc, parentconfigcontents);
  	if(!get_ok){
  		Log("ERROR: CAMAC Tool failed to get parent config entry "+configcc,0,m_verbose);
  	} else {
  		configstream = new std::stringstream(parentconfigcontents);
  	}
  }
  
  if(configstream==nullptr) return false;

  // the first entry in the parent file should be its type.
  // this will specify whether the following per-slot config files
  // are DB entry names or paths to local files.
  // if not present, we will assume local files.
  std::string entrytype="";
  
  std::string Line;
  std::stringstream ssL;
  std::string sEmp;
  int iEmp;
  
  int c = 0;
  while (getline(*configstream, Line))
  {
      if (Line[0] == '#') continue;
      else
	{
	  ssL.str("");
	  ssL.clear();
	  ssL << Line;
	  if (ssL.str() == "") continue;
      
	  if(entrytype==""){
	  	ssL >> sEmp >> entrytype;
	  	std::cout<<"scanning entrytype from line "<<Line<<"; sEmp="<<sEmp<<"; entrytype="<<entrytype<<std::endl;
	  	if(sEmp ==  "entrytype" && (entrytype=="local" || entrytype=="remote")) continue;
	  	Log("WARNING: CAMAC Tool parent configcc did not specify file type! "
	  	    "Assuming local!",0,m_verbose);
	  	entrytype="local";   // default: assume local for backwards compatibility
	  	// and assuming this was a normal line we need to re-parse it
	    ssL.str("");
	  	ssL.clear();
	  	ssL << Line;
	  }
	  
	  ssL >> sEmp;
	  Lcard.push_back(sEmp);		//Modeli L
	  ssL >> iEmp;
	  Ncard.push_back(iEmp);		//Slot N
	  ssL >> iEmp;
	  Ncrate.push_back(iEmp);         // crate
	  ssL >> sEmp;
	  Ccard.push_back(sEmp);		//Slot register file
	}
  }
  // close file if it's a file
  std::ifstream* filepointer = dynamic_cast<std::ifstream*>(configstream);
  if(filepointer) filepointer->close();
  delete configstream;
  configstream=0;

  // a stringstream to hold the contents of the slot configs, if reading from DB
  std::stringstream sslotconfig;

  // creating card classes based on config file
  
  trg_pos = 0;								//must implemented for more CC
  for (int i = 0; i < Lcard.size(); i++)	//CHECK i
    {
      std::cout << "for begin " <<Lcard.at(i)<< std::endl;

      // if our configurations for each slot are DB entries we need to get the data from the DB
      if(entrytype=="remote"){
      	std::string contents;
      	bool get_ok = m_data->postgres_helper.GetToolConfig(Ccard.at(i), contents);
      	if(!get_ok){
      		Log("ERROR! CAMAC failed to get DB config entry '"+Ccard.at(i)+"'",0,m_verbose);
      		return false;
      	}
      	sslotconfig.clear();
      	sslotconfig.str(contents);
      }
    
      if (Lcard.at(i) == "TDC" || Lcard.at(i) == "ADC")
	{
	  //std::cout << "d1 " << Ccard.at(i) << " " << Ncard.at(i) <<" "<< Ncrate.at(i)<<std::endl; 
	  if(entrytype=="local")  MRDdata.List.CC[Lcard.at(i)].push_back(Create(Lcard.at(i), Ccard.at(i), Ncard.at(i), Ncrate.at(i)));	//They use CC at 0
	  if(entrytype=="remote") MRDdata.List.CC[Lcard.at(i)].push_back(Create(Lcard.at(i), &sslotconfig, Ncard.at(i), Ncrate.at(i)));	//They use CC at 0
	  //std::cout << "d2 "<<std::endl;
	}
		else if (Lcard.at(i) == "TRG")
		  {
		    //std::cout << "d3 "<<std::endl;
		    trg_pos = MRDdata.List.CC["TDC"].size();
		    //std::cout << "d4 "<<std::endl;			
		    if(entrytype=="local")  MRDdata.List.CC["TDC"].push_back(Create("TDC", Ccard.at(i), Ncard.at(i), Ncrate.at(i)));	//They use CC at 0
		    if(entrytype=="remote") MRDdata.List.CC["TDC"].push_back(Create("TDC", &sslotconfig, Ncard.at(i), Ncrate.at(i)));	//They use CC at 0
		    //std::cout << "d5 "<<std::endl;
		  }
		else if (Lcard.at(i) == "DISC"){
		  //		  std::string cardname, std::string config, int cardslot, int crate
		  //std::cout<<"setting descriminator crate:"<<Ncrate.at(i)<<", Card:"<<Ncard.at(i)<<std::endl;
		  LeCroy4413* tmp=nullptr;
		  if(entrytype=="local")  tmp=new LeCroy4413(Ncard.at(i), Ccard.at(i), Ncrate.at(i));
		  if(entrytype=="remote") tmp=new LeCroy4413(Ncard.at(i), &sslotconfig, Ncrate.at(i));
		  disc.push_back(tmp);
		  //		  tmp.
		  
		}
		else std::cout << "\n\nUnkown card\n" << std::endl;
      //std::cout << "for over " << std::endl;
    }
  
  //std::cout << "Trigger is in slot ";
  //std::cout << MRDdata.List.CC["TDC"].at(trg_pos)->GetSlot();
  //std::cout << " and crate "<<MRDdata.List.CC["TDC"].at(trg_pos)->GetCrate() << std::endl;
 


  //from Lecroy tool
	Log("Clearing modules and printing the registers\n", 1, 1);

	for (int i = 0; i < MRDdata.List.CC["TDC"].size(); i++)
	{
		MRDdata.List.CC["TDC"].at(i)->ClearAll();
		MRDdata.List.CC["TDC"].at(i)->GetRegister();
		MRDdata.List.CC["TDC"].at(i)->PrintRegRaw();
	}


	return true;////////BEN Error return fix please

}


void CAMAC::StoreThread(Thread_args* arg){

  CAMAC_args* args=reinterpret_cast<CAMAC_args*>(arg);
  
  zmq::poll(args->in, args->polltimeout);
  
  if(args->in.at(1).revents & ZMQ_POLLIN) Store_Send_Data(args); /// received store data request
  
  if(args->in.at(0).revents & ZMQ_POLLIN) Store_Receive_Data(args); // received new data for adding to stores
  
}

bool CAMAC::Store_Send_Data(CAMAC_args* args){
   
   zmq::message_t message;
  
   if(args->m_data_send->recv(&message) && !message.more()){
     // send store pointer
     zmq::poll(args->out, args->storesendpolltimeout);
     
     if(args->out.at(0).revents & ZMQ_POLLOUT){
      zmq::message_t key(6);                                                                       
      
      snprintf ((char *) key.data(), 6 , "%s" , "CAMAC");
      args->m_data_send->send(key, ZMQ_SNDMORE);
    
      if (args->data_counter==0){
      	delete args->CCData;
      	args->CCData=0;
       } 
      
      args->m_utils->SendPointer(args->m_data_send, args->CCData);
    
      args->data_counter=0;
      args->CCData=new BoostStore(false, 2);
      std::string mn=args->mb;
      args->mb=args->ma;
      args->ma=mn;   
      
      return true; 
    }
    
   }
   else args->m_logger->Log("ERROR:CAMAC Store Thread: bad store request");
   
   return false;
   
}

bool CAMAC::Store_Receive_Data(CAMAC_args* args){
  
  
  MRDOut* tmp;
  if(args->m_utils->ReceivePointer(args->m_data_receive, tmp)){
   
    args->CCData->Set("Data",*tmp);
    args->CCData->Save(args->ma);
    args->CCData->Delete();

    args->data_counter++;
    
    delete tmp;
    tmp=0;

    return true;    
  }
  else{
    args->m_logger->Log("ERROR:CAMAC Store Thread: failed to receive data pointer");
    return false; 
  }
  
  return false;
}




bool CAMAC::CAMAC_Thread_Setup(DataModel* m_data, std::vector<CAMAC_args*> &m_args, DAQUtilities* m_util){

  CAMAC_args* tmp_args = new CAMAC_args();
  m_args.push_back(tmp_args);  
  
  tmp_args->MRDdata=&MRDdata;
  tmp_args->MRDout = new MRDOut;

  tmp_args->trg_pos=&trg_pos;
  tmp_args->perc=&perc;
  tmp_args->DC="TDC";
  m_variables.Get("card_type",tmp_args->DC);

  tmp_args->m_trigger_pub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_trigger_pub->connect("inproc://CAMACtrigger_status");
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_PAIR); 
  tmp_args->m_data_send->bind("inproc://CAMACtobuffer");   
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;
  tmp_args->ref_time=new time_t;
  *tmp_args->ref_time=time(NULL);
  tmp_args->Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));         

  zmq::pollitem_t tmp_item;
  tmp_item.socket=*tmp_args->m_data_send; 
  tmp_item.fd=0;
  tmp_item.events=ZMQ_POLLOUT;
  tmp_item.revents=0;
  tmp_args->out.push_back(tmp_item);
  
  tmp_args->polltimeout=10;
  m_variables.Get("polltimeout",tmp_args->polltimeout);

  tmp_args->statsperiod=60;
  m_variables.Get("statsperiod",tmp_args->statsperiod);

  return m_util->CreateThread("camac", &CAMACThread, tmp_args);

}

bool CAMAC::Store_Thread_Setup(DataModel* m_data, std::vector<CAMAC_args*> &m_args, DAQUtilities* m_util){
 
  CAMAC_args* tmp_args = new CAMAC_args();
  m_args.push_back(tmp_args); 

  tmp_args->CCData= new BoostStore(false, 2);
  

  //Ben add timeout and linger settings;
  tmp_args->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_data_receive->connect("inproc://CAMACtobuffer");
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_DEALER);
  tmp_args->m_data_send->setsockopt(ZMQ_IDENTITY, "CAMAC", 5);
  m_data->identities.push_back("CAMAC");
  tmp_args->m_data_send->connect("inproc://datatostoresave");
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;

  zmq::pollitem_t tmp_item3;
  tmp_item3.socket=*tmp_args->m_data_receive;
  tmp_item3.fd=0;
  tmp_item3.events=ZMQ_POLLIN;
  tmp_item3.revents=0;
  tmp_args->in.push_back(tmp_item3);
  
  zmq::pollitem_t tmp_item4;
  tmp_item4.socket=*tmp_args->m_data_send;
  tmp_item4.fd=0;
  tmp_item4.events=ZMQ_POLLIN;
  tmp_item4.revents=0;
  tmp_args->in.push_back(tmp_item4);
  
  zmq::pollitem_t tmp_item5;
  tmp_item5.socket=*tmp_args->m_data_send;
  tmp_item5.fd=0;
  tmp_item5.events=ZMQ_POLLOUT;
  tmp_item5.revents=0;
  tmp_args->out.push_back(tmp_item5);

  tmp_args->polltimeout=10;
  m_variables.Get("polltimeout",tmp_args->polltimeout);

  tmp_args->storesendpolltimeout = 10000;
  m_variables.Get("storesendpolltimeout",tmp_args->storesendpolltimeout);

  std::string ramdiskpath="/mnt/ramdisk";
  m_variables.Get("ramdiskpath", ramdiskpath);
  tmp_args->ma=ramdiskpath+"/ma";
  tmp_args->mb=ramdiskpath+"/mb";
  
  return  m_util->CreateThread("store", &StoreThread, tmp_args);    
   
}
