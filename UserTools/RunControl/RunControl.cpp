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
  
  //m_variables.Print();
  
  m_utils = new DAQUtilities(m_data->context);

  sock = new zmq::socket_t(*m_data->context, ZMQ_PUB);

  m_utils->UpdateConnections("VME", sock, connections, "78787");
  m_utils->UpdateConnections("LAPPD", sock, connections, "78787");

  bool start=true;
  zmq::message_t msg(sizeof(start));
  memcpy(msg.data(), &start, sizeof(start));

  sock->send(msg);

  return true;
}


bool RunControl::Execute(){

  return true;
}


bool RunControl::Finalise(){

  bool start=false;
  zmq::message_t msg(sizeof(start));
  memcpy(msg.data(), &start, sizeof(start));

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
