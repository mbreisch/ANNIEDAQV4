#include "ACC.h" 
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

/*ID:3+4 sigint handling*/
std::atomic<bool> quitacc(false); 
void ACC::got_signal(int){quitacc.store(true);}

/*------------------------------------------------------------------------------------*/
/*--------------------------------Constructor/Deconstructor---------------------------*/

/*ID:5 Constructor*/
ACC::ACC()
{
	bool clearCheck;
	usb = new stdUSB();
	if(!usb->isOpen())
	{
		errorcode.push_back(0xAC05EE01);
		delete usb;
		exit(EXIT_FAILURE);
	}

	clearCheck = emptyUsbLine();
	if(clearCheck==false)
	{
		errorcode.push_back(0xAC05EE02);
	}
}

/*ID:6 Deconstructor*/
ACC::~ACC()
{
	bool clearCheck;
	cout << "Calling acc destructor" << endl;
	clearAcdcs();
	clearCheck = emptyUsbLine();
	if(clearCheck==false)
	{
		errorcode.push_back(0xAC06EE01);
	}
	delete usb;
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Setup functions for ACC/ACDC-----------------------------*/

/*ID:9 Create ACDC class instances for each connected ACDC board*/
int ACC::createAcdcs()
{
	//To prepare clear the USB line just in case
	bool clearCheck = emptyUsbLine();
	if(clearCheck==false)
	{
		errorcode.push_back(0xAC09EE01);
	}

	//Check for connected ACDC boards
	int retval = whichAcdcsConnected(); 
	if(retval==-1)
	{
		std::cout << "Trying to reset ACDC boards" << std::endl;
		unsigned int command = 0xFFFF0000;
		usb->sendData(command);
		usleep(1000000);
		int retval = whichAcdcsConnected();
		if(retval==-1)
		{
			errorcode.push_back(0xAC09EE02);
		}
	}

	//if there are no ACDCs, return 0
	if(alignedAcdcIndices.size() == 0)
	{
		errorcode.push_back(0xAC09EE03);
		return 0;
	}
	
	//Clear the ACDC class vector if one exists
	clearAcdcs();

	//Create ACDC objects with their board numbers
	//loaded into alignedAcdcIndices in the last function call. 
	for(int bi: alignedAcdcIndices)
	{
		ACDC* temp = new ACDC();
		temp->setBoardIndex(bi);
		acdcs.push_back(temp);
	}
	if(acdcs.size()==0)
	{
		errorcode.push_back(0xAC09EE04);
		return 0;
	}

	return 1;
}

/*ID:11 Queries the ACC for information about connected ACDC boards*/
int ACC::whichAcdcsConnected()
{
	int retval=0;
	unsigned int command;
	vector<int> connectedBoards;

	//New sequence to ask the ACC to reply with the number of boards connected 
	//Disables the PSEC4 frame data transfer for this sequence. Has to be set to HIGH later again
	enableTransfer(0); 
	usleep(100000);

	//Resets the RX buffer on all 8 ACDC boards
	command = 0x000200FF; 
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC11EE01);}
	//Sends a reset for detected ACDC boards to the ACC
	command = 0x00030000; 
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC11EE02);}
	//Request a 32 word ACDC ID frame containing all important infomations
	command = 0xFFD00000; 
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC11EE03);}

	usleep(100000);

	//Request and read the ACC info buffer and pass it the the corresponding vector
	command=0x00200000;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC11EE04);}
	lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);

	//Check if buffer size is 32 words
	if(lastAccBuffer.size() != ACCFRAME) 
	{
		errorcode.push_back(0xAC11EE05);
		return 0;
	}

	unsigned short alignment_packet = lastAccBuffer.at(7); 
	
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{
		/* old read for buffersizes, now no longer needed for boardfind 
		if(lastAccBuffer.at(16+i) == ACDCFRAME && (lastAccBuffer.at(14) & (1 << i)))
		{
			cout << "Board "<< i << " with 32 words after ACC buffer read, ";
			cout << "Board "<< i << " connected" << endl;
		}else if(lastAccBuffer.at(16+i) != ACDCFRAME && lastAccBuffer.at(16+i) != 0)
		{
			cout << "Board "<< i << " not with 32 words after ACC buffer read, ";
			cout << "Size " << lastAccBuffer.at(16+i)  << endl;
			retval = -1;
			continue;
		}else if(lastAccBuffer.at(16+i) != ACDCFRAME)
		{
			cout << "Board "<< i << " not with 32 words after ACC buffer read ";
			cout << "Size " << lastAccBuffer.at(16+i)  << endl;
			continue;
		}
		*/
		//(1<<i) should be true if aligned & synced respectively
		if((alignment_packet & (1 << i)))
		{
			//the i'th board is connected
			connectedBoards.push_back(i);
		}
	}
	if(connectedBoards.size()==0)
	{
		return -1;
	}
	//this allows no vector clearing to be needed
	alignedAcdcIndices = connectedBoards;
	cout << "Connected Boards: " << alignedAcdcIndices.size() << endl;
	return 1;
}

