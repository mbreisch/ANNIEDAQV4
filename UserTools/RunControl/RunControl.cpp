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
    return false;
  }
  // parse the configuration to populate the m_variables Store.
  std::stringstream configstream(configtext);
  if(configtext!="") m_variables.Initialise(configstream);
  
  // allow overrides from local config file
  if(configfile!="")  m_variables.Initialise(configfile);

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  int portnum=78787;
  m_variables.Get("portnum",portnum);
  
  //m_variables.Print();
  
  m_utils = new DAQUtilities(m_data->context);

  sock = new zmq::socket_t(*m_data->context, ZMQ_PUB);
  polltimeout=10000;
  m_variables.Get("polltimeout", polltimeout);
  sock->setsockopt(ZMQ_SNDTIMEO, polltimeout);


  m_utils->UpdateConnections("VME", sock, connections, std::to_string(portnum));
  m_utils->UpdateConnections("LAPPD", sock, connections, std::to_string(portnum));

  bool start=true;
  zmq::message_t msg(sizeof(start));
  memcpy(msg.data(), &start, sizeof(start));
  
  zmq::pollitem_t outpoll = zmq::pollitem_t{*sock, 0, ZMQ_POLLOUT, 0};
  int ok = zmq::poll(&outpoll, 1, polltimeout);
  if(ok<0){
  	Log(m_tool_name+"::Initialise Error polling output socket! ("+std::to_string(ok)+")",
  	    0, m_verbose);
    return false;
  } else if(outpoll.revents & ZMQ_POLLOUT){
      sock->send(msg);
  } else {
  	Log(m_tool_name+"::Initialise timed out polling output socket!",0,m_verbose);
  	return false;
  }

  return true;
}


bool RunControl::Execute(){

  return true;
}


bool RunControl::Finalise(){

  bool start=false;
  zmq::message_t msg(sizeof(start));
  memcpy(msg.data(), &start, sizeof(start));

  zmq::pollitem_t outpoll = zmq::pollitem_t{*sock, 0, ZMQ_POLLOUT, 0};
  int ok = zmq::poll(&outpoll, 1, polltimeout);
  if(ok<0){
  	Log(m_tool_name+"::Finalise Error polling output socket! ("+std::to_string(ok)+")",
  	    0, m_verbose);
    return false;
  } else if(outpoll.revents & ZMQ_POLLOUT){
      sock->send(msg);
  } else {
  	Log(m_tool_name+"::Finalise timed out polling output socket!",0,m_verbose);
  	return false;
  }
  sock->send(msg);

  delete sock;
  sock=0;

  delete m_utils;
  m_utils=0;

  for (std::map<std::string, Store*>::iterator it= connections.begin(); it!=connections.end(); it++){
    delete it->second;
    it->second=0;
  }

  connections.clear();

  return true;
}
