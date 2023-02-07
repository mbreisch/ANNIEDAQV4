#ifndef ACC_SetupBoards_H
#define ACC_SetupBoards_H

#include <string>
#include <iostream>
#include <vector>
#include <queue>

#include "Tool.h"

using namespace std;

#define MAX_NUM_BOARDS 8
#define PSECFRAME 7795



/**
 * \class SetupBoards
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ACC_SetupBoards: public Tool {


 public:

    ACC_SetupBoards(); ///< Simple constructor
    bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute(); ///< Executre function used to perform Tool perpose. 
    bool Finalise(); ///< Finalise funciton used to clean up resorces.

    int Timeoutcounter;
    int TimeoutResetCount; ///< how many timeouts before we reset the ACC

 private:

    bool Setup();
    void LoadDefaults();
    std::string localconfigfile;

};


#endif
