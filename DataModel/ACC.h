#ifndef _ACC_H_INCLUDED
#define _ACC_H_INCLUDED

#include "ACDC.h" // load the ACDC class
#include "stdUSB.h" //load the usb class 
#include <algorithm>
#include <vector>
#include <map>

using namespace std;

#define SAFE_BUFFERSIZE 100000 //used in setup procedures to guarantee a proper readout 
#define NUM_CH 30 //maximum number of channels for one ACDC board
#define MAX_NUM_BOARDS 8 // maxiumum number of ACDC boards connectable to one ACC 
#define ACCFRAME 32
#define ACDCFRAME 32
#define PSECFRAME 7795
#define PPSFRAME 16

class ACC
{
public:
	/*------------------------------------------------------------------------------------*/
	/*--------------------------------Constructor/Deconstructor---------------------------*/
	/*ID 5: Constructor*/
	ACC();
	/*ID 6: Constructor*/
	~ACC();

	/*------------------------------------------------------------------------------------*/
	/*--------------------------------Local return functions------------------------------*/
	/*ID Nan: Returns vector of aligned ACDC indices*/
	vector<int> getAlignedIndices(){return alignedAcdcIndices;}
	/*ID Nan: Returns set triggermode */
	int getTriggermode(){return trigMode;} 
	/*ID Nan: Returns the raw data map*/
	vector<unsigned short> returnRaw(){return vector_raw;}
	vector<int> returnBoardIndices(){return vector_bi;}
	/*ID Nan: Returns the acdc info frame map*/
	map<int, vector<unsigned short>> returnACDCIF(){return map_acdcIF;} 
	/*ID Nan: Returns the acc info frame map*/
	vector<unsigned short> returnACCIF(){return map_accIF;} 
	/*ID Nan: Returns the ACC info frame*/
	vector<unsigned short> getACCInfoFrame();
	/*ID Nan: Error management*/
	void clearErrors(){errorcode.clear();}
	void clearData(){vector_raw.clear(); vector_bi.clear(); map_accIF.clear();}
	vector<unsigned int> returnErrors()
	{
		if(errorcode.size()==0)
		{
			errorcode.push_back(returncode);
		}
		return errorcode;
	}

	/*------------------------------------------------------------------------------------*/
	/*-------------------------Local set functions for board setup------------------------*/
	/*-------------------Sets global variables, see below for description-----------------*/
	void setNumChCoin(unsigned int in){SELF_number_channel_coincidence = in;} 
	void setEnableCoin(int in){SELF_coincidence_onoff = in;} 
	void setThreshold(unsigned int in){SELF_threshold = in;} 
	void setPsecChipMask(vector<int> in){SELF_psec_chip_mask = in;} 
	void setPsecChannelMask(vector<unsigned int> in){SELF_psec_channel_mask = in;} 
	void setValidationStart(unsigned int in){validation_start=in;}
	void setValidationWindow(unsigned int in){validation_window=in;} 
	void setTriggermode(int in){trigMode = in;} 	
	void setSign(int in, int source) 
	{
		if(source==2){ACC_sign = in;}
		else if(source==3){ACDC_sign = in;}
		else if(source==4){SELF_sign = in;}
	}
	void setPPSRatio(unsigned int in){PPSRatio = in;} 
	void setPPSBeamMultiplexer(int in){PPSBeamMultiplexer = in;}
    void setTimeoutInMs(int in){timeoutvalue = in;}
    void setLAPPD1(vector<int> in){LAPPD1 = in;} 
    void setLAPPD2(vector<int> in){LAPPD2 = in;}   
	
