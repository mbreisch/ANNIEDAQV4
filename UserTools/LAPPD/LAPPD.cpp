#include "LAPPD.h"

LAPPD_args::LAPPD_args():DAQThread_args(){

  m_trigger_pub=0;
  m_data_receive=0;
  m_data_send=0;
  m_utils=0;
  m_logger=0;
  ref_time1=0;
  ref_time2=0;
  data_counter=0;
  LAPPDData=0;
  la="";
  lb="";
  servicename="";
  update_conns_period=0;
  portnum=0;
  polltimeout=0;
  storesendpolltimeout=0;
  statsperiod=0;
  ramdiskpath="";

}

LAPPD_args::~LAPPD_args(){

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

  delete ref_time1;
  delete ref_time2;
  ref_time1=0;
  ref_time2=0;
  
  for (std::map<std::string,Store*>::iterator it=connections.begin(); it!=connections.end(); ++it){
    delete it->second;
    it->second=0;
  }
  
  connections.clear();
  
  for(int i=0; i<data_buffer.size(); i++){
      delete data_buffer.at(i);
      data_buffer.at(i)=0;
    }
  
  data_buffer.clear();
  
  
  delete LAPPDData;
  
  LAPPDData=0;
  
   
}


LAPPD::LAPPD():Tool(){}


bool LAPPD::Initialise(std::string configfile, DataModel &data){

  m_data= &data;
  m_log= m_data->Log;

  if(m_tool_name=="") m_tool_name="LAPPD";
  
  // get tool config from database
  std::string configtext;
  bool get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
  if(!get_ok){
    Log(m_tool_name+" Failed to get Tool config from database!",0,0);
    //return false;
  }
  // parse the configuration to populate the m_variables Store.
  std::stringstream configstream(configtext);
  if(configtext!="") m_variables.Initialise(configstream);
  
  // allow overrides from local config file
  if(configfile!="")  m_variables.Initialise(configfile);

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
  
  //m_variables.Print();

  m_util=new DAQUtilities(m_data->context);

  //setting up main thread socket

  trigger_sub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  trigger_sub->bind("inproc://LAPPDtrigger_status");

  items[0].socket=*trigger_sub;
  items[0].fd=0;
  items[0].events=ZMQ_POLLIN;
  items[0].revents=0;


  if(!LAPPD_Thread_Setup(m_data, m_args, m_util)){
    Log("ERROR: LAPPD Tool error setting up LAPPD thread",0,m_verbose);
    return false;  // Setting up LAPPD thread (BEN error on retur)
  }
    
  if(!Store_Thread_Setup(m_data, m_args, m_util)){
    Log("ERROR: LAPPD Tool error setting up Store thread",0,m_verbose);
    return false;// Setting UP Store Thread (BEN error on return)
  }
  
  return true;
}


bool LAPPD::Execute(){
 
  zmq::poll(&items[0],1,0);
  
  if ((items[0].revents & ZMQ_POLLIN)){
    
    Store* tmp=0;
    
    if(!m_util->ReceivePointer(trigger_sub, tmp) || !tmp) Log("ERROR: receiving vme trigger status",0,m_verbose);
    
    std::string status="";
    *tmp>>status;

    Log(status,2,m_verbose);
    
    unsigned long LAPPD_readouts;
    tmp->Get("data_counter",LAPPD_readouts);
  
    m_data->triggers["LAPPD"]= LAPPD_readouts;
   
    unsigned long LAPPD_buffer;
    int get_ok = tmp->Get("data_buffer",LAPPD_buffer);
    if(get_ok) m_data->buffers["LAPPD"]= LAPPD_buffer;

    delete tmp;
    tmp=0;

  }
 

  return true;
}


bool LAPPD::Finalise(){
 
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
  
  
  
  return true;
}


void LAPPD::LAPPD_Thread(Thread_args* arg){
  
  LAPPD_args* args=reinterpret_cast<LAPPD_args*>(arg);
   
  if(args->connections.size()) args->m_utils->UpdateConnections(args->servicename, args->m_data_receive, args->connections, args->update_conns_period, args->ref_time1, std::to_string(args->portnum));
  else  args->m_utils->UpdateConnections(args->servicename, args->m_data_receive, args->connections, 5, args->ref_time1, std::to_string(args->portnum));

  //printf("lappd connections=%d\n",args->connections.size());
  //printf("lappd port=%d\n",args->portnum);
  //printf("servicename=%s\n",args->servicename.c_str());  
   
  for(std::map<std::string,Store*>::iterator it=args->connections.begin(); it!=args->connections.end(); it++){
  
    //printf("%s\n", it->first.c_str());
    std::string tmp="";
    it->second->Get("ip",tmp);
    //printf("ip=%s", tmp.c_str());
    //  it->second->Print();
  } 
  
  int pollok = zmq::poll(args->in, args->polltimeout);
  //std::cout<<"lappd poll returned "<<pollok<<" items"<<std::endl;
  if(pollok<0){
      printf("LAPPD poll for data error!\n");
  }
  
  else if(args->in.at(0).revents & ZMQ_POLLIN){

    //printf("in Get_Data\n");
    Get_Data(args); //new data comming in buffer it and send akn
  }
  
  zmq::poll(args->out, args->polltimeout);
  
  if((args->out.at(0).revents & ZMQ_POLLOUT) && args->data_buffer.size() ) LAPPD_To_Store(args);         /// data in buffer so send it to store writer 
  
  
  if(difftime(time(NULL),*args->ref_time2) > args->statsperiod){
    LAPPD_Stats_Send(args);   // on timer stats pub to main thread for triggering
    *args->ref_time2=time(NULL);
  }

}

