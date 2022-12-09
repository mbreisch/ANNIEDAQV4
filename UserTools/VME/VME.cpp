#include "VME.h"

VME_args::VME_args():DAQThread_args(){

  m_trigger_pub=0;
  m_data_receive=0;
  m_data_send=0;
  m_utils=0;
  m_logger=0;
  ref_time1=0;
  ref_time2=0;
  data_counter=0;
  trigger_counter=0;  
  PMTData=0;
  TrigData=0;
  da="";
  db="";
  ta="";
  tb="";
  servicename="";
  portnum=0;
  update_conns_period=0;
  statsperiod=0;
  polltimeout=0;
  storesendpolltimeout=0;
  ramdiskpath="";

}

VME_args::~VME_args(){

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
  /*  
  for(int i=0; i<data_buffer.size(); i++){
    for (int j=0; j<data_buffer.at(i)->size(); j++){
      delete data_buffer.at(i)->at(j);
      data_buffer.at(i)->at(j)=0;
    }
  }
  */
  data_buffer.clear();
  

  for( int i=0; i<trigger_buffer.size(); i++){
    for (int j=0; j<trigger_buffer.at(i)->size(); j++){
      delete trigger_buffer.at(i)->at(j);
      trigger_buffer.at(i)->at(j)=0;
    }
  }

  trigger_buffer.clear();
  
  delete PMTData;
  delete TrigData;
  
  PMTData=0;
  TrigData=0;
  
}


VME::VME():Tool(){}


bool VME::Initialise(std::string configfile, DataModel &data){

  m_data= &data;
  m_log= m_data->Log;

  if(m_tool_name=="") m_tool_name="VME";
  
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
  trigger_sub->bind("inproc://VMEtrigger_status");

  items[0].socket=*trigger_sub;
  items[0].fd=0;
  items[0].events=ZMQ_POLLIN;
  items[0].revents=0;


  if(!VME_Thread_Setup(m_data, m_args, m_util)){
    Log("ERROR: VME Tool error setting up VME thread",0,m_verbose);
    return false;  // Setting up VME thread (BEN error on retur)
  }
  
  if(!Store_Thread_Setup(m_data, m_args, m_util)){
    Log("ERROR: VME Tool error setting up Store thread",0,m_verbose);
    return false;// Setting UP Store Thread (BEN error on return)
  }
  
  return true;
}


bool VME::Execute(){
 
  zmq::poll(&items[0],1,0);
  
  if ((items[0].revents & ZMQ_POLLIN)){
    
    Store* tmp=0;
    
    if(!m_util->ReceivePointer(trigger_sub, tmp) || !tmp) Log("ERROR: receiving vme trigger status",0,m_verbose);
    
    std::string status="";
    *tmp>>status;

    Log(status,2,m_verbose);
    
    unsigned long VME_readouts;
    tmp->Get("data_counter",VME_readouts);
    unsigned long TRIG_readouts;
    tmp->Get("trigger_counter",TRIG_readouts);

    m_data->triggers["VME"]= VME_readouts;
    m_data->triggers["Trig"]= TRIG_readouts;

    unsigned long VME_buffer;
    tmp->Get("data_buffer",VME_buffer);
    unsigned long TRIG_buffer;
    tmp->Get("trigger_buffer",TRIG_buffer);

    m_data->buffers["VME"]= VME_buffer;
    m_data->buffers["Trig"]= TRIG_buffer;
   
    delete tmp;
    tmp=0;

  }
 

  return true;
}


