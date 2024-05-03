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
  m_variables.Print();
  
  // allow overrides from local config file
  if(configfile!="")  m_variables.Initialise(configfile);

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
  
  //m_variables.Print();

  m_data->running=false;
  old_running=false;
  
  int portnum = 78787;
  int zmq_linger_ms = 100;
  int zmq_rcvtimeo_ms = 100;
  zmq_polltimeo_ms = 100;
  m_variables.Get("portnum",portnum);
  m_variables.Get("zmq_linger_ms",zmq_linger_ms);
  m_variables.Get("zmq_rcvtimeo_ms",zmq_rcvtimeo_ms);
  m_variables.Get("zmq_polltimeo_ms",zmq_polltimeo_ms);
  
  sock = new zmq::socket_t(*m_data->context, ZMQ_SUB);
  std::string bindaddr = "tcp://*:" + std::to_string(portnum);
  sock->bind(bindaddr.c_str());
  sock->setsockopt(ZMQ_SUBSCRIBE,"",0);
  sock->setsockopt(ZMQ_LINGER,zmq_linger_ms);
  sock->setsockopt(ZMQ_RCVTIMEO,zmq_rcvtimeo_ms);
  
  items[0].socket=*sock;
  items[0].fd=0;
  items[0].events=ZMQ_POLLIN;
  items[0].revents=0;
  
  return true;
}


bool RunControl::Execute(){
  
  m_data->reinit = false;

  zmq::poll(&items[0],1,zmq_polltimeo_ms);

  if ((items[0].revents & ZMQ_POLLIN)){
    
    zmq::message_t msg;
    sock->recv(&msg);
    
    memcpy(&m_data->running, msg.data(), sizeof(m_data->running));
    std::cout<<"got "<< m_data->running<<std::endl;

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
    
    if(old_running==false && m_data->running) m_data->reinit = true;
    
    old_running=m_data->running;
    
  }

  
  return true;
}


bool RunControl::Finalise(){

  delete sock;
  sock=0;
  
  return true;
}