	/*------------------------------------------------------------------------------------*/
	/*-------------------------Local set functions for board setup------------------------*/
	/*ID 7: Function to completly empty the USB line until the correct response is received*/
	bool emptyUsbLine(); 
	/*ID 8: USB return*/
	stdUSB* getUsbStream(); 
	/*ID:9 Create ACDC class instances for each connected ACDC board*/
	int createAcdcs(); 
	/*ID 10: Clear all ACDC class instances*/
	void clearAcdcs(); 
	/*ID:11 Queries the ACC for information about connected ACDC boards*/
	int whichAcdcsConnected(); 
	/*ID 12: Set up the software trigger*/
	void setSoftwareTrigger(unsigned int boardMask); 
	/*ID 13: Fires the software trigger*/
	void softwareTrigger(); 
	/*ID 14: Software read function*/
	//int readAcdcBuffers(); 
	/*ID 15: Main listen fuction for data readout. Runs for 5s before retuning a negative*/
	int listenForAcdcData(int trigMode,vector<int> LAPPD_on_ACC); 
	/*ID 16: Used to dis/enable transfer data from the PSEC chips to the buffers*/
	void enableTransfer(int onoff=0); 
	/*ID 17: Main init function that controls generalk setup as well as trigger settings*/
	int initializeForDataReadout(int trigMode = 0,unsigned int boardMask = 0xFF, int calibMode = 0); 
	/*ID 18: Tells ACDCs to clear their ram.*/ 	
	void dumpData(unsigned int boardMask); 
	/*ID 19: Pedestal setting procedure.*/
	bool setPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc); 
	/*ID 20: Switch for the calibration input on the ACC*/
	void toggleCal(int onoff, unsigned int channelmask = 0x7FFF,  unsigned int boardMask=0xFF); 
	/*ID 21: Set up the hardware trigger*/
	void setHardwareTrigSrc(int src, unsigned int boardMask = 0xFF); 
	/*ID 22: Special function to check the ports for connections to ACDCs*/
	void connectedBoards(); 
	/*ID 23: Wakes up the USB by requesting an ACC info frame*/
	void usbWakeup(); 
	/*ID 24: Special function to check connected ACDCs for their firmware version*/ 
	void versionCheck();
	//:::
	void resetACDC(); //resets the acdc boards
	void resetACC(); //resets the acdc boards 
	/*------------------------------------------------------------------------------------*/
	/*--------------------------------Public gloabl variables-----------------------------*/
	vector<unsigned int> errorcode;
	unsigned int returncode = 0x00000000;

	void setSMA_ON(bool PPS,bool Beamgate);
	void setSMA_OFF(bool PPS, bool Beamgate);
	
private:
	/*------------------------------------------------------------------------------------*/
	/*---------------------------------Load neccessary classes----------------------------*/
	stdUSB* usb; //calls the usb class for communication
	vector<ACDC*> acdcs; //a vector of active acdc boards. 

	/*------------------------------------------------------------------------------------*/
	/*-----------------------------Neccessary global variables----------------------------*/
	int ACC_sign; //var: ACC sign (normal or inverted)
	int ACDC_sign; //var: ACDC sign (normal or inverted)
	int SELF_sign; //var: self trigger sign (normal or inverted)
	int SELF_coincidence_onoff; //var: flag to enable self trigger coincidence
	int trigMode; //var: decides the triggermode
	int PPSBeamMultiplexer;
	unsigned int SELF_number_channel_coincidence; //var: number of channels required in coincidence for the self trigger
	unsigned int SELF_threshold; //var: threshold for the selftrigger
	unsigned int validation_start; //var: validation window for some triggermodes
	unsigned int validation_window; //var: validation window for some triggermodes
	unsigned int PPSRatio;
    vector<int> LAPPD1; //Index pair for LAPPD1 - references to the ACC port
    vector<int> LAPPD2; //Index pair for LAPPD2 - references to the ACC port
	vector<unsigned short> lastAccBuffer; //most recently received ACC buffer
	vector<int> alignedAcdcIndices; //number relative to ACC index (RJ45 port) corresponds to the connected ACDC boards
	vector<unsigned int> SELF_psec_channel_mask; //var: PSEC channels active for self trigger
	vector<int> SELF_psec_chip_mask; //var: PSEC chips actove for self trigger
	vector<unsigned short> vector_raw;
	vector<int> vector_bi;
	map<int, vector<unsigned short>> map_acdcIF;
	vector<unsigned short> map_accIF;
	bool usbcheck;
    int timeoutvalue;
	
	static void got_signal(int);
};

#endif
