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
    for (int j=0; j<data_buffer.at(i)->size(); j++){
      delete data_buffer.at(i)->at(j);
      data_buffer.at(i)->at(j)=0;
    }
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
    return false;
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

    Log(status,1,m_verbose);
    
    unsigned long LAPPD_readouts;
    tmp->Get("data_counter",LAPPD_readouts);
  
    m_data->triggers["LAPPD"]= LAPPD_readouts;
   
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
   
  args->m_utils->UpdateConnections("LAPPD", args->m_data_receive, args->connections, 300, args->ref_time1, "PORT");
   
  zmq::poll(args->in, 10);
  
  if(args->in.at(0).revents & ZMQ_POLLIN) Get_Data(args); //new data comming in buffer it and send akn
  
  zmq::poll(args->out, 10);
  
  if((args->out.at(0).revents & ZMQ_POLLOUT) && args->data_buffer.size() ) LAPPD_To_Store(args);         /// data in buffer so send it to store writer 
  
  
  if(difftime(time(NULL),*args->ref_time2) > 60){
    LAPPD_Stats_Send(args);   // on timer stats pub to main thread for triggering
    *args->ref_time2=time(NULL);
  }

}

void LAPPD::Store_Thread(Thread_args* arg){
  
  LAPPD_args* args=reinterpret_cast<LAPPD_args*>(arg);
  
  zmq::poll(args->in, 10);
  
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
  
  tmp_args->la="/mnt/ramdisk/la";
  tmp_args->lb="/mnt/ramdisk/lb";
  
  return m_util->CreateThread("Store", &Store_Thread, tmp_args); // Ben Setup error return correctly
  
}
 

bool LAPPD::Get_Data(LAPPD_args* args){  

  std::queue<zmq::message_t> message_queue;
  if(args->m_utils->ReceiveMessages(args->m_data_receive, message_queue)){

    if(message_queue.size()>=4){

      zmq::message_t identity;
      identity.copy(&message_queue.front());
      message_queue.pop();
      zmq::message_t id;
      id.copy(&message_queue.front());
      message_queue.pop(); 
      int cards=0;
      memcpy (&cards, message_queue.front().data(), message_queue.front().size()); 

      std::vector<PsecData*>* tmp_cards=0;
      
      bool error=false;

      for(int i=0; i<cards; i++){
	
	PsecData* data=new PsecData;
	if(data->Receive(message_queue)){;   //havent added error checking and buffer flush
	  tmp_cards->push_back(data);
	}
	else{
	  delete data;
	  args->m_logger->Log("ERROR: Receiving PsecData");
	  error=true;   
	  break;  
	}
      }
      
      
      //SEND AN AKN and MESSAGE NUMEBR ID;
      //BEN maybe add poll for outmessage to be sure otherwise will block
      if(error || !args->m_data_receive->send(identity) || !args->m_data_receive->send(id)){
	args->m_logger->Log("ERROR: Cant send data aknoledgement",0,0);  
	error=true;	
      }
      
      if (!message_queue.size() && !error && cards!=0){
	
	args->data_buffer.push_back(tmp_cards);                                               
	args->data_counter++;
	
      }
      
      if (message_queue.size() || error || cards==0){
	
	for(int i=0; i<tmp_cards->size(); i++){
	  delete tmp_cards->at(i);
	  tmp_cards->at(i)=0;
	}
	tmp_cards->clear();
	
	
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
    //send data pointer
    if(args->m_utils->SendPointer(args->m_data_send, args->data_buffer.at(0))){
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
    zmq::poll(args->out, 10000);
    
    if(args->out.at(0).revents & ZMQ_POLLOUT){
      
      zmq::message_t key(6);                                                                       
      
      snprintf ((char *) key.data(), 6 , "%s" , "LAPPD");
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

  
  std::vector<PsecData*>* tmp;
  if(args->m_utils->ReceivePointer(args->m_data_receive, tmp)){
    
    for(int i=0; i<tmp->size(); i++){
      
      args->LAPPDData->Set("LAPPDData",*tmp);
      args->LAPPDData->Save(args->la);
      args->LAPPDData->Delete();
      args->data_counter++;
      delete tmp->at(i);
      tmp->at(i)=0;
    }
    
    tmp->clear();
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
