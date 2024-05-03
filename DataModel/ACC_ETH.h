#ifndef _ACC_ETH_H_INCLUDED
#define _ACC_ETH_H_INCLUDED

#include "ACC.h"
#include "Ethernet.h"
#include "CommandLibrary.h"
#include <iostream>
#include <cstdlib> 
#include <bitset> 
#include <sstream> 
#include <string> 
#include <thread> 
#include <algorithm> 
#include <thread> 
#include <fstream> 
#include <atomic> 
#include <signal.h> 
#include <unistd.h> 
#include <cstring>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>
#include <map>

using namespace std;

#define NUM_CH 30 //maximum number of channels for one ACDC board
#define MAX_NUM_BOARDS 8 // maxiumum number of ACDC boards connectable to one ACC 
#define ACCFRAME 32
#define ACDCFRAME 32 
#define PPSFRAME 16
#define PSECFRAME 7795

class ACC_ETH : public ACC
{
    public:
        //------------------------------------------------------------------------------------//
        //--------------------------------Constructor/Deconstructor---------------------------//
        // >>>> ID 1: Constructor
        ACC_ETH();
        // >>>> ID 2: Constructor with IP and port argument
        ACC_ETH(std::string ip, std::string port);
        // >>>> ID 3: Destructor
        ~ACC_ETH();

        //------------------------------------------------------------------------------------//
        //--------------------------------Local return functions------------------------------//
        vector<unsigned short> ReturnRawData() override {return out_raw_data;}
        vector<unsigned short> ReturnACCIF() override {return out_acc_if;} 
        vector<int> ReturnBoardIndices() override {return out_boardid;}
        vector<unsigned int> ReturnErrors() override
        {
            if(errorcodes.size()==0){errorcodes.push_back(0x0);}
            return errorcodes;
        }

        //------------------------------------------------------------------------------------//
        //-------------------------Local set functions for board setup------------------------//
        //-------------------Sets global variables, see below for description-----------------//
        void SetNumChCoin(unsigned int in) override {SELF_number_channel_coincidence = in;} //sets the number of channels needed for self trigger coincidence	
        void SetEnableCoin(int in) override {SELF_coincidence_onoff = in;} //sets the enable coincidence requirement flag for the self trigger
        void SetThreshold(unsigned int in) override {SELF_threshold = in;} //sets the threshold for the self trigger
        void SetPsecChipMask(vector<int> in) override {SELF_psec_chip_mask = in;} //sets the PSEC chip mask to set individual chips for the self trigger 
        void SetPsecChannelMask(vector<unsigned int> in) override {SELF_psec_channel_mask = in;} //sets the PSEC channel mask to set individual channels for the self trigger 
        void SetValidationStart(unsigned int in) override {validation_start=in;} //sets the validation window start delay for required trigger modes
        void SetValidationWindow(unsigned int in) override {validation_window=in;} //sets the validation window length for required trigger modes
        void SetTriggermode(int in) override {triggersource = in;} //sets the overall triggermode
        void SetSign(int in, int source) override  //sets the sign (normal or inverted) for chosen source
        {
            if(source==2){ACC_sign = in;}
            else if(source==3){ACDC_sign = in;}
            else if(source==4){SELF_sign = in;}
        }
        void SetPPSRatio(unsigned int in) override {PPSRatio = in;} 
        void SetPPSBeamMultiplexer(int in) override {PPSBeamMultiplexer = in;} 
        void SetTimeoutInMs(int in) override {timeoutvalue = in;}

        //------------------------------------------------------------------------------------//
        //-------------------------Local set functions for board setup------------------------//
        //ID 4: Main init function that controls generalk setup as well as trigger settings//
        int InitializeForDataReadout(unsigned int boardmask = 0xFF, int triggersource = 0) override ; 
        //ID 5: Set up the software trigger//
        int SetTriggerSource(unsigned int boardmask = 0xFF, int triggersource = 0) override ; 
        //ID 6: Main listen fuction for data readout. Runs for 5s before retuning a negative//
        int ListenForAcdcData(int triggersource, vector<int> LAPPD_on_ACC) override ; 
        //ID 7: Special function to check connected ACDCs for their firmware version// 
        void VersionCheck() override ;
        //ID 8: Fires the software trigger//
        void GenerateSoftwareTrigger() override ; 
        //ID 9: Tells ACDCs to clear their ram.// 	
        void DumpData(unsigned int boardmask = 0xFF) override ; 
        //ID 10
        void ResetACDC() override ; //resets the acdc boards
        //ID 11
        void ResetACC() override ; //resets the acdc boards 
        //ID 12: Switch PPS input to SMA
        void SetSMA_Debug(unsigned int PPS, unsigned int Beamgate) override ;
        //ID 13: Set Pedestal values
        bool SetPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc) override ;
        //ID 14:
        void PPS_TestMode(int state) override;
        //ID 16
        void WriteErrorLog(string errorMsg);
        //ID 17
        std::vector<unsigned short> CorrectData(std::vector<uint64_t> input_data);
        //ID 18
        void ClearData() override {out_raw_data.clear(); out_boardid.clear(); out_acc_if.clear();}
        //ID 19
        void ClearErrors() override {errorcodes.clear();}

        //-------------------Special functions
        //ID USB-1:	
        bool EmptyUsbLine() override {return false;}

    private:
        //------------------------------------------------------------------------------------//
        //---------------------------------Load neccessary classes----------------------------//
        Ethernet* eth;
        Ethernet* eth_burst;
        CommandLibrary_ACC CML_ACC;
        CommandLibrary_ACDC CML_ACDC;

        //----------all neccessary global variables
        unsigned int command_address; //var: contain command address for ethernet communication
        unsigned long long command_value; //var: contain command value for ethernet communication
        int ACC_sign; //var: ACC sign (normal or inverted)
        int ACDC_sign; //var: ACDC sign (normal or inverted)
        int SELF_sign; //var: self trigger sign (normal or inverted)
        int SELF_coincidence_onoff; //var: flag to enable self trigger coincidence
        int triggersource; //var: decides the triggermode
        int timeoutvalue;
        int PPSBeamMultiplexer; //var: defines the multiplication of the PPS value 
        unsigned int SELF_number_channel_coincidence; //var: number of channels required in coincidence for the self trigger
        unsigned int SELF_threshold; //var: threshold for the selftrigger
        unsigned int validation_start; //var: validation start for some triggermodes
        unsigned int validation_window; //var: validation window for some triggermodes
        unsigned int PPSRatio; //var: defines the multiplication of the PPS value 

        vector<unsigned short> LastACCBuffer;
        vector<int> AcdcIndices; //number relative to ACC index (RJ45 port) corresponds to the connected ACDC boards
        vector<unsigned int> SELF_psec_channel_mask; //var: PSEC channels active for self trigger
        vector<int> SELF_psec_chip_mask; //var: PSEC chips actove for self trigger
        vector<unsigned short> out_raw_data; //var: raw data vector appended with (number of boards)*7795
        vector<unsigned short> out_acc_if; //var: a vector containing different information about the ACC and ACDC
        vector<int> out_boardid;
        
        vector<unsigned int> errorcodes;

        // >>>> ID 0: Sigint handling
        static void got_signal(int);
};

#endif