/*ID 17: Main init function that controls generalk setup as well as trigger settings*/
int ACC::initializeForDataReadout(int trigMode, unsigned int boardMask, int calibMode)
{
	unsigned int command;
	int retval;

	// Creates ACDCs for readout
	retval = createAcdcs();
	if(retval==0)
	{
		errorcode.push_back(0xAC17EE01);
	}

	// Toogels the calibration mode on if requested
	toggleCal(calibMode, 0x7FFF, boardMask);

	// Set trigger conditions
	switch(trigMode)
	{ 	
		case 0: //OFF
			errorcode.push_back(0xAC17EE02);	
			break;
		case 1: //Software trigger
			setSoftwareTrigger(boardMask);
			break;
		case 2: //SMA trigger ACC 
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE03);}	
			break;
		case 3: //SMA trigger ACDC 
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE04);}	
			break;
		case 4: //Self trigger
			setHardwareTrigSrc(trigMode,boardMask);
			goto selfsetup;

			break;				
		case 5: //Self trigger with SMA validation on ACC
 			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE05);}	

			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE06);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE07);}	
			command = 0x00350000;
			command = command | PPSBeamMultiplexer;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE08);}	
			goto selfsetup;
			break;
		case 6: //Self trigger with SMA validation on ACDC
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE09);}	
	
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE10);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE11);}	
			goto selfsetup;
			break;
		case 7:
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE12);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE13);}	
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE14);}	
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE15);}	
			break;
		case 8:
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE16);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE17);}	
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE18);}	
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE19);}	
			command = 0x00350000;
			command = command | PPSBeamMultiplexer;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE20);}	
			break;
		case 9: 
			setHardwareTrigSrc(trigMode,boardMask);
			break;
		default: // ERROR case
			if(usbcheck==false){errorcode.push_back(0xAC17EE21);}	
			break;
		selfsetup:
 			command = 0x00B10000;

			if(SELF_psec_chip_mask.size()!=SELF_psec_channel_mask.size())
			{
				errorcode.push_back(0xAC17EE22);	
			}
			
			std::vector<unsigned int> CHIPMASK = {0x00000000,0x00001000,0x00002000,0x00003000,0x00004000};
			for(int i=0; i<(int)SELF_psec_chip_mask.size(); i++)
			{		
				command = 0x00B10000;
				command = (command | (boardMask << 24)) | CHIPMASK[i] | SELF_psec_channel_mask[i]; 
				usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE23);}	
			}
			command = 0x00B16000;
			command = (command | (boardMask << 24)) | SELF_sign;
			usbcheck=usb->sendData(command);	if(usbcheck==false){errorcode.push_back(0xAC17EE24);}			
			command = 0x00B15000;
			command = (command | (boardMask << 24)) | SELF_number_channel_coincidence;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE25);}	
			command = 0x00B18000;
			command = (command | (boardMask << 24)) | SELF_coincidence_onoff;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE26);}	
			command = 0x00A60000;
			command = (command | (boardMask << 24)) | (0x1F << 12) | SELF_threshold;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE27);}	
	}
	command = 0x00340000;
	command = command | PPSRatio;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC17EE28);}	
	return 0;
}