bool VME::Finalise(){

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


void VME::VME_Thread(Thread_args* arg){
  
  VME_args* args=reinterpret_cast<VME_args*>(arg);
   
  if(args->connections.size())  args->m_utils->UpdateConnections(args->servicename, args->m_data_receive, args->connections, args->update_conns_period, args->ref_time1, std::to_string(args->portnum));
  else  args->m_utils->UpdateConnections(args->servicename, args->m_data_receive, args->connections, 5 , args->ref_time1, std::to_string(args->portnum));

  zmq::poll(args->in, args->polltimeout);
  
  if(args->in.at(0).revents & ZMQ_POLLIN) Get_Data(args); //new data comming in buffer it and send akn
  //printf("x1\n");
  zmq::poll(args->out, args->polltimeout);
  //printf("x2\n");
  if((args->out.at(0).revents & ZMQ_POLLOUT) && (args->data_buffer.size() || args->trigger_buffer.size()) ){
    //printf("x3\n");
 VME_To_Store(args);         /// data in buffer so send it to store writer 
 //printf("x4\n");  
}  
  //printf("x5\n");
  if(difftime(time(NULL), *args->ref_time2) >= args->statsperiod) {
    //printf("x6\n");
    VME_Stats_Send(args);   // on timer stats pub to main thread for triggering
    //printf("x7\n");  
  *args->ref_time2=time(NULL);
  //printf("x8\n");
  }
  //printf("x9\n");
}

void VME::Store_Thread(Thread_args* arg){
  //printf("f1\n");
  VME_args* args=reinterpret_cast<VME_args*>(arg);
  //printf("f2\n");
  zmq::poll(args->in, args->polltimeout);
  //printf("f3\n");
  if(args->in.at(1).revents & ZMQ_POLLIN){
    //printf("f4\n");
 Store_Send_Data(args); /// received store data request
 //printf("f5\n");  
}  
  //printf("f6\n");
  if(args->in.at(0).revents & ZMQ_POLLIN){
    //printf("f7\n");
 Store_Receive_Data(args); // received new data for adding to stores
 //printf("f8\n");
  }
  //printf("f9\n");
}


bool VME::VME_Thread_Setup(DataModel* m_data, std::vector<VME_args*> &m_args, DAQUtilities* m_util){


  VME_args* tmp_args=new VME_args();
  m_args.push_back(tmp_args);
 
  //Ben add timeout and linger settings;
  tmp_args->m_trigger_pub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_trigger_pub->connect("inproc://VMEtrigger_status");
  tmp_args->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_ROUTER);
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_data_send->bind("inproc://VMEtobuffer");
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;
  tmp_args->ref_time1=new time_t;
  tmp_args->ref_time2=new time_t;
  *tmp_args->ref_time1=time(NULL);
  *tmp_args->ref_time2=time(NULL);
  
  tmp_args->servicename="VME";
  tmp_args->portnum=98989;
  tmp_args->update_conns_period=300;
  m_variables.Get("servicename",tmp_args->servicename);
  m_variables.Get("portnum",tmp_args->portnum);
  m_variables.Get("updateconnsperiod",tmp_args->update_conns_period);
  
  tmp_args->statsperiod=60;
  m_variables.Get("statsperiod",tmp_args->statsperiod);


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

  tmp_args->polltimeout=10;
  m_variables.Get("polltimeout",tmp_args->polltimeout);
  tmp_args->m_data_receive->setsockopt(ZMQ_RCVTIMEO, tmp_args->polltimeout);  // FIXME make unique
  tmp_args->m_data_receive->setsockopt(ZMQ_SNDTIMEO, tmp_args->polltimeout);  // poll times

  return m_util->CreateThread("VME", &VME_Thread, tmp_args); ///Ben improve return with catches for socket setup failures

}

