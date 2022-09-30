#ifndef CrateReaderStream3_H
#define CrateReaderStream3_H

#include <string>
#include <iostream>
#include <time.h>

#include "Tool.h"

/**
 * \struct CrateReaderStream3_args_args
 *
 * This is a struct to place data you want your thread to access or exchange with it. The idea is the datainside is only used by the threa\d and so will be thread safe
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 */

struct CrateReaderStream3_args:Thread_args{

  CrateReaderStream3_args();
  ~CrateReaderStream3_args();

  zmq::socket_t* m_trigger_pub;
  //  zmq::socket_t* m_data_receive;
  zmq::socket_t* m_data_send;

  //std::vector<zmq::pollitem_t> in;
  std::vector<zmq::pollitem_t> out;

  DAQUtilities* m_utils;
  Logging* m_logger;

  time_t* ref_time;

  //std::map<std::string,Store*> connections;

  std::deque<std::vector<CardData>* > data_buffer;
  std::deque<TriggerData*> trigger_buffer;

  unsigned long data_counter;
  unsigned long trigger_counter;

  int crate_num;

  Store start_variables;
  Store stop_variables;

  UC500ADCInterface* Crate;
  ANNIETriggerInterface* TriggerCard;

  //bool triggered;
  bool soft;

  //bool old_triggered;


  bool* running;
  bool old_running;


};

/**
 * \class CrateReaderStream3
 *
 * This is a template for a Tool that produces a single thread that can be assigned a function seperate to the main thread. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class CrateReaderStream3: public Tool {


 public:

  CrateReaderStream3(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  static void Thread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it
  
  bool Thread_Setup(CrateReaderStream3_args* &m_args);
  static bool Get_Data(CrateReaderStream3_args* args);
  static bool VME_To_Send(CrateReaderStream3_args* args);
  static bool VME_Stats_Send(CrateReaderStream3_args* args);
  

  DAQUtilities* m_util;  ///< Pointer to utilities class to help with threading
  CrateReaderStream3_args* args; ///< thread args (also holds pointer to the thread)
  
  zmq::pollitem_t items[1]; 
  zmq::socket_t* trigger_sub;
  
};


#endif
