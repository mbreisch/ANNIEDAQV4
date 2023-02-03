#include "test.h"

test::test():Tool(){}


bool test::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  sock= new zmq::socket_t(*(m_data->context), ZMQ_DEALER);
  sock->bind("tcp://*:89765");

  num=0;
  
  return true;
}


bool test::Execute(){

  zmq::message_t msg(sizeof(num));

  memcpy(msg.data(), &num, sizeof(num));

  sock->send(msg);

  num++;

 zmq::message_t msg2;

  sock->recv(&msg2);

  int num2=0;

  memcpy(&num2, msg2.data(), sizeof(num2));
  std::cout<<"num="<<num2<<std::endl;
  
  return true;
}


bool test::Finalise(){

  return true;
}
