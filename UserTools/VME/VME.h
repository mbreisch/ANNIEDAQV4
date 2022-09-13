#ifndef VME_H
#define VME_H

#include <string>
#include <iostream>

#include "Tool.h"

/**
 * \struct VME_args_args
 *
 * This is a struct to place data you want your thread to access or exchange with it. The idea is the datainside is only used by the threa\d and so will be thread safe
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 */

struct VME_args:DAQThread_args{

  VME_args();
  ~VME_args();

  zmq::socket_t* m_trigger_pub;
  zmq::socket_t* m_data_receive;
  zmq::socket_t* m_data_send;

  std::vector<zmq::pollitem_t> in;
  std::vector<zmq::pollitem_t> out;

  DAQUtilities* m_utils;
  Logging* m_logger;

  clock_t* ref_clock1;
  clock_t* ref_clock2;

  std::map<std::string,Store*> connections;

  std::deque<std::vector<CardData*>* > data_buffer;
  std::deque<std::vector<TriggerData*>* > trigger_buffer;

  unsigned long data_counter;
  unsigned long trigger_counter;

  BoostStore* PMTData;
  BoostStore* TrigData;

  std::string da;
  std::string db;

  std::string ta;
  std::string tb;

};

/**
 * \class VME
 *
 * This is a template for a Tool that produces a single thread that can be assigned a function seperate to the main thread. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class VME: public Tool {


 public:

  VME(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  static void VME_Thread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it
  static void Store_Thread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it

  bool VME_Thread_Setup(DataModel* m_data, std::vector<VME_args*> &m_args, DAQUtilities* m_util);
  bool Store_Thread_Setup(DataModel* m_data, std::vector<VME_args*> &m_args, DAQUtilities* m_util);
  static bool Get_Data(VME_args* args);
  static bool VME_To_Store(VME_args* args);
  static bool VME_Stats_Send(VME_args* args);
  static bool Store_Receive_Data(VME_args* args);
  static bool Store_Send_Data(VME_args* args);

  DAQUtilities* m_util;  ///< Pointer to utilities class to help with threading
  std::vector<VME_args*> m_args; ///< thread args (also holds pointer to the thread)

  zmq::pollitem_t items[1];
  zmq::socket_t* trigger_sub;
  
};


#endif
