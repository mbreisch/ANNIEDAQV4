#ifndef ACC_Receive_H
#define ACC_Receive_H

#include <string>
#include <iostream>

#include "Tool.h"

using namespace std;
/**
 * \class Receive
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ACC_Receive: public Tool {


 public:

  ACC_Receive(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  std::string localconfigfile;
  bool running_old;
  std::string RunStartConfigType;
  std::string RunStartConfigName;
  std::string RunStartConfig;
  std::string RunEndConfigType;
  std::string RunEndConfigName;
  std::string RunEndConfig;


};


#endif
