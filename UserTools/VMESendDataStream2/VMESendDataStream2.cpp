#include "VMESendDataStream2.h"

VMESendDataStream2_args::VMESendDataStream2_args(){
						 
  m_utils=0;
  m_logger=0;
  m_data_receive=0;
  m_data_send=0;
  id=0;
  running=0;

}


VMESendDataStream2_args::~VMESendDataStream2_args(){

  m_utils=0;
  m_logger=0;

  delete m_data_receive;
  delete m_data_send;

  m_data_receive=0;
  m_data_send=0;

  in.clear();
  out.clear();

  id=0;

  for(int i=0; i<data_buffer.size(); i++){
    delete data_buffer.at(i);
    data_buffer.at(i)=0;
  }
  
  
  data_buffer.clear();  
  
  for(int i=0; i<trigger_buffer.size(); i++){ 
    delete trigger_buffer.at(i);
    trigger_buffer.at(i)=0;
  } 

  trigger_buffer.clear();

  running=0;
}


VMESendDataStream2::VMESendDataStream2():Tool(){}


bool VMESendDataStream2::Initialise(std::string configfile, DataModel &data){

  m_data= &data;
  m_log= m_data->Log;

  if(m_tool_name=="") m_tool_name="VMESendDataStream2";
  
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
  
  //m_variables.Print();

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  //  if(!m_variables.Get("Crate_Num",m_data->crate_num)) return false;

  m_util=new DAQUtilities(m_data->context);

  //setting up main thread socket


  if(!Thread_Setup(args)){
    Log("ERROR: Tool error setting up crate thread",0,m_verbose);
    return false;// Setting UP Store Thread (BEN error on return)
  }
  

  return true;
}


bool VMESendDataStream2::Execute(){




  return true;
}


bool VMESendDataStream2::Finalise(){

  m_util->KillThread(args);

  delete args;
  args=0;

  delete m_util;
  m_util=0;


  return true;
}

void VMESendDataStream2::Thread(Thread_args* arg){
  
  VMESendDataStream2_args* args=reinterpret_cast<VMESendDataStream2_args*>(arg);
  
  if(*args->running){  
    //printf("VMESendDataStream2::Thread loop\n");
    
    // see if we have CardData from upstream CrateReaderStream tool
    zmq::poll(args->in, 10);
    if((args->in.at(0).revents & ZMQ_POLLIN) && args->data_buffer.size()==0 && args->trigger_buffer.size()==0){
	//printf("VMESendDataStream2 receiving data from CrateReaderStream\n");
	if(!Receive_Data(args)){
        printf("Error receiving data from inproc socket!\n");
      }
    } else {
         //printf("VMESendDataStream2::Thread no new data from CrateReaderStream\n");
    }    

    // see if we have an outgoing connection to the main DAQ
    zmq::poll(args->out, 10);
    if(args->out.at(0).revents & ZMQ_POLLOUT){
      //printf("VMESendDataStream2 sending received data\n");
      Send_Data(args);
      //printf("VMESendDataStream2 sent\n");
    } else {
      //printf("VMESendDataStream2::Thread no listener on output dealer socket tcp://*98989\n");
    }
  
  }else {
    
    //delete any buffered data;
    for(int i=0; i<args->data_buffer.size(); ++i){
        delete args->data_buffer.at(i);
	args->data_buffer.at(i)=0;
    }
    args->data_buffer.clear();
    for(int i=0; i<args->trigger_buffer.size(); ++i){
        delete args->trigger_buffer.at(i);
	args->trigger_buffer.at(i)=0;
    }
    args->trigger_buffer.clear();
  }

}


bool VMESendDataStream2::Thread_Setup(VMESendDataStream2_args* &arg){



  arg=new VMESendDataStream2_args();

  //Ben add timeout and linger settings;
  arg->m_data_send = new zmq::socket_t(*m_data->context, ZMQ_DEALER);
  arg->m_data_send->setsockopt(ZMQ_LINGER,10);
  arg->m_data_send->setsockopt(ZMQ_RCVTIMEO,1000);
  arg->m_data_send->setsockopt(ZMQ_SNDTIMEO,100);
  arg->m_data_send->bind("tcp://*:98989");
  arg->m_data_receive = new zmq::socket_t(*m_data->context, ZMQ_PAIR);
  arg->m_data_receive->setsockopt(ZMQ_LINGER,10);
  arg->m_data_receive->setsockopt(ZMQ_RCVTIMEO,1000);
  arg->m_data_receive->connect("inproc://tobuffer");
  arg->m_utils = m_util;
  arg->m_logger = m_log;
  arg->id=0;

  zmq::pollitem_t tmp_item1;
  tmp_item1.socket=*arg->m_data_receive;
  tmp_item1.fd=0;
  tmp_item1.events=ZMQ_POLLIN;
  tmp_item1.revents=0;
  arg->in.push_back(tmp_item1);


  zmq::pollitem_t tmp_item2;
  tmp_item2.socket=*arg->m_data_send;
  tmp_item2.fd=0;
  tmp_item2.events=ZMQ_POLLOUT;
  tmp_item2.revents=0;
  arg->out.push_back(tmp_item2);
  
  arg->running=&m_data->running;

  return  m_util->CreateThread("Crate", &Thread, args);
      
}

