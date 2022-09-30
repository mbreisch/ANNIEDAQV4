#ifndef VMESendDataStream2_H
#define VMESendDataStream2_H

#include <string>
#include <iostream>

#include "Tool.h"


/**
 * \class VMESendDataStream2
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

struct VMESendDataStream2_args:Thread_args{

  VMESendDataStream2_args();
  ~VMESendDataStream2_args();

  DAQUtilities* m_utils;
  Logging* m_logger;

  zmq::socket_t* m_data_receive;
  zmq::socket_t* m_data_send;

  std::vector<zmq::pollitem_t> in;
  std::vector<zmq::pollitem_t> out;

  std::deque<std::vector<CardData>* > data_buffer; 
  std::deque<TriggerData*> trigger_buffer;

  unsigned long id;

  bool* running;

};


class VMESendDataStream2: public Tool {


 public:

  VMESendDataStream2(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  bool Thread_Setup(VMESendDataStream2_args* &arg);

  static void Thread(Thread_args* arg);

  static bool Receive_Data(VMESendDataStream2_args* args);
  static bool Send_Data(VMESendDataStream2_args* args);

  DAQUtilities* m_util;  ///< Pointer to utilities class to help with threading     
  VMESendDataStream2_args* args;



};


#endif
