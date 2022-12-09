#include "RunControl.h"

RunControl::RunControl():Tool(){}


bool RunControl::Initialise(std::string configfile, DataModel &data){

  m_data= &data;
  m_log= m_data->Log;

  if(m_tool_name=="") m_tool_name="RunControl";
  
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

  m_data->running=false;
  old_running=false;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  sock = new zmq::socket_t(*m_data->context, ZMQ_SUB);
  sock->bind("tcp://*:78787");
  sock->setsockopt(ZMQ_SUBSCRIBE,"",0);
  sock->setsockopt(ZMQ_LINGER,100);
  sock->setsockopt(ZMQ_RCVTIMEO,100);

  items[0].socket=*sock;
  items[0].fd=0;
  items[0].events=ZMQ_POLLIN;
  items[0].revents=0;

 


  return true;
}


bool RunControl::Execute(){

  zmq::poll(&items[0],1,100);
  
  if ((items[0].revents & ZMQ_POLLIN)){

    zmq::message_t msg;
    sock->recv(&msg);

    memcpy(&m_data->running, msg.data(), sizeof(m_data->running));
 
    // if we've received a 'run start' when we're already running,
    // this implies that we missed a 'run stop'.
    // To force clearing of any buffered data so we start fresh
    // set m_data->running to false so trigger the data acquisition
    // threads to dump any currently held data.
    if(m_data->running && old_running){
      m_data->running=false;
      usleep(500*1000);  // 500ms wait, should be enough
      m_data->running=true;
      
    }

    //    std::cout<<"got "<< m_data->running<<std::endl;

    old_running=m_data->running;
    
  }


  return true;
}


bool RunControl::Finalise(){

  delete sock;
  sock=0;

  return true;
}