bool VME::Store_Thread_Setup(DataModel* m_data, std::vector<VME_args*> &m_args, DAQUtilities* m_util){

  VME_args* tmp_args=new VME_args();
  m_args.push_back(tmp_args);
  
  //Ben add timeout and linger settings;
  tmp_args->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_data_receive->connect("inproc://VMEtobuffer");
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_DEALER);
  tmp_args->m_data_send->setsockopt(ZMQ_IDENTITY, "VME", 3);
  m_data->identities.push_back("VME");
  tmp_args->m_data_send->connect("inproc://datatostoresave");
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;
  tmp_args->PMTData= new BoostStore(false, 2);
  tmp_args->TrigData=new BoostStore(false, 2);

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

  tmp_args->polltimeout = 10;
  m_variables.Get("polltimeout",tmp_args->polltimeout);
  
  // we also have a longer poll timeout used for sending store data
  tmp_args->storesendpolltimeout = 10000;
  m_variables.Get("storesendpolltimeout",tmp_args->storesendpolltimeout);

  std::string ramdiskpath="/mnt/ramdisk";
  m_variables.Get("ramdiskpath",ramdiskpath);
  tmp_args->da=ramdiskpath+"/da";
  tmp_args->db=ramdiskpath+"/db";
  tmp_args->ta=ramdiskpath+"/ta";
  tmp_args->tb=ramdiskpath+"/tb";
  
  return m_util->CreateThread("Store", &Store_Thread, tmp_args); // Ben Setup error return correctly
  
}
 

