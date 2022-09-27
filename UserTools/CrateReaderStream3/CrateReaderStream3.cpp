#include "CrateReaderStream3.h"

CrateReaderStream3_args::CrateReaderStream3_args():Thread_args(){

  m_trigger_pub=0;
  //  m_data_receive=0;
  m_data_send=0;
  m_utils=0;
  m_logger=0;
  ref_time=0;
  data_counter=0;
  trigger_counter=0;  
  Crate=0;
  TriggerCard=0;
  soft=false;
  running=0;
  old_running=false;
  crate_num=0;
}

CrateReaderStream3_args::~CrateReaderStream3_args(){

  delete m_trigger_pub;
  //delete m_data_receive;
  delete m_data_send;

  m_trigger_pub=0;
  //m_data_receive=0;
  m_data_send=0;

  //in.clear();                            
  out.clear();

  m_utils=0;
  m_logger=0;

  delete ref_time;
  ref_time=0;

  /*  
  for (std::map<std::string,Store*>::iterator it=connections.begin(); it!=connections.end(); ++it){
    delete it->second;
    it->second=0;
  }
  
  connections.clear();
  */

  for(int i=0; i<data_buffer.size(); i++){
      delete data_buffer.at(i);
      data_buffer.at(i)=0;
    }
  
  data_buffer.clear();
  
  
  for( int i=0; i<trigger_buffer.size(); i++){
        delete trigger_buffer.at(i);
      trigger_buffer.at(i)=0;
    }

  trigger_buffer.clear();


  
  //  int b=0; 
  //  std::cout<<"inthread start"<<std::endl;
  
  //std::cin>>b;
  delete Crate;
  //std::cout<<"deleted crate"<<std::endl;
  Crate=0;
  //std::cin>>b;
  delete TriggerCard;
  //std::cout<<"deleted triggercard"<<std::endl;
  TriggerCard=0;
  //std::cin>>b;
  start_variables.Delete();
  stop_variables.Delete();
  //std::cout<<"deleted variables "<<std::endl;
  
  soft=false;
  running=0;
  old_running=false;

}


CrateReaderStream3::CrateReaderStream3():Tool(){}


bool CrateReaderStream3::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  if(!m_variables.Get("Crate_Num",m_data->crate_num)) return false;

  m_util=new DAQUtilities(m_data->context);

  //setting up main thread socket

  trigger_sub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  trigger_sub->bind("inproc://Crate_status");


  items[0].socket=*trigger_sub;
  items[0].fd=0;
  items[0].events=ZMQ_POLLIN;
  items[0].revents=0;



  if(!Thread_Setup(args)){
    Log("ERROR: Tool error setting up crate thread",0,m_verbose);
    return false;// Setting UP Store Thread (BEN error on return)
  }
  
  return true;
}


bool CrateReaderStream3::Execute(){

  zmq::poll(&items[0],1,0);
  

  if ((items[0].revents & ZMQ_POLLIN)){
    
    Store* tmp=0;
    
    if(!m_util->ReceivePointer(trigger_sub, tmp) || !tmp) Log("ERROR: receiving vme trigger status",0,m_verbose);
    
    std::string status="";
    *tmp>>status;

    Log(status,1,m_verbose);
    
    unsigned long VME_readouts;
    tmp->Get("data_counter",VME_readouts);
    unsigned long TRIG_readouts;
    tmp->Get("trigger_counter",TRIG_readouts);

    m_data->triggers["VME"]= VME_readouts;
    m_data->triggers["Trig"]= TRIG_readouts;
   
    delete tmp;
    tmp=0;

  }


  return true;
}


bool CrateReaderStream3::Finalise(){

  m_util->KillThread(args);

  delete args;
  args=0;

  delete trigger_sub;
  trigger_sub=0;

  delete m_util;
  m_util=0;

  return true;
}

void CrateReaderStream3::Thread(Thread_args* arg){

  CrateReaderStream3_args* args=reinterpret_cast<CrateReaderStream3_args*>(arg);

  if(*args->running != args->old_running){
    
    args->old_running=*args->running;

     args->Crate->PresetCounters();

     if(*args->running) args->Crate->Initialise(args->start_variables);
     else  args->Crate->Initialise(args->stop_variables);      
    
    if(args->crate_num==1){
       args->TriggerCard->PresetCounters();
       if(*args->running){
	 args->TriggerCard->Initialise(args->start_variables);
	 args->TriggerCard->EnableTrigger();
       }
       else  args->TriggerCard->Initialise(args->stop_variables);
       
    }
  }
  
  
  if(args->running){  
   
    Get_Data(args); //new data comming in buffer it and send akn
    
    zmq::poll(args->out, 10);
    
    if(args->out.at(0).revents & ZMQ_POLLOUT && (args->data_buffer.size() || args->trigger_buffer.size()) ) VME_To_Send(args);         /// data in buffer so send it to store writer 
    
    if(difftime(time(NULL), *args->ref_time) > 60){
      //    if(clock()-(*args->ref_clock2) >= 60*CLOCKS_PER_SEC){
      VME_Stats_Send(args);   // on timer stats pub to main thread for triggering
      //*args->ref_clock=clock();
      *args->ref_time=time(NULL);
    }
    
  }
  else{   } //delete databuffered ???
  
}