bool VMESendDataStream2::Receive_Data(VMESendDataStream2_args* args){


  zmq::message_t msgtype;
  int got_ok = args->m_data_receive->recv(&msgtype);
  if(!got_ok){
    std::cerr<<"ERROR in VMESendDataStream2::Receive_Data receiving data type!"<<std::endl;
    // POSSIBLE MEMORY LEAK? WHAT HAPPENS TO THE OTHER MESSAGE PARTS, INCLUDING POINTER TO OUR DATA?
    return false;
  }
  std::istringstream type(static_cast<char*>(msgtype.data()));

  if(type.str()=="T"){

    TriggerData* tmp=0;
    got_ok = args->m_utils->ReceivePointer(args->m_data_receive, tmp);
    if(!got_ok || tmp==0){
       std::cerr<<"ERROR in VMESendDataStream2::Receive_Data receiving trigger pointer!"<<std::endl;
       // MEMORY LEAK! POSSIBLE CORRUPT POINTER, DATA IS LOST!
       delete tmp;
       tmp=0;
       return false;
    }
    args->trigger_buffer.push_back(tmp);
  }

  else if (type.str()=="D"){

    std::vector<CardData>* tmp=0;;
    got_ok = args->m_utils->ReceivePointer(args->m_data_receive, tmp);
    if(!got_ok || tmp==0){
       std::cerr<<"ERROR in VMESendDataStream2::Receive_Data receiving data pointer!"<<std::endl;
       // MEMORY LEAK! POSSIBLE CORRUPT POINTER, DATA IS LOST!
       delete tmp;
       tmp=0;      
       return false;
    }
    args->data_buffer.push_back(tmp);

  } else {
    printf("ERROR! VMESendDataStream2::Receive_Data unknown data type '%s'\n",type.str().c_str());
    // POSSIBLE MEMORY LEAK! WHAT HAPPENS TO THE OTHER MESSAGE PARTS, INCLUDING POINTER TO OUR DATA?
    return false;
  }


  return true;
  
}


bool VMESendDataStream2::Send_Data(VMESendDataStream2_args* args){

  if(args->trigger_buffer.size()){
    bool send_ok = true;
    
    zmq::message_t id(sizeof(args->id));
    memcpy(id.data(), &args->id, sizeof(args->id));
    send_ok &= args->m_data_send->send(id, ZMQ_SNDMORE);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send trigger ID!"<<std::endl;
        return false;
    }

    zmq::message_t type(2);
    snprintf ((char *) type.data(), 2 , "%s" , "T") ;
    send_ok &= args->m_data_send->send(type, ZMQ_SNDMORE);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send trigger type!"<<std::endl;
        return false;
    }

    int size=1;

    zmq::message_t cards(sizeof(size));
    memcpy(cards.data(), &size, sizeof(size));
    send_ok &= args->m_data_send->send(cards, ZMQ_SNDMORE);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send size of trigger cards!"<<std::endl;
        return false;
    }


    //printf("VMESendDataStream2 sending next trigger_buffer entry\n");
    send_ok &= args->trigger_buffer.at(0)->Send(args->m_data_send);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send trigger buffer entry 0!"<<std::endl;
        return false;
    }

    zmq::message_t akn;
    if(args->m_data_send->recv(&akn)){
      unsigned long ackid=0;
      memcpy(&ackid, akn.data(), sizeof(unsigned long));
      //printf("ackid=%d, args->id=%d, (ackid!=args->id)=%d \n",ackid, args->id, (ackid!=args->id));
      if(ackid!=args->id){
	printf("Warning: VMESendDataStream2 received ack for mismatching ID for trigger! mID=%d ID=%d\n",ackid,args->id);
      } else {
	//printf("Matching Trigger ID\n");
        //printf("y2\n");
        delete args->trigger_buffer.at(0);
        //printf("y3\n");      
        args->trigger_buffer.at(0)=0;
        //printf("y4\n");
        args->trigger_buffer.pop_front();
        //printf("y5\n");
        ++args->id;
      }
    }


  }



  if( args->data_buffer.size()){
    bool send_ok=true;

    zmq::message_t id(sizeof(args->id));
    memcpy(id.data(), &args->id, sizeof(args->id));
    send_ok &= args->m_data_send->send(id, ZMQ_SNDMORE);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send data ID!"<<std::endl;
        return false;
    }

    zmq::message_t type(2);
    snprintf ((char *) type.data(), 2 , "%s" , "D") ;
    send_ok &= args->m_data_send->send(type, ZMQ_SNDMORE);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send data type!"<<std::endl;
        return false;
    }

    
    //printf("VMESendDataStream2 sending next data_buffer entry\n");
    int size= args->data_buffer.at(0)->size();
    //printf("y8\n");

    zmq::message_t cards(sizeof(size));
    memcpy(cards.data(), &size, sizeof(size));
    send_ok &= args->m_data_send->send(cards, ZMQ_SNDMORE);
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send data size!"<<std::endl;
        return false;
    }


    for(int i=0; i<size; i++){
      //printf("y9\n");
      if(i<size-1 && send_ok) send_ok &= args->data_buffer.at(0)->at(i).Send(args->m_data_send, ZMQ_SNDMORE);
      else if(send_ok) send_ok &= args->data_buffer.at(0)->at(i).Send(args->m_data_send);
      else break;
      //printf("y10\n");
    }
    if(!send_ok){
        std::cerr<<"ERROR VMESendDataStream2::Send_Data failed to send card data!"<<std::endl;
        return false;
    }

    //printf("y10.5\n");   
    zmq::message_t akn;
    if(args->m_data_send->recv(&akn)){
      unsigned long ackid=0;
      memcpy(&ackid, akn.data(), sizeof(ackid));
      if(ackid!=args->id){
         printf("Warning: VMESendDataStream2 received ack for mismatching Data ID!\n");
      } else {
        //printf("y11\n");
	//printf("Matching Data ID\n");
        delete args->data_buffer.at(0);
        args->data_buffer.at(0)=0;
        args->data_buffer.pop_front();
        //printf("y12\n");
        ++args->id;
      }
    }
    //printf("y13\n");   


  }

  return true;

}