void LAPPD::Store_Thread(Thread_args* arg){
  
  LAPPD_args* args=reinterpret_cast<LAPPD_args*>(arg);
  
  zmq::poll(args->in, args->polltimeout);
  
  if(args->in.at(1).revents & ZMQ_POLLIN) Store_Send_Data(args); /// received store data request
  
  if(args->in.at(0).revents & ZMQ_POLLIN) Store_Receive_Data(args); // received new data for adding to stores

}


bool LAPPD::LAPPD_Thread_Setup(DataModel* m_data, std::vector<LAPPD_args*> &m_args, DAQUtilities* m_util){


  LAPPD_args* tmp_args=new LAPPD_args();
  m_args.push_back(tmp_args);
 
  //Ben add timeout and linger settings;
  tmp_args->m_trigger_pub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_trigger_pub->connect("inproc://LAPPDtrigger_status");
  tmp_args->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_ROUTER);
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_data_send->bind("inproc://LAPPDtobuffer");
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;
  tmp_args->ref_time1=new time_t;
  tmp_args->ref_time2=new time_t;
  *tmp_args->ref_time1=time(NULL);
  *tmp_args->ref_time2=time(NULL);

  //  tmp_args->m_data_receive->connect("tcp://192.168.163.84:89765");
  
  tmp_args->polltimeout=10;
  m_variables.Get("polltimeout",tmp_args->polltimeout);
  tmp_args->m_data_receive->setsockopt(ZMQ_RCVTIMEO, tmp_args->polltimeout);
  tmp_args->m_data_receive->setsockopt(ZMQ_SNDTIMEO, tmp_args->polltimeout);

  zmq::pollitem_t tmp_item;
  tmp_item.socket=*tmp_args->m_data_receive;
  tmp_item.fd=0;
  tmp_item.events=ZMQ_POLLIN;
  tmp_item.revents=0;
  tmp_args->in.push_back(tmp_item);

  zmq::pollitem_t tmp_item2;
  tmp_item2.socket=*tmp_args->m_data_send;
  tmp_item2.fd=0;
  tmp_item2.events=ZMQ_POLLOUT;
  tmp_item2.revents=0;
  tmp_args->out.push_back(tmp_item2);

  zmq::pollitem_t tmp_item3;
  tmp_item3.socket=*tmp_args->m_data_receive;
  tmp_item3.fd=0;
  tmp_item3.events=ZMQ_POLLOUT;
  tmp_item3.revents=0;
  tmp_args->out.push_back(tmp_item3);

  tmp_args->servicename="LAPPD";
  tmp_args->update_conns_period = 300;
  tmp_args->portnum = 89765;
  m_variables.Get("servicename",tmp_args->servicename);
  m_variables.Get("updateconnsperiod", tmp_args->update_conns_period);
  m_variables.Get("portnum",tmp_args->portnum);
  
  tmp_args->statsperiod=60;
  m_variables.Get("statsperiod",tmp_args->statsperiod);

  return m_util->CreateThread("LAPPD", &LAPPD_Thread, tmp_args); ///Ben improve return with catches for socket setup failures

}

bool LAPPD::Store_Thread_Setup(DataModel* m_data, std::vector<LAPPD_args*> &m_args, DAQUtilities* m_util){

  LAPPD_args* tmp_args=new LAPPD_args();
  m_args.push_back(tmp_args);
  
  //Ben add timeout and linger settings;
  tmp_args->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_data_receive->connect("inproc://LAPPDtobuffer");
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_DEALER);
  tmp_args->m_data_send->setsockopt(ZMQ_IDENTITY, "LAPPD", 5);
  m_data->identities.push_back("LAPPD");
  tmp_args->m_data_send->connect("inproc://datatostoresave");
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;
  tmp_args->LAPPDData= new BoostStore(false, 2);
  
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

  tmp_args->storesendpolltimeout=10000;
  m_variables.Get("storesendpolltimeout",tmp_args->storesendpolltimeout);

  std::string ramdiskpath="/mnt/ramdisk";
  m_variables.Get("ramdiskpath",ramdiskpath);
  tmp_args->la=ramdiskpath+"/la";
  tmp_args->lb=ramdiskpath+"/lb";
  
  return m_util->CreateThread("Store", &Store_Thread, tmp_args); // Ben Setup error return correctly
  
}
 