/*ID 12: Set up the software trigger*/
void ACC::setSoftwareTrigger(unsigned int boardMask)
{	
	unsigned int command;

	//Set the trigger
	command = 0x00B00001; //Sets the trigger of all ACDC boards to 1 = Software trigger
	command = (command | (boardMask << 24));
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC12EE01);}
	command = 0x00300000; //Sets all ACDC boards to software trigger on the ACC 
	command = (command | (boardMask << 4)) | 1;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC12EE02);}
}

/*ID 21: Set up the hardware trigger*/
void ACC::setHardwareTrigSrc(int src, unsigned int boardMask)
{
	//ACC hardware trigger
	unsigned int command = 0x00300000;
	command = (command | (boardMask << 4)) | (unsigned short)src;
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC21EE01);}
	//ACDC hardware trigger
	command = 0x00B00000;
	command = (command | (boardMask << 24)) | (unsigned short)src;
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC21EE02);}
}

/*ID 20: Switch for the calibration input on the ACC*/
void ACC::toggleCal(int onoff, unsigned int channelmask, unsigned int boardMask)
{
	unsigned int command = 0x00C00000;
	//the firmware just uses the channel mask to toggle
	//switch lines. So if the cal is off, all channel lines
	//are set to be off. Else, uses channel mask
	if(onoff == 1)
	{
		//channelmas is default 0x7FFF
		command = (command | (boardMask << 24)) | channelmask;
	}else if(onoff == 0)
	{
		command = (command | (boardMask << 24));
	}
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC20EE01);}
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Read functions listening for data------------------------*/

/*ID 14: Software read function
int ACC::readAcdcBuffers()
{
	vector<int> boardsReadyForRead;
	map<int,int> readoutSize;
	unsigned int command;
	int maxCounter=0;
	bool clearCheck;
	//Enables the transfer of data from ACDC to ACC
   	enableTransfer(1);
	
	while(true)
	{
		boardsReadyForRead.clear();
		readoutSize.clear();
		//Request the ACC info frame to check buffers
		command = 0x00200000;
		usbcheck=usb->sendData(command);
		if(usbcheck==false)
		{
			errorcode.push_back(0x11001402);
			clearCheck = emptyUsbLine();
			if(clearCheck==false)
			{
				errorcode.push_back(0x31001405);
			}
		}
		lastAccBuffer = usb->safeReadData(ACCFRAME);
		if(lastAccBuffer.size()==0)
		{
			continue;
		}
		for(int k=0; k<MAX_NUM_BOARDS; k++)
		{
			if(lastAccBuffer.at(14) & (1 << k))
			{
				if(lastAccBuffer.at(16+k)==PSECFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PSECFRAME;
				}else if(lastAccBuffer.at(16+k)==PPSFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PPSFRAME;
				}
			}
		}
		//old trigger
		if(boardsReadyForRead==alignedAcdcIndices)
		{
			map_accIF = lastAccBuffer;
			break;
		}
		//new trigger
		std::sort(boardsReadyForRead.begin(), boardsReadyForRead.end());
		bool control = false;
		if(boardsReadyForRead.size()%2==0)
		{
			for(int m=0; m<boardsReadyForRead.size(); m+=2)
			{
				if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={0,1})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={2,3})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={4,5})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={6,7})
				{
					control = true;
				}else
				{
					control = false;
				}
			}
			if(control==true)
			{
				map_accIF = lastAccBuffer;
				break;
			}
		}
		maxCounter++;
		if(maxCounter>500)
		{
			errorcode.push_back(0x21001401);
			return 404;
		}
	}
	for(int bi: boardsReadyForRead)
	{
		//base command for set readmode and which board bi to read
		unsigned int command = 0x00210000; 
		command = command | (unsigned int)(bi); 
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0x31001403);}	
		//Tranfser the data to a receive vector
		vector<unsigned short> acdc_buffer = usb->safeReadData(readoutSize[bi]);
		//Handles buffers =/= 7795 words
		if((int)acdc_buffer.size() != readoutSize[bi])
		{
			cout << "Couldn't read " << readoutSize[bi] << " words as expected! Trying to fix it! Size was: " << acdc_buffer.size() << endl;
			errorcode.push_back(0x31001404);
			return (bi+1);
		}
		if(acdc_buffer[0] != 0x1234)
		{
			acdc_buffer.clear();
		}
		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		for(ACDC* a: acdcs)
		{
			if(a->getBoardIndex() == bi)
			{
				map_raw[bi] = acdc_buffer;
			}
		}
	}
	return 0;
}
*/