bool VME::Get_Data(VME_args* args){  

  std::queue<zmq::message_t> message_queue;
  if(args->m_utils->ReceiveMessages(args->m_data_receive, message_queue)){
    //printf("r1\n");

    if(message_queue.size()>=4){
      //printf("r2\n");
      zmq::message_t identity;
      identity.copy(&message_queue.front());
      message_queue.pop();
      //printf("r3\n");
      zmq::message_t id;
      id.copy(&message_queue.front());
      message_queue.pop();
      //printf("r4\n"); 
      std::istringstream data_type(static_cast<char*>(message_queue.front().data()));
      message_queue.pop();
      //printf("r5\n"); 
     int cards=0;
      memcpy (&cards, message_queue.front().data(), message_queue.front().size());
      message_queue.pop(); 
      //printf("r6\n");
      std::vector<CardData>* tmp_cards=0;
      std::vector<TriggerData*>* tmp_trig=0;
      //printf("r7\n");
      if(data_type.str() == "D") tmp_cards = new std::vector<CardData>;
      if(data_type.str() == "T") tmp_trig = new std::vector<TriggerData*>;
      
      //printf("r8\n");
      bool error=false;

      for(int i=0; i<cards; i++){
	//printf("r9\n");
	if(data_type.str() == "D"){
	  //printf("r10\n");
	  CardData data;//=new CardData;
	  if(data.Receive(message_queue)){;   //havent added error checking and buffer flush
	    //printf("r10.5\n");
	    tmp_cards->push_back(data);
	  }
	  else{
	    //printf("r11\n");
	    //	    delete data;
	    // data=0;
	    args->m_logger->Log("ERROR: Receiving CardData");
	    error=true;   
	    break;  
	  }
	}
	
	else if(data_type.str() == "T"){
	  //printf("r12\n");
	  TriggerData* data=new TriggerData;  
	  if(data->Receive(message_queue)){ 
	    tmp_trig->push_back(data);		  
	  }
	  else{
	    //printf("r13\n");
	    delete data;
	    data=0;
	    args->m_logger->Log("ERROR: Receiving TriggerData"); 
	    error=true;
	    break;  
	  }
	}
	else{
	  //printf("r14\n");
	  args->m_logger->Log("ERROR: unkown data type");
	  error=true;
	  break;
	}
      }
      
      //SEND AN AKN and MESSAGE NUMEBR ID;
      //BEN maybe add poll for outmessage to be sure otherwise will block
      int ok = zmq::poll(&args->out.at(1),1,args->polltimeout);
      //printf("r15.1\n");
      //if( ok<0 || (args->out.at(1).revents & ZMQ_POLLOUT)==0) error=true;
      //printf("r15.2\n");
      if(error){
	unsigned long tmpid=0;
	memcpy(&tmpid, id.data(), id.size());
	tmpid--;
	memcpy(id.data(),&tmpid, id.size());

      }
      //if(error || 
      if((!args->out.at(1).revents & ZMQ_POLLOUT) || (!args->m_data_receive->send(identity, ZMQ_SNDMORE) || !args->m_data_receive->send(id)){
        //printf("r16\n");
	args->m_logger->Log("ERROR: Cant send data aknoledgement",0,0);  
	error=true;	
      }
      //printf("r17\n");
      
      if (!message_queue.size() && !error && cards!=0){
	//printf("r18\n");
	//printf("r18.1 %s\n",data_type.str().c_str());
	if(data_type.str() == "D"){
	  //printf("r19\n");
	  args->data_buffer.push_back(tmp_cards);                                               
	  args->data_counter++;
	  //printf("r20\n");
	}
	
	else if(data_type.str() == "T"){
	  //printf("r20.1\n");
	  args->trigger_buffer.push_back(tmp_trig);                                             
	  args->trigger_counter++;
	  //printf("r20.2\n");
	}
     
	else error=true;
      }
      //printf("r20.3\n");
     
      if (message_queue.size() || error || cards==0){
	//printf("r22 1=%d, 2=%d, 3=%d\n", message_queue.size(), error, cards);
	if(tmp_cards!=0){
	  for(int i=0; i<tmp_cards->size(); i++){
	    //printf("r23\n");	  
	    //  delete tmp_cards->at(i);
	    // tmp_cards->at(i)=0;
	    //printf("r24\n");
	  }
	  tmp_cards->clear();
	}
	
	//printf("r25\n");
	if(tmp_trig!=0){
	  for(int i=0; i<tmp_trig->size(); i++){
	    //printf("r25.1 i=%d\n",i);
	    delete tmp_trig->at(i);
	    //printf("r25.2 i=%d\n",i);
	    tmp_trig->at(i)=0;
	    //printf("r25.3 i=%d\n",i);
	  }
	  //printf("r25.4\n");
	  tmp_trig->clear();
	}
	//printf("r26\n");
	args->m_logger->Log("ERROR: With message format/contents");       
	return false;
      }
      //printf("r26.2\n");
    }
    
    else{
      //printf("r27\n");
      args->m_logger->Log("ERROR: not enough message parts");
      return false;
    }
    
    
  }
  else{
    //printf("r28\n");
    args->m_logger->Log("ERROR:  error getting data from vme crate");      
    return false;
  }
  
  return true;
}


/*bool VME::Get_Data(VME_args* args){  
  
  zmq::message_t identity;
  if(args->m_data_receive->recv(&identity) && identity.more()){
    
    zmq::message_t id;
    if(args->m_data_receive->recv(&id) && id.more()){
      
      zmq::message_t type;
      if(args->m_data_receive->recv(&type) && type.more()){
	std::istringstream data_type(static_cast<char*>(type.data()));
	
	zmq::message_t size;
	if(args->m_data_receive->recv(&size) && size.more()){
	  
	  int cards;
	  memcpy ( &cards, size.data(), size.size() );	
	  
	  std::vector<CardData*>* tmp_cards=0;
	  std::vector<TriggerData*>* tmp_trig=0;
	  
	  if(data_type.str() == "D") tmp_cards = new std::vector<CardData*>;
	  if(data_type.str() == "T") tmp_trig = new std::vector<TriggerData*>;
	  
	  for(int i=0; i<cards; i++){
	    
	    if(data_type.str() == "D"){
	      
	      CardData* data=new CardData;
	      if(data->Receive(args->m_data_receive)){;   //havent added error checking and buffer flush
		tmp_cards->push_back(data);
	      }
	      else{
		delete data;
		args->m_logger->Log("ERROR: Receiving CardData");     
	      }
	    }
	    
	    else if(data_type.str() == "T"){
	      
	      TriggerData* data=new TriggerData;  
	      if(data->Receive(args->m_data_receive)){ 
		tmp_trig->push_back(data);		  
	      }
	      else{
		delete data;
		args->m_logger->Log("ERROR: Receiving TriggerData");   
	      }
	    }
	    else args->m_logger->Log("ERROR: unkown data key");
	  }
	  if(cards>0){
	    if(data_type.str() == "D"){
	      args->data_buffer.push_back(tmp_cards);                                               
	      args->data_counter++;
	    }
	    if(data_type.str() == "T"){
	      args->trigger_buffer.push_back(tmp_trig);                                             
	      args->trigger_counter++;
	    }
	  }
	  else{
	    delete tmp_cards;
	    delete tmp_trig;
	    tmp_cards=0;
	    tmp_trig=0;
	  }
	  
	  //SEND AN AKN and MESSAGE NUMEBR ID;
	  //BEN maybe add poll for outmessage to be sure otherwise will block
	  if(!args->m_data_receive->send(identity) || !args->m_data_receive->send(id)) args->m_logger->Log("ERROR: Cant send data aknoledgement",0,0);  
	  
	   
	  if(data_type.str() != "T" && data_type.str() != "D") args->m_logger->Log("ERROR: Unkown data type",0, 0);
	  
	}
	else args->m_logger->Log("ERROR: Failed to receive size or no data"); 
      }
      else args->m_logger->Log("ERROR: Failed to receive type or no data");
    }
    else args->m_logger->Log("ERROR: Failed to receive msg id or no type");
  }
  else args->m_logger->Log("ERROR:Failed to receive identity or no msg id");
  
  
  return true; 
}

*/

bool VME::VME_To_Store(VME_args* args){ 
  
  if(args->trigger_buffer.size()){
    //send trigger pointer
    zmq::message_t tmp(2);
    snprintf ((char *) tmp.data(), 2 , "%s" , "T") ;
    if(args->m_data_send->send(tmp, ZMQ_SNDMORE) && args->m_utils->SendPointer(args->m_data_send, args->trigger_buffer.at(0))){
      args->trigger_buffer.at(0)=0;
      args->trigger_buffer.pop_front();
    }
    else args->m_logger->Log("ERROR:Failed to send trigger pointer to VME Store Thread");
    
  }
  
  
  
  else if(args->data_buffer.size()){
    //send data pointer
    zmq::message_t tmp(2);           
    snprintf ((char *) tmp.data(), 2 , "%s" , "D") ;
    if(args->m_data_send->send(tmp,ZMQ_SNDMORE) && args->m_utils->SendPointer(args->m_data_send, args->data_buffer.at(0))){
      args->data_buffer.at(0)=0;       
      args->data_buffer.pop_front();        	
    }
    else args->m_logger->Log("ERROR:Failed to send data pointer to VME Store Thread");
    
  }
 
  return true; //BEN do this better
 
}

bool VME::VME_Stats_Send(VME_args* args){

  Store* tmp= new Store;
  tmp->Set("data_counter",args->data_counter);
  tmp->Set("trigger_counter",args->trigger_counter);
  tmp->Set("data_buffer",args->data_buffer.size());
  tmp->Set("trigger_buffer",args->trigger_buffer.size());
  tmp->Set("connections",args->connections.size());
  
  if(!args->m_utils->SendPointer(args->m_trigger_pub, tmp)){
    args->m_logger->Log("ERROR:Failed to send vme status data");  
    delete tmp;
    tmp=0;    
    return false;
  }  //    send store pointer possbile mem leak here as pub socket and not garenteed that main thread will receive, maybe use string //its PAR not pub sub
  
  return true;
}

bool VME::Store_Send_Data(VME_args* args){
   
   zmq::message_t message;
  
  if(args->m_data_send->recv(&message) && !message.more()){
    // send store pointer 
    zmq::poll(args->out, args->storesendpolltimeout);
    
    if(args->out.at(0).revents & ZMQ_POLLOUT){
      
      zmq::message_t key(4);                                                                       
      
      snprintf ((char *) key.data(), 8 , "%s" , "PMTData");
      args->m_data_send->send(key, ZMQ_SNDMORE);
    
      if (args->data_counter==0){
      	delete args->PMTData;
      	args->PMTData=0;
      }
      
      args->m_utils->SendPointer(args->m_data_send, args->PMTData);
      
      args->data_counter=0;
      args->PMTData=new BoostStore(false, 2);
      std::string dn=args->db;
      args->db=args->da;
      args->da=dn;   
      
      zmq::message_t key2(5);
      snprintf ((char *) key2.data(), 9 , "%s" , "TrigData");
      args->m_data_send->send(key2, ZMQ_SNDMORE);
      
      if (args->trigger_counter==0){
      	delete args->TrigData;
      	args->TrigData=0;
      }
      
      args->m_utils->SendPointer(args->m_data_send, args->TrigData);  
      
      args->trigger_counter=0;
      args->TrigData=new BoostStore(false, 2);
      std::string tn=args->tb;
      args->tb=args->ta;
      args->ta=tn;
      
      return true; 
    }
    
  }
  else args->m_logger->Log("ERROR:VME Store Thread: bad store request");

  return false;

}

bool VME::Store_Receive_Data(VME_args* args){

  
  zmq::message_t key_msg;
  if(args->m_data_receive->recv(&key_msg) && key_msg.more()){
    std::istringstream key(static_cast<char*>(key_msg.data()));
    
    if(key.str() == "D"){
      //printf("heyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n");
      std::vector<CardData>* tmp;
      if(args->m_utils->ReceivePointer(args->m_data_receive, tmp)){
	
	//	for(int i=0; i<tmp->size(); i++){
	if(tmp->size()>0){
	  args->PMTData->Set("CardData",*tmp);
	  args->PMTData->Save(args->da);
	  args->PMTData->Delete();
	  args->data_counter++;
	}
	for(int i=0; i<tmp->size(); i++){
	  //	  delete tmp->at(i);
	  // tmp->at(i)=0;
	}
	
	tmp->clear();
	delete tmp;
	tmp=0;
	
	return true;
      }
      else{
	args->m_logger->Log("ERROR:VME Store Thread: failed to receive data pointer");
	return false; 
      }
    }
    
    else if(key.str() == "T"){
      std::vector<TriggerData*>* tmp; // BEN THINK THIS IS NOT A VECTOR
      if(args->m_utils->ReceivePointer(args->m_data_receive, tmp)){
	//printf("o1 size=%d\n",tmp->size());

	for(int i=0; i<tmp->size(); i++){
	  //printf("o1.5 i=%d\n",i); 
	  //printf("o2 pointer=%p\n",tmp->at(i));  
	  args->TrigData->Set("TrigData",*(tmp->at(i)));
	  //printf("o3\n");	
	  args->TrigData->Save(args->ta);
	  //printf("o4\n");
	  args->TrigData->Delete();
	  //printf("o5\n");	 
	  args->trigger_counter++;
	  //printf("o6\n");
	  //printf("o6.2 i=%d\n",i);
	  //printf("o6.5 pointer=%p\n",tmp->at(i));
	  //delete tmp->at(i);
	  //printf("o7\n");
	  tmp->at(i)=0;
	  //printf("o8\n");
	}
	//printf("o9\n");
	
	tmp->clear();
	delete tmp;
	tmp=0;

	return true;
	
      }
      else{
	args->m_logger->Log("ERROR:VME Store Thread: failed to receive trigger data pointer");
	return false;      
      }
    }     
    
    else args->m_logger->Log("ERROR:VME Store Thread: Unkown Key");
    
  }
  else  args->m_logger->Log("ERROR:VME Store Thread: Bad message key or no more");
  
  return false;
}
