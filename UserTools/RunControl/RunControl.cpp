#include "RunControl.h"

RunControl::RunControl():Tool(){}


bool RunControl::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  m_data->running=false;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  sock = new zmq::socket_t(*m_data->context, ZMQ_SUB);
  sock->bind("tcp://*:78787");
  sock->setsockopt(ZMQ_SUBSCRIBE,"",0);

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
    std::cout<<"got "<< m_data->running<<std::endl;
  }


  return true;
}


bool RunControl::Finalise(){

  delete sock;
  sock=0;

  return true;
}