/*ID 15: Main listen fuction for data readout. Runs for .5s before retuning a 404*/
int ACC::listenForAcdcData(int trigMode, vector<int> LAPPD_on_ACC)
{
	vector<int> boardsReadyForRead;
	map<int,int> readoutSize;
	unsigned int command; 
	bool clearCheck;

	//setup a sigint capturer to safely
	//reset the boards if a ctrl-c signal is found
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = got_signal;
	sigfillset(&sa.sa_mask);
	sigaction(SIGINT,&sa,NULL);


	//Enables the transfer of data from ACDC to ACC
   	enableTransfer(1); 
  	
	//duration variables
	auto start = chrono::steady_clock::now(); //start of the current event listening. 
	auto now = chrono::steady_clock::now(); //just for initialization 
	auto timeoutDuration = chrono::milliseconds(timeoutvalue); // will exit and reinitialize

	while(true)
	{ 
		//Clear the boards read vector
		boardsReadyForRead.clear(); 
		readoutSize.clear();
        lastAccBuffer.clear();

		//Time the listen fuction
		now = chrono::steady_clock::now();
		if(chrono::duration_cast<chrono::milliseconds>(now - start) > timeoutDuration)
		{
			//errorcode.push_back(0xAC15EE01);
			return 404;
		}

		//If sigint happens, return value of 3
		if(quitacc.load())
		{
			return 405;
		}

		//Request the ACC info frame to check buffers
		command = 0x00200000;
		usbcheck=usb->sendData(command);
		if(usbcheck==false)
		{
			errorcode.push_back(0xAC15EE02);
			clearCheck = emptyUsbLine();
			if(clearCheck==false)
			{
				errorcode.push_back(0xAC15EE03);
			}
		}
		lastAccBuffer = usb->safeReadData(ACCFRAME);
		if(lastAccBuffer.size()==0)
		{
			continue;
		}


		//go through all boards on the acc info frame and if 7795 words were transfered note that board
		for(int k: LAPPD_on_ACC)
		{
			if(lastAccBuffer.at(14) & (1 << k))
			{
				if(lastAccBuffer.at(16+k)==PSECFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PSECFRAME;
				}else if(lastAccBuffer.at(16+k)==PPSFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PPSFRAME;
				}
			}
		}

		//old trigger
		if(boardsReadyForRead==LAPPD_on_ACC)
		{
			map_accIF = lastAccBuffer;
			break;
		}
	}

    //check for mixed buffersizes
    bool chk=true;
    int first = readoutSize.begin()->second;
    for(map<int,int>::iterator it=readoutSize.begin(); it!=readoutSize.end(); it++)
    {
        if(it->second!=first){chk=false;}
    }
    if(chk==false)
    {
        errorcode.push_back(0xAC15EE06);
        return 407;       
    }

	//each ACDC needs to be queried individually
	//by the ACC for its buffer. 
	for(int bi: boardsReadyForRead)
	{
		//base command for set readmode and which board bi to read
		unsigned int command = 0x00210000; 
		command = command | (unsigned int)(bi); 
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC15EE04);}	

		//Tranfser the data to a receive vector
		vector<unsigned short> acdc_buffer = usb->safeReadData(readoutSize[bi]);

		//Handles buffers =/= 7795 words
		if((int)acdc_buffer.size() != readoutSize[bi])
		{
			cout << "Couldn't read " << readoutSize[bi] << " words as expected! Tryingto fix it! Size was: " << acdc_buffer.size() << endl;
			errorcode.push_back(0xAC15EE05);
			return (bi+1);
		}
		if(acdc_buffer[0] != 0x1234)
		{
			acdc_buffer.clear();
            return 406;
		}

		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		for(ACDC* a: acdcs)
		{
			if(a->getBoardIndex() == bi)
			{
				vector_raw.insert(vector_raw.end(), acdc_buffer.begin(), acdc_buffer.end());
			}
		}
	}
	vector_bi = boardsReadyForRead;
    boardsReadyForRead.clear();
	
    return 0;
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Active functions for informations------------------------*/

