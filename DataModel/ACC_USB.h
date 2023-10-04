#ifndef _ACC_USB_H_INCLUDED
#define _ACC_USB_H_INCLUDED

#include "ACC.h"
#include "stdUSB.h" //load the usb class 
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
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

using namespace std;

#define SAFE_BUFFERSIZE 100000 //used in setup procedures to guarantee a proper readout 
#define NUM_CH 30 //maximum number of channels for one ACDC board
#define MAX_NUM_BOARDS 8 // maxiumum number of ACDC boards connectable to one ACC 
#define ACCFRAME 32
#define ACDCFRAME 32
#define PSECFRAME 7795
#define PPSFRAME 16

class ACC_USB : public ACC
{
    public:
        //------------------------------------------------------------------------------------//
        //--------------------------------Constructor/Deconstructor---------------------------//
        // >>>> ID 1: Constructor
        ACC_USB();
        // >>>> ID 3: Destructor
        ~ACC_USB();

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
        
        //------------------------------------------------------------------------------------
        //-------------------------Local set functions for board setup------------------------
        //ID 4: Main init function that controls generalk setup as well as trigger settings//
        int InitializeForDataReadout(unsigned int boardmask = 0xFF, int triggersource = 0) override ; 
        //ID 5: Set up the software trigger//
        int SetTriggerSource(unsigned int boardmask = 0xFF, int triggersource = 0) override ; 
        //ID 6: Main listen fuction for data readout. Runs for 5s before retuning a negative//
        int ListenForAcdcData(int triggersource, vector<int> LAPPD_on_ACC) override ; 
        vector<unsigned short> Temp_Read(int triggersource, vector<int> LAPPD_on_ACC) override {return {};}
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
        bool EmptyUsbLine() override; 
        //ID USB-2:	
        stdUSB* GetUsbStream(); 
        //ID USB-3:	
        int CreateAcdcs(); 
        //ID USB-4:	
        int WhichAcdcsConnected(); 
        //ID USB-5:	
        void EnableTransfer(int onoff=0); 
        //ID USB-6:	
        void ToggleCal(int onoff, unsigned int channelmask = 0x7FFF,  unsigned int boardMask=0xFF); 
        //ID USB-7:	
        void UsbWakeup(); 
        
    private:
        //------------------------------------------------------------------------------------//
        //---------------------------------Load neccessary classes----------------------------//
        stdUSB* usb; //calls the usb class for communication

        //----------all neccessary global variablesint ACC_sign; //var: ACC sign (normal or inverted)
        int ACC_sign;
        int ACDC_sign; //var: ACDC sign (normal or inverted)
        int SELF_sign; //var: self trigger sign (normal or inverted)
        int SELF_coincidence_onoff; //var: flag to enable self trigger coincidence
        int triggersource; //var: decides the triggermode
        int timeoutvalue;
        int PPSBeamMultiplexer;
        unsigned int SELF_number_channel_coincidence; //var: number of channels required in coincidence for the self trigger
        unsigned int SELF_threshold; //var: threshold for the selftrigger
        unsigned int validation_start; //var: validation window for some triggermodes
        unsigned int validation_window; //var: validation window for some triggermodes
        unsigned int PPSRatio;

        vector<unsigned short> LastACCBuffer; //most recently received ACC buffer
        vector<int> AcdcIndices; //number relative to ACC index (RJ45 port) corresponds to the connected ACDC boards
        vector<unsigned int> SELF_psec_channel_mask; //var: PSEC channels active for self trigger
        vector<int> SELF_psec_chip_mask; //var: PSEC chips actove for self trigger
        vector<unsigned short> out_raw_data;
        vector<unsigned short> out_acc_if;
        vector<int> out_boardid;
        vector<unsigned int> errorcodes;

        bool usbcheck;
        
        // >>>> ID 0: Sigint handling
        static void got_signal(int);
};
#endif