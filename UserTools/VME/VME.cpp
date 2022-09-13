#include "VME.h"

VME_args::VME_args():DAQThread_args(){

  m_trigger_pub=0;
  m_data_receive=0;
  m_data_send=0;
  m_utils=0;
  m_logger=0;
  ref_clock1=0;
  ref_clock2=0;
  data_counter=0;
  trigger_counter=0;  
  PMTData=0;
  TrigData=0;
  da="";
  db="";
  ta="";
  tb="";

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

  delete ref_clock1;
  delete ref_clock2;
  ref_clock1=0;
  ref_clock2=0;
  
  for (std::map<std::string,Store*>::iterator it=connections.begin(); it!=connections.end(); ++it){
    delete it->second;
    it->second=0;
  }
  
  connections.clear();
  
  for(int i=0; i<data_buffer.size(); i++){
    for (int j=0; j<data_buffer.at(i)->size(); j++){
      delete data_buffer.at(i)->at(j);
      data_buffer.at(i)->at(j)=0;
    }
  }
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

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

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
    
    Log(status,1,m_verbose);
    
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
  
  args->m_utils->UpdateConnections("VME", args->m_data_receive, args->connections, 300, args->ref_clock1);
  
  zmq::poll(args->in, 10);
  
  if(args->in.at(0).revents & ZMQ_POLLIN) Get_Data(args); //new data comming in buffer it and send akn
  
  zmq::poll(args->out, 10);
  
  if(args->out.at(0).revents & ZMQ_POLLOUT && (args->data_buffer.size() || args->trigger_buffer.size()) ) VME_To_Store(args);         /// data in buffer so send it to store writer 
   
  
  if(clock()-(*args->ref_clock2) >= 60*CLOCKS_PER_SEC) VME_Stats_Send(args);   // on timer stats pub to main thread for triggering
   
}

void VME::Store_Thread(Thread_args* arg){
  
  VME_args* args=reinterpret_cast<VME_args*>(arg);
  
  zmq::poll(args->in, 10);
  
  if(args->in.at(1).revents & ZMQ_POLLIN) Store_Send_Data(args); /// received store data request
  
  if(args->in.at(0).revents & ZMQ_POLLIN) Store_Receive_Data(args); // received new data for adding to stores



}


bool VME::VME_Thread_Setup(DataModel* m_data, std::vector<VME_args*> m_args, DAQUtilities* m_util){


  VME_args* tmp_args=new VME_args();
  m_args.push_back(tmp_args);

  tmp_args->m_trigger_pub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_trigger_pub->connect("inproc://VMEtrigger_status");
  tmp_args->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_ROUTER);
  tmp_args->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  tmp_args->m_data_send->bind("inproc://VMEtobuffer");
  tmp_args->m_utils = m_util;
  tmp_args->m_logger = m_log;
  tmp_args->ref_clock1=new clock_t;
  tmp_args->ref_clock2=new clock_t;
  *tmp_args->ref_clock1=clock();
  *tmp_args->ref_clock2=clock();

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


  return m_util->CreateThread("VME", &VME_Thread, tmp_args); ///Ben improve return with catches for socket setup failures

}

bool VME::Store_Thread_Setup(DataModel* m_data, std::vector<VME_args*> m_args, DAQUtilities* m_util){

  VME_args* tmp_args=new VME_args();
  m_args.push_back(tmp_args);
  
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
  
  tmp_args->da="/mnt/ramdisk/da";
  tmp_args->db="/mnt/ramdisk/db";
  tmp_args->ta="/mnt/ramdisk/ta";
  tmp_args->tb="/mnt/ramdisk/tb";
  
  return m_util->CreateThread("Store", &Store_Thread, tmp_args); // Ben Setup error return correctly
  
}
 

bool VME::Get_Data(VME_args* args){  
  
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


bool VME::VME_To_Store(VME_args* args){ 
  
  if(args->trigger_buffer.size()){
    //send trigger pointer
    zmq::message_t tmp(2);
    snprintf ((char *) tmp.data(), 2 , "%s" , "T") ;
    if(args->m_data_send->send(tmp) && args->m_utils->SendPointer(args->m_data_send, args->trigger_buffer.at(0))){
      args->trigger_buffer.at(0)=0;
      args->trigger_buffer.pop_front();
    }
    else args->m_logger->Log("ERROR:Failed to send trigger pointer to VME Store Thread");
    
  }
  
  
  
  else if(args->data_buffer.size()){
    //send data pointer
    zmq::message_t tmp(2);           
    snprintf ((char *) tmp.data(), 2 , "%s" , "D") ;
    if(args->m_data_send->send(tmp) && args->m_utils->SendPointer(args->m_data_send, args->data_buffer.at(0))){
      args->trigger_buffer.at(0)=0;       
      args->trigger_buffer.pop_front();        	
    }
    else args->m_logger->Log("ERROR:Failed to send data pointer to VME Store Thread");
    
  }
 
  return true; //BEN do this better
 
}

bool VME::VME_Stats_Send(VME_args* args){

  *args->ref_clock2=clock();
  
  Store* tmp= new Store;
  tmp->Set("data_counter",args->data_counter);
  tmp->Set("trigger_counter",args->trigger_counter);
  tmp->Set("data_buffer",args->data_buffer.size());
  tmp->Set("trigger_buffer",args->trigger_buffer.size());
  tmp->Set("connections",args->connections.size());
  
  if(!args->m_utils->SendPointer(args->m_trigger_pub, tmp)) args->m_logger->Log("ERROR:Failed to send vme status data");    //    send store pointer possbile mem leak here as pub socket and not garenteed that main thread will receive, maybe use string //its PAR not pub sub
  
}

bool VME::Store_Send_Data(VME_args* args){
   
  zmq::message_t message;
  
  if(args->m_data_send->recv(&message) && !message.more()){
    // send store pointer 
    zmq::poll(args->out, 10000);
    
    if(args->out.at(0).revents & ZMQ_POLLOUT){
      
      zmq::message_t key(3);                                                                       
      
      snprintf ((char *) key.data(), 3 , "%s" , "VME");
      args->m_data_send->send(key, ZMQ_SNDMORE);
      
      args->m_utils->SendPointer(args->m_data_send, args->PMTData);
      
      args->PMTData=new BoostStore(false, 2);
      
      std::string dn=args->db;
      args->db=args->da;
      args->da=dn;   
      
      zmq::message_t key2(4);
      snprintf ((char *) key2.data(), 4 , "%s" , "TRIG");
      args->m_data_send->send(key2, ZMQ_SNDMORE);
      
      args->m_utils->SendPointer(args->m_data_send, args->TrigData);  
      
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
      std::vector<CardData*>* tmp;
      args->m_utils->ReceivePointer(args->m_data_receive, tmp);
      
      for(int i=0; i<tmp->size(); i++){
	
	args->PMTData->Set("CardData",*tmp);
	args->PMTData->Save(args->da);
	args->PMTData->Delete();
      }
      return true;
    }
    
    else if(key.str() == "T"){
      std::vector<TriggerData*>* tmp;
      args->m_utils->ReceivePointer(args->m_data_receive, tmp);
      
      for(int i=0; i<tmp->size(); i++){
	
	args->TrigData->Set("TrigData",*tmp);
	args->TrigData->Save(args->ta);
	args->TrigData->Delete();
	
      }

      return true;
    }     
    
    else args->m_logger->Log("ERROR:VME Store Thread: Unkown Key");
    
  }
  else  args->m_logger->Log("ERROR:VME Store Thread: Bad message key or no more");
  
  return false;
}