/*ID 19: Pedestal setting procedure.*/
bool ACC::setPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc)
{
	unsigned int command = 0x00A20000;
	command = (command | (boardmask << 24)) | (chipmask << 12) | adc;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC19EE01);}
	return true;
}

/*ID 22: Special function to check the ports for connections to ACDCs*/ 
void ACC::connectedBoards()
{
	//Disables Psec communication
	unsigned int command = 0xFFB54000; 
	usbcheck = usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC22EE01);}

	//Give the firmware time to disable
	usleep(10000); 

	//Reset the RX buffer
	command = 0x000200FF; 
	usbcheck = usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC22EE02);}
	//Request a 32 word ACDC ID frame containing all important infomations
	command = 0xFFD00000; 
	usbcheck = usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC22EE03);}

	//Give the ACDC time to transfer
	usleep(10000);

	command = 0x00200000; 
	usbcheck=usb->sendData(command);	if(usbcheck==false){errorcode.push_back(0xAC22EE04);}
	lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);

	//Check if ACC buffer size is 32 words
	if(lastAccBuffer.size() != 32) 
	{
		errorcode.push_back(0xAC22EE05);
	}else if(lastAccBuffer.size() == 32)
	{
		//Loop over the ACC buffer words that show the ACDC buffer size
		//32 words represent a connected ACDC
		for(int i = 0; i < MAX_NUM_BOARDS; i++)
		{
			if(lastAccBuffer.at(16+i) == 32){
				cout << "Board "<< i << " not found with 32 words. ";
				cout << "Size was " << lastAccBuffer.at(16+i)  << endl;
			}else if(lastAccBuffer.at(16+i) != 32)
			{
				cout << "Board "<< i << " found with 32 words. ";
				cout << "Board " << i << " is connected!" << endl;
			}
		}
	}
}

/*ID 24: Special function to check connected ACDCs for their firmware version*/ 
void ACC::versionCheck()
{
	unsigned int command;
	
	//Request ACC info frame
	command = 0x00200000; 
	usb->sendData(command);
	
	lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);
	if(lastAccBuffer.size()==ACCFRAME)
	{
		if(lastAccBuffer.at(1)=0xaaaa)
		{
			std::cout << "ACC got the firmware version: " << std::hex << lastAccBuffer.at(2) << std::dec;
			std::cout << " from " << std::hex << lastAccBuffer.at(4) << std::dec << "/" << std::hex << lastAccBuffer.at(3) << std::dec << std::endl;
		}else
		{
			std::cout << "ACC got the wrong info frame" << std::endl;
		}
	}else
	{
		std::cout << "ACC got the no info frame" << std::endl;
	}

	//Disables Psec communication
	command = 0xFFB54000; 
	usb->sendData(command);

	//Give the firmware time to disable
	usleep(10000); 

	//Read the ACC infoframe, use sendAndRead for additional buffersize to prevent
	//leftover words
	command = 0xFFD00000; 
	usb->sendData(command);

	//Loop over the ACC buffer words that show the ACDC buffer size
	//32 words represent a connected ACDC
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{
		command = 0x00210000;
		command = command | i;
		usb->sendData(command);

		lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);
		if(lastAccBuffer.size()==ACDCFRAME)
		{
			if(lastAccBuffer.at(1)=0xbbbb)
			{
				std::cout << "Board " << i << " got the firmware version: " << std::hex << lastAccBuffer.at(2) << std::dec;
				std::cout << " from " << std::hex << lastAccBuffer.at(4) << std::dec << "/" << std::hex << lastAccBuffer.at(3) << std::dec << std::endl;
			}else
			{
				std::cout << "Board " << i << " got the wrong info frame" << std::endl;
			}
		}else
		{
			std::cout << "Board " << i << " is not connected" << std::endl;
		}
	}
}