bool LAPPD::Get_Data(LAPPD_args* args){  

  std::queue<zmq::message_t> message_queue;
  if(args->m_utils->ReceiveMessages(args->m_data_receive, message_queue)){


    //printf("got LAPPD data size=%d\n",  message_queue.size());

    if(message_queue.size()>=3){

      //printf("LAPPD in message queue check\n");
      
      zmq::message_t identity;
      identity.copy(&message_queue.front());
      message_queue.pop();
      zmq::message_t id;
      id.copy(&message_queue.front());
      message_queue.pop(); 
      
      bool error=false;
      
      	
      PsecData* data=new PsecData;
      if(!data->Receive(message_queue)){;   //havent added error checking and buffer flush
	printf("error Receiving PsecData\n");
	delete data;
	data=0;
	args->m_logger->Log("ERROR: Receiving PsecData");
	error=true;   
      }
      
      //printf("sending akn to LAPPD\n");
      //SEND AN AKN and MESSAGE NUMEBR ID;
      //BEN maybe add poll for outmessage to be sure otherwise will block
      int ok = zmq::poll(&args->out.at(1),1,args->polltimeout);
      
      if(ok < 0 || (args->out.at(1).revents & ZMQ_POLLOUT)==0){
	error=true;
	printf("error sending akn to LAPPD\n");
      }
      if(error || !args->m_data_receive->send(identity, ZMQ_SNDMORE) || !args->m_data_receive->send(id)){
	printf("error sending akn to LAPPD2\n");
	args->m_logger->Log("ERROR: Cant send data aknoledgement",0,0);  
	error=true;	
      }
      //printf("sent akn to LAPPD\n");
      
      if (!message_queue.size() && !error ){
	//printf("LAPPD data pushed back\n");
	args->data_buffer.push_back(data);                                               
	args->data_counter++;
	
      }
      
      if (message_queue.size() || error ){
	
	  delete data;
	  data=0;
	
	
	args->m_logger->Log("ERROR: With message format/contents");       
	return false;
      }
    
    }
	
    else{
      args->m_logger->Log("ERROR: not enough message parts");
      return false;
    }
    
    
  }
  else{
    args->m_logger->Log("ERROR:  error getting data from vme crate");      
    return false;
  }

  return true;
}


bool LAPPD::LAPPD_To_Store(LAPPD_args* args){ 
  
   
  if(args->data_buffer.size()){
    //printf("LAPPD data being sent to store thread\n");
    //send data pointer
    if(args->m_utils->SendPointer(args->m_data_send, args->data_buffer.at(0))){
      //printf("LAPPD data sent to store thread\n");      
      args->data_buffer.at(0)=0;       
      args->data_buffer.pop_front();
    }
    else args->m_logger->Log("ERROR:Failed to send data pointer to LAPPD Store Thread");
    
  }
 
  return true; //BEN do this better
 
}

bool LAPPD::LAPPD_Stats_Send(LAPPD_args* args){
  
  Store* tmp= new Store;
  tmp->Set("data_counter", args->data_counter);
  tmp->Set("data_buffer", args->data_buffer.size());
  tmp->Set("connections", args->connections.size());
  
  if(!args->m_utils->SendPointer(args->m_trigger_pub, tmp)){
    args->m_logger->Log("ERROR:Failed to send vme status data");  
    delete tmp;
    tmp=0;    
    return false;
  }  //    send store pointer possbile mem leak here as pub socket and not garenteed that main thread will receive, maybe use string //its PAR not pub sub
  
  return true;
}

bool LAPPD::Store_Send_Data(LAPPD_args* args){
   
   zmq::message_t message;
  
  if(args->m_data_send->recv(&message) && !message.more()){
    // send store pointer 
    zmq::poll(args->out, args->storesendpolltimeout);
    
    if(args->out.at(0).revents & ZMQ_POLLOUT){
      
      zmq::message_t key(6);                                                                       
      
      snprintf ((char *) key.data(), 10 , "%s" , "LAPPDData");
      args->m_data_send->send(key, ZMQ_SNDMORE);
    
      if (args->data_counter==0){
      	delete args->LAPPDData;
      	args->LAPPDData=0;
      }
      
      args->m_utils->SendPointer(args->m_data_send, args->LAPPDData);
      
      args->data_counter=0;
      args->LAPPDData=new BoostStore(false, 2);
      std::string ln=args->lb;
      args->lb=args->la;
      args->la=ln;   
            
      return true; 
    }
    
  }
  else args->m_logger->Log("ERROR:LAPPD Store Thread: bad store request");

  return false;

}

bool LAPPD::Store_Receive_Data(LAPPD_args* args){

  
  PsecData* tmp;
  if(args->m_utils->ReceivePointer(args->m_data_receive, tmp)){
    //printf("LAPPD data resecived by store thread\n");
    // for(int i=0; i<tmp->size(); i++){
      
      
      args->LAPPDData->Set("LAPPDData",*tmp);
      args->LAPPDData->Save(args->la);
      args->LAPPDData->Delete();
      args->data_counter++;
      //delete tmp->at(i);
      //tmp->at(i)=0;
      //}
      //printf("store thread saved in store\n");
      //tmp->clear();
    delete tmp;
    tmp=0;
    
    return true;
  }
  else{
    args->m_logger->Log("ERROR:LAPPD Store Thread: failed to receive data pointer");
    return false; 
  }
  
  return false;
}
