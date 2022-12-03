#ifndef CAMAC_H
#define CAMAC_H

#include <string>
#include <iostream>

#include "Lecroy3377.h"
#include "Lecroy4300b.h"
#include "Lecroy4413.h"

#include "Tool.h"

/**
 * \struct CAMAC_args_args
 *
 * This is a struct to place data you want your thread to access or exchange with it. The idea is the datainside is only used by the threa\d and so will be thread safe
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 */

struct CAMAC_args:Thread_args{

  CAMAC_args();
  ~CAMAC_args();

  zmq::socket_t* m_trigger_pub;
  zmq::socket_t* m_data_receive;
  zmq::socket_t* m_data_send;

  std::vector<zmq::pollitem_t> in;
  std::vector<zmq::pollitem_t> out;
  int polltimeout;
  int storesendpolltimeout;

  DAQUtilities* m_utils;
  Logging* m_logger;

  time_t* ref_time;

  MRDData* MRDdata;
  int* trg_pos;
  int* perc;
  std::string DC;

  MRDData::Channel Data;

  std::deque<MRDOut*> data_buffer;

  MRDOut* MRDout;

  unsigned long data_counter;

  BoostStore* CCData;

  std::string ramdiskpath;
  std::string ma;
  std::string mb;

  int statsperiod;

  std::map<std::string, MRDData::Card>::iterator inn;
  std::vector<MRDData::Channel>::iterator is; 
  std::map<int, int>::iterator it;   
  boost::posix_time::ptime *Epoch;

};

/**
 * \class CAMAC
 *
 * This is a template for a Tool that produces a single thread that can be assigned a function seperate to the main thread. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class CAMAC: public Tool {


 public:

  CAMAC(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  static void CAMACThread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it
  static void StoreThread(Thread_args* arg); ///< Function to be run by the thread in a loop. Make sure not to block in it

  bool CAMAC_Thread_Setup(DataModel* m_data, std::vector<CAMAC_args*> &m_args, DAQUtilities* m_util);
  bool Store_Thread_Setup(DataModel* m_data, std::vector<CAMAC_args*> &m_args, DAQUtilities* m_util);
  static bool Get_Data(CAMAC_args* args);
  static bool CAMAC_To_Store(CAMAC_args* args);
  static bool CAMAC_Stats_Send(CAMAC_args* args);
  static bool Store_Receive_Data(CAMAC_args* args);
  static bool Store_Send_Data(CAMAC_args* args);

  inline CamacCrate* Create(std::string cardname, std::string config, int cardslot, int crate);
  inline CamacCrate* Create(std::string cardname, std::istream* config, int cardslot, int crate);
  
  bool SetupCards();

  DAQUtilities* m_util;  ///< Pointer to utilities class to help with threading
  std::vector<CAMAC_args*> m_args; ///< thread args (also holds pointer to the thread)

  zmq::socket_t* trigger_sub;
  zmq::pollitem_t items[1];

  zmq::socket_t* monitoring_pub;

  //CamacCrate* CC;
  std::vector<std::string> Lcard, Ccard;
  std::vector<int> Ncard, Ncrate ;
  
  //int verb;
  int perc;
  std::string configcc="";
  std::string configcctype=""; // 'local' (file) or 'remote' (DB)
  bool trg_mode;
  int trg_pos;
  
  // std::map<std::string, std::vector<CamacCrate*> >::iterator iL;	//Iterates over Module.CC
  //std::vector<CamacCrate*>::iterator iC;				//Iterates over Module.CC<vector>
  std::vector<LeCroy4413*> disc;
  //int threshold;
  //unsigned long reftriggernum;

  MRDData MRDdata;

  // from store save
  //BoostStore* CCData;
  std::string OutPath, OutName, OutNameB;
  std::vector<unsigned int> Value, Slot, Channel;
  std::vector<std::string> Type;
  //  ULong64_t TimeStamp;
  std::string StartTime;


};


#endif