/*------------------------------------------------------------------------------------*/
/*-------------------------------------Help functions---------------------------------*/

/*ID 13: Fires the software trigger*/
void ACC::softwareTrigger()
{
	unsigned int command = 0xFFB70000;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC13EE01);}	
}

/*ID 16: Used to dis/enable transfer data from the PSEC chips to the buffers*/
void ACC::enableTransfer(int onoff)
{
	unsigned int command;
	if(onoff == 0)//OFF
	{ 
		command = 0xFFB54000;
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC16EE01);}	
	}else if(onoff == 1)//ON
	{
		command = 0xFFB50000;
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC16EE02);}	
	}
}

/*ID 7: Function to completly empty the USB line until the correct response is received*/
bool ACC::emptyUsbLine()
{
	int send_counter = 0; //number of usb sends
	int max_sends = 10; //arbitrary. 
	unsigned int command = 0x00200000; // a refreshing command
	vector<unsigned short> tempbuff;
	while(true)
	{
		usb->safeReadData(SAFE_BUFFERSIZE);
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC07EE01);}
		send_counter++;
		tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);

		//if it is exactly an ACC buffer size, success. 
		if(tempbuff.size() == ACCFRAME)
		{
			return true;
		}
		if(send_counter > max_sends)
		{
			usbWakeup();
			tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);
			if(tempbuff.size() == ACCFRAME){
				return true;
			}else{
				errorcode.push_back(0xAC07EE02);
				return false;
 			}
		}
	}
}

/*ID 8: USB return*/
stdUSB* ACC::getUsbStream()
{
	return usb;
}

/*ID 10: Clear all ACDC class instances*/
void ACC::clearAcdcs()
{
	for(int i = 0; i < (int)acdcs.size(); i++)
	{
		delete acdcs[i];
	}
	acdcs.clear();
}

/*ID 23: Wakes up the USB by requesting an ACC info frame*/
void ACC::usbWakeup()
{
	unsigned int command = 0x00200000;
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcode.push_back(0xAC23EE01);}
}

/*ID 18: Tells ACDCs to clear their ram.*/ 
void ACC::dumpData(unsigned int boardMask)
{
	unsigned int command = 0x00020000; //base command for set readmode
	command = command | boardMask;
	//send and read. 
	usbcheck=usb->sendData(command);
 	if(usbcheck==false){errorcode.push_back(0xAC18EE01);}
}

/*ID 26: Read ACC buffer for Info frame*/
vector<unsigned short> ACC::getACCInfoFrame()
{
	unsigned int command = 0x00200000;	 
	vector<unsigned short> buffer;
	int counter = 0;
	while(counter<5)
	{	
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC26EE01);}
		buffer = usb->safeReadData(SAFE_BUFFERSIZE);
		
		if(buffer.size()!=ACCFRAME)
		{
            buffer.clear();
			counter++;
			errorcode.push_back(0xAC26EE02);
		}else
		{
			//map_accIF = buffer;
			return buffer;	
		}
	}
	return {};
}

/*ID 27: Resets the ACDCs*/
void ACC::resetACDC()
{
		unsigned int command = 0xFFFF0000;
		usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC27EE01);}
		usleep(1000000);
}

/*ID 28: Resets the ACCs*/
void ACC::resetACC()
{
		unsigned int command = 0x00000000;
		usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC28EE01);}
		usleep(1000000);
}

/*---------------------------------New commands--------------------------------------*/
/*ID 29: SMA special input on*/
void ACC::setSMA_ON()
{
	unsigned int command = 0xFF900001;
	usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC29EE01);}
	usleep(1000000);
	command = 0xFF910001;
	usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC29EE02);}
	usleep(1000000);
}

/*ID 30: SMA special input off*/
void ACC::setSMA_OFF()
{
	unsigned int command = 0xFF900000;
	usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC30EE01);}
	usleep(1000000);
	command = 0xFF910000;
	usb->sendData(command); if(usbcheck==false){errorcode.push_back(0xAC30EE02);}
	usleep(1000000);
}