#ifndef LAPPD_H
#define LAPPD_H

#include <string>
#include <iostream>

#include "Tool.h"

/**
 * \struct LAPPD_args_args
 *
 * This is a struct to place data you want your thread to access or exchange with it. The idea is the datainside is only used by the threa\d and so will be thread safe
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 */

struct LAPPD_args:DAQThread_args{

  LAPPD_args();
  ~LAPPD_args();

  zmq::socket_t* m_trigger_pub;
  zmq::socket_t* m_data_receive;
  zmq::socket_t* m_data_send;

  std::vector<zmq::pollitem_t> in;
  std::vector<zmq::pollitem_t> out;
  int polltimeout;
  int storesendpolltimeout;

  DAQUtilities* m_utils;
  Logging* m_logger;

  time_t* ref_time1;
  time_t* ref_time2;
  int statsperiod;

  std::map<std::string,Store*> connections;
  int update_conns_period;
  int portnum;
  std::string servicename;

  std::deque<std::vector<PsecData*>* > data_buffer;
 
  unsigned long data_counter;

  BoostStore* LAPPDData;

  std::string ramdiskpath;
  std::string la;
  std::string lb;


};

/**
 * \class LAPPD
 *
 * This is a template for a Tool that produces a single thread that can be assigned a function seperate to the main thread. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class LAPPD: public Tool {


 public:

  LAPPD(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  static void LAPPD_Thread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it
  static void Store_Thread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it

  bool LAPPD_Thread_Setup(DataModel* m_data, std::vector<LAPPD_args*> &m_args, DAQUtilities* m_util);
  bool Store_Thread_Setup(DataModel* m_data, std::vector<LAPPD_args*> &m_args, DAQUtilities* m_util);
  static bool Get_Data(LAPPD_args* args);
  static bool LAPPD_To_Store(LAPPD_args* args);
  static bool LAPPD_Stats_Send(LAPPD_args* args);
  static bool Store_Receive_Data(LAPPD_args* args);
  static bool Store_Send_Data(LAPPD_args* args);

  DAQUtilities* m_util;  ///< Pointer to utilities class to help with threading
  std::vector<LAPPD_args*> m_args; ///< thread args (also holds pointer to the thread)

  zmq::pollitem_t items[1];
  zmq::socket_t* trigger_sub;
  
};


#endif