bool CrateReaderStream3::Thread_Setup(CrateReaderStream3_args* arg){



  arg=new CrateReaderStream3_args();

  //Ben add timeout and linger settings;
  arg->m_trigger_pub = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  arg->m_trigger_pub->connect("inproc://Crate_status");
  arg->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  arg->m_data_send->bind("inproc://tobuffer");
  arg->m_utils = m_util;
  arg->m_logger = m_log;
  arg->ref_time=new time_t;
  *arg->ref_time=time(NULL);
  arg->crate_num=m_data->crate_num;
  /// Ben add linger DataSend->setsockopt(ZMQ_LINGER, 0);  

  //BEN load variables from DB


  zmq::pollitem_t tmp_item2;
  tmp_item2.socket=*arg->m_data_send;
  tmp_item2.fd=0;
  tmp_item2.events=ZMQ_POLLOUT;
  tmp_item2.revents=0;
  arg->out.push_back(tmp_item2);
  
  
  arg->Crate= new UC500ADCInterface(args->crate_num);
  arg->TriggerCard= new ANNIETriggerInterface(args->crate_num);
  
  
  arg->running=&m_data->running;
  arg->soft=false;

  arg->old_running=m_data->running;


  return  m_util->CreateThread("Crate", &Thread, args);
      
}


bool CrateReaderStream3::Get_Data(CrateReaderStream3_args* args){  

  bool ret=false;

  //  usleep(10000);
  
  if(args->Crate->Triggered()){ // check if crate has been triggered
    //std::cout<<"m3"<<std::endl;
      
      
    std::vector<CardData>* tmpcarddata=new std::vector<CardData>; //BEN shes leaking nned to either check state of pointer before calling new or better store in a vector so that if messages are unsent they can be sent again in the future
    *(tmpcarddata)=args->Crate->GetCardData();
	
    if(tmpcarddata->size()==0){
      //std::cout<<"no carddata"<<std::endl;
      delete tmpcarddata;
      tmpcarddata=0;
    }
    //else if(carddata->size()>0 && carddata->at(0).Data.size()==0) std::cout<<"no carddata data"<<std::endl;
    else{
      args->data_buffer.push_back(tmpcarddata);
      //	for(int k=0;k<carddata->size();k++){
      
      //if(carddata->at(k).Data.size()==0) std::cout<<"wegot one "<< std::endl;
      //}
      //std::cout<<"m4"<<std::endl;
      args->data_counter++;
      ret=true;
    }
  }
  
  else if(args->soft && args->crate_num==1){
    //std::cout<<"m7"<<std::endl;
    
    args->TriggerCard->ForceTriggerNow();
  }

  
  if(args->crate_num==1){
    //std::cout<<"z2"<<std::endl;
    if(args->TriggerCard->HasData()){
      //std::cout<<"m8"<<std::endl;
      
      
      TriggerData* tmptriggerdata=args->TriggerCard->GetTriggerData();
      
      if(tmptriggerdata->TimeStampData.size()==0){
	//std::cout<<"no trigger data"<<std::endl;
	delete tmptriggerdata;
      }
      else{
	args->trigger_buffer.push_back(tmptriggerdata);
	args->trigger_counter++;
	ret=true;
      }
    }
  }
  
  if (!ret) usleep(100);
  
  return ret;
  
}

bool CrateReaderStream3::VME_To_Send(CrateReaderStream3_args* args){ 
  
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

bool CrateReaderStream3::VME_Stats_Send(CrateReaderStream3_args* args){

  Store* tmp= new Store;
  tmp->Set("data_counter",args->data_counter);
  tmp->Set("trigger_counter",args->trigger_counter);
  tmp->Set("data_buffer",args->data_buffer.size());
  tmp->Set("trigger_buffer",args->trigger_buffer.size());
  //tmp->Set("connections",args->connections.size());
  
  if(!args->m_utils->SendPointer(args->m_trigger_pub, tmp)){
    args->m_logger->Log("ERROR:Failed to send vme status data");  
    delete tmp;
    tmp=0;    
    return false;
  }  //    send store pointer possbile mem leak here as pub socket and not garenteed that main thread will receive, maybe use string //its PAR not pub sub
  
  return true;
}
