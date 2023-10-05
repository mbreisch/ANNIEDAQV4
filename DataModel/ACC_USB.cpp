#include "ACC_USB.h" 

// >>>> ID:0 Sigint handling
static std::atomic<bool> quitacc(false); 
void ACC_USB::got_signal(int){quitacc.store(true);}

//------------------------------------------------------------------------------------//
//--------------------------------Constructor/Deconstructor---------------------------//

// >>>> ID:1 Constructor
ACC_USB::ACC_USB()
{
	bool clearCheck;
	usb = new stdUSB();
	if(!usb->isOpen())
	{
		errorcodes.push_back(0xACCB0101);
		delete usb;
		exit(EXIT_FAILURE);
	}

	clearCheck = EmptyUsbLine();
	if(clearCheck==false)
	{
		errorcodes.push_back(0xACCB0102);
	}
}

// >>>> ID:3 Destructor 
ACC_USB::~ACC_USB()
{
	bool clearCheck;
	cout << "Calling acc destructor" << endl;
	clearCheck = EmptyUsbLine();
	if(clearCheck==false)
	{
		errorcodes.push_back(0xACCB0301);
	}
	delete usb;
}

//------------------------------------------------------------------------------------//
//---------------------------Setup functions for ACC/ACDC-----------------------------//

// >>>> ID:USB-3 Create ACDC class instances for each connected ACDC board
int ACC_USB::CreateAcdcs()
{
	//To prepare clear the USB line just in case
	bool clearCheck = EmptyUsbLine();
	if(clearCheck==false)
	{
		errorcodes.push_back(0xACCBB301);
	}

	//Check for connected ACDC boards
	int retval = WhichAcdcsConnected(); 
	if(retval==-1)
	{
		std::cout << "Trying to reset ACDC boards" << std::endl;
		unsigned int command = 0xFFFF0000;
		usb->sendData(command);
		usleep(1000000);
		int retval = WhichAcdcsConnected();
		if(retval==-1)
		{
			errorcodes.push_back(0xACCBB302);
		}
	}

	//if there are no ACDCs, return 0
	if(AcdcIndices.size() == 0)
	{
		errorcodes.push_back(0xACCBB303);
		return 0;
	}

	return 1;
}

// >>>> ID:USB-4 Queries the ACC for information about connected ACDC boards
int ACC_USB::WhichAcdcsConnected()
{
	int retval=0;
	unsigned int command;
	vector<int> connectedBoards;

	//New sequence to ask the ACC to reply with the number of boards connected 
	//Disables the PSEC4 frame data transfer for this sequence. Has to be set to HIGH later again
	EnableTransfer(0); 
	usleep(100000);

	//Resets the RX buffer on all 8 ACDC boards
	command = 0x000200FF; 
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB401);}
	//Sends a reset for detected ACDC boards to the ACC
	command = 0x00030000; 
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB402);}
	//Request a 32 word ACDC ID frame containing all important infomations
	command = 0xFFD00000; 
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB403);}

	usleep(100000);

	//Request and read the ACC info buffer and pass it the the corresponding vector
	command=0x00200000;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB405);}
	LastACCBuffer = usb->safeReadData(SAFE_BUFFERSIZE);

	//Check if buffer size is 32 words
	if(LastACCBuffer.size() != ACCFRAME) 
	{
		errorcodes.push_back(0xACCBB406);
		return 0;
	}

	unsigned short alignment_packet = LastACCBuffer.at(7); 
	
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{
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
	AcdcIndices = connectedBoards;
	cout << "Connected Boards: " << AcdcIndices.size() << endl;
	return 1;
}

// >>>> ID:4 Main init function that controls generalk setup as well as trigger settings*/
int ACC_USB::InitializeForDataReadout(unsigned int boardmask, int triggersource)
{
	unsigned int command;
	int retval;

	// Creates ACDCs for readout
	retval = CreateAcdcs();
	if(retval==0)
	{
		errorcodes.push_back(0xACCB0401);
	}

	// Set trigger conditions
	switch(triggersource)
	{ 	
		case 0: //OFF
			errorcodes.push_back(0xACCB0402);	
			break;
		case 1: //Software trigger
			SetTriggerSource(boardmask,triggersource);
			break;
		case 2: //SMA trigger ACC 
			SetTriggerSource(boardmask,triggersource);
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0403);}	
			break;
		case 3: //SMA trigger ACDC 
			SetTriggerSource(boardmask,triggersource);
			command = 0x00B20000;
			command = (command | (boardmask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0404);}	
			break;
		case 4: //Self trigger
			SetTriggerSource(boardmask,triggersource);
			goto selfsetup;

			break;				
		case 5: //Self trigger with SMA validation on ACC
 			SetTriggerSource(boardmask,triggersource);
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0405);}	

			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0406);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0407);}	
			command = 0x00350000;
			command = command | PPSBeamMultiplexer;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0408);}	
			goto selfsetup;
			break;
		case 6: //Self trigger with SMA validation on ACDC
			SetTriggerSource(boardmask,triggersource);
			command = 0x00B20000;
			command = (command | (boardmask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0409);}	
	
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB040a);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB040b);}	
			goto selfsetup;
			break;
		case 7:
			SetTriggerSource(boardmask,triggersource);
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB040c);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB040d);}	
			command = 0x00B20000;
			command = (command | (boardmask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB040e);}	
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB040f);}	
			break;
		case 8:
			SetTriggerSource(boardmask,triggersource);
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0411);}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0412);}	
			command = 0x00B20000;
			command = (command | (boardmask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0413);}	
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0414);}	
			command = 0x00350000;
			command = command | PPSBeamMultiplexer;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0415);}	
			break;
		case 9: 
			SetTriggerSource(boardmask,triggersource);
			break;
		default: // ERROR case
			if(usbcheck==false){errorcodes.push_back(0xACCB0416);}	
			break;
		selfsetup:
 			command = 0x00B10000;

			if(SELF_psec_chip_mask.size()!=SELF_psec_channel_mask.size())
			{
				errorcodes.push_back(0xACCB0417);	
			}
			
			std::vector<unsigned int> CHIPMASK = {0x00000000,0x00001000,0x00002000,0x00003000,0x00004000};
			for(int i=0; i<(int)SELF_psec_chip_mask.size(); i++)
			{		
				command = 0x00B10000;
				command = (command | (boardmask << 24)) | CHIPMASK[i] | SELF_psec_channel_mask[i]; 
				usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0418);}	
			}
			command = 0x00B16000;
			command = (command | (boardmask << 24)) | SELF_sign;
			usbcheck=usb->sendData(command);	if(usbcheck==false){errorcodes.push_back(0xACCB0419);}			
			command = 0x00B15000;
			command = (command | (boardmask << 24)) | SELF_number_channel_coincidence;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB041a);}	
			command = 0x00B18000;
			command = (command | (boardmask << 24)) | SELF_coincidence_onoff;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB041b);}	
			command = 0x00A60000;
			command = (command | (boardmask << 24)) | (0x1F << 12) | SELF_threshold;
			usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB041c);}	
	}
	command = 0x00340000;
	command = command | PPSRatio;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB041d);}	
	
    return 0;
}

// >>>> ID 5: Set up the trigger source
int ACC_USB::SetTriggerSource(unsigned int boardmask, int triggersource)
{	
	//ACC trigger
	unsigned int command = 0x00300000;
	command = (command | (boardmask << 4)) | (unsigned short)triggersource;
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcodes.push_back(0xACCB0501);}

	//ACDC trigger
	command = 0x00B00000;
	command = (command | (boardmask << 24)) | (unsigned short)triggersource;
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcodes.push_back(0xACCB0502);}

    return 0;
}

// >>>> ID USB-6: Switch for the calibration input on the ACC*/
void ACC_USB::ToggleCal(int onoff, unsigned int channelmask, unsigned int boardmask)
{
	unsigned int command = 0x00C00000;
	//the firmware just uses the channel mask to toggle
	//switch lines. So if the cal is off, all channel lines
	//are set to be off. Else, uses channel mask
	if(onoff == 1)
	{
		//channelmas is default 0x7FFF
		command = (command | (boardmask << 24)) | channelmask;
	}else if(onoff == 0)
	{
		command = (command | (boardmask << 24));
	}
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcodes.push_back(0xACCBB601);}
}


//------------------------------------------------------------------------------------//
//---------------------------Read functions listening for data------------------------//

// >>>> ID 6: Main listen fuction for data readout. Runs for # before retuning a timeout
int ACC_USB::ListenForAcdcData(int trigMode, vector<int> LAPPD_on_ACC)
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
   	EnableTransfer(1); 
  	
	//duration variables
	auto start = chrono::steady_clock::now(); //start of the current event listening. 
	auto now = chrono::steady_clock::now(); //just for initialization 
	auto timeoutDuration = chrono::milliseconds(timeoutvalue); // will exit and reinitialize

	while(true)
	{ 
		//Clear the boards read vector
		boardsReadyForRead.clear(); 
		readoutSize.clear();
        LastACCBuffer.clear();

		//Time the listen fuction
		now = chrono::steady_clock::now();
		if(chrono::duration_cast<chrono::milliseconds>(now - start) > timeoutDuration)
		{
			//errorcodes.push_back(0xAC15EE01);
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
			errorcodes.push_back(0xACCB0601);
			clearCheck = EmptyUsbLine();
			if(clearCheck==false)
			{
				errorcodes.push_back(0xACCB0602);
			}
		}
		LastACCBuffer = usb->safeReadData(ACCFRAME);
		if(LastACCBuffer.size()==0)
		{
			continue;
		}else
		{
			out_acc_if = LastACCBuffer;
		}

		//go through all boards on the acc info frame and if 7795 words were transfered note that board
		for(int k: LAPPD_on_ACC)
		{
			if(LastACCBuffer.at(14) & (1 << k))
			{
                unsigned int bit = LastACCBuffer.at(14) & (1 << k);
				if(LastACCBuffer.at(16+k)==PSECFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PSECFRAME;
				}else if(LastACCBuffer.at(16+k)==PPSFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PPSFRAME;
				}else
                {
                    std::cout<<"I broke down"<<std::endl;
                    errorcodes.push_back(0xACCB0603);
                    return 406;
                }
			}
		}

		//old trigger
		if(boardsReadyForRead==LAPPD_on_ACC)
		{
			out_acc_if = LastACCBuffer;
			break;
		}
	}

    //check for mixed buffersizes
    if(readoutSize[LAPPD_on_ACC[0]]!=readoutSize[LAPPD_on_ACC[1]])
    {
        errorcodes.push_back(0xACCB0604);
        return 407;       
    }

	//each ACDC needs to be queried individually
	//by the ACC for its buffer. 
	for(int bi: boardsReadyForRead)
	{
		//base command for set readmode and which board bi to read
		unsigned int command = 0x00210000; 
		command = command | (unsigned int)(bi); 
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0605);}	

		//Tranfser the data to a receive vector
		vector<unsigned short> acdc_buffer = usb->safeReadData(readoutSize[bi]);

		//Handles buffers =/= 7795 words
		if((int)acdc_buffer.size() != readoutSize[bi])
		{
			cout << "Couldn't read " << readoutSize[bi] << " words as expected! Tryingto fix it! Size was: " << acdc_buffer.size() << endl;
			errorcodes.push_back(0xACCB0606);
			return 408;
		}
		if(acdc_buffer[0] != 0x1234)
		{
			acdc_buffer.clear();
            return 409;
		}

		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		out_raw_data.insert(out_raw_data.end(), acdc_buffer.begin(), acdc_buffer.end());
	}
	out_boardid = boardsReadyForRead;
    boardsReadyForRead.clear();
	
    return 0;
}

//------------------------------------------------------------------------------------//
//---------------------------Active functions for informations------------------------//

// >>>> ID 7: Special function to check connected ACDCs for their firmware version 
void ACC_USB::VersionCheck()
{
	unsigned int command;
	
	//Request ACC info frame
	command = 0x00200000; 
	usb->sendData(command);
	
	LastACCBuffer = usb->safeReadData(SAFE_BUFFERSIZE);
	if(LastACCBuffer.size()==ACCFRAME)
	{
		if(LastACCBuffer.at(1)==0xaaaa)
		{
			std::cout << "ACC got the firmware version: " << std::hex << LastACCBuffer.at(2) << std::dec;
			std::cout << " from " << std::hex << LastACCBuffer.at(4) << std::dec << "/" << std::hex << LastACCBuffer.at(3) << std::dec << std::endl;
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

		LastACCBuffer = usb->safeReadData(SAFE_BUFFERSIZE);
		if(LastACCBuffer.size()==ACDCFRAME)
		{
			if(LastACCBuffer.at(1)==0xbbbb)
			{
				std::cout << "Board " << i << " got the firmware version: " << std::hex << LastACCBuffer.at(2) << std::dec;
				std::cout << " from " << std::hex << LastACCBuffer.at(4) << std::dec << "/" << std::hex << LastACCBuffer.at(3) << std::dec << std::endl;
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

//------------------------------------------------------------------------------------//
//-------------------------------------Help functions---------------------------------//

// >>>> ID 8: Fires the software trigger
void ACC_USB::GenerateSoftwareTrigger()
{
	unsigned int command = 0x00100000;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB0801);}	
}

// >>>> ID 9: Tells ACDCs to clear their buffer
void ACC_USB::DumpData(unsigned int boardmask)
{
	unsigned int command = 0x00020000; //base command for set readmode
	command = command | boardmask;
	//send and read. 
	usbcheck=usb->sendData(command);
 	if(usbcheck==false){errorcodes.push_back(0xACCB0901);}
}

// >>>> ID 10: Resets the ACDCs
void ACC_USB::ResetACDC()
{
    unsigned int command = 0xFFFF0000;
    usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB1001);}
    usleep(1000000);
}

// >>>> ID 11: Resets the ACC
void ACC_USB::ResetACC()
{
    unsigned int command = 0x00000000;
    usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB1101);}
    usleep(1000000);
}

// >>>> ID 12: Sets SMA Debug settings
void ACC_USB::SetSMA_Debug(unsigned int PPS, unsigned int Beamgate)
{
	unsigned int command;

    command = 0xFF900000 | PPS;
	usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB1201);}

	usleep(1000000);

    command = 0xFF910000 | Beamgate;
    usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB1202);}
    
	usleep(1000000);
}

// >>>> ID 13: Pedestal setting procedure.
bool ACC_USB::SetPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc)
{
	unsigned int command = 0x00A20000;
	command = (command | (boardmask << 24)) | (chipmask << 12) | adc;
	usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCB1301);}
	return true;
}

// >>>> ID 16: Write function for the error log
void ACC_USB::WriteErrorLog(string errorMsg)
{
    string err = "errorlog.txt";
    cout << "------------------------------------------------------------" << endl;
    cout << errorMsg << endl;
    cout << "------------------------------------------------------------" << endl;

    ofstream os_err(err, ios_base::app);
    os_err << "------------------------------------------------------------" << endl;
    os_err << errorMsg << endl;
    os_err << "------------------------------------------------------------" << endl;
    os_err.close();
}

//ID USB-1:	
bool ACC_USB::EmptyUsbLine()
{
	int send_counter = 0; //number of usb sends
	int max_sends = 10; //arbitrary. 
	unsigned int command = 0x00200000; // a refreshing command
	vector<unsigned short> tempbuff;
	while(true)
	{
		usb->safeReadData(SAFE_BUFFERSIZE);
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB101);}
		send_counter++;
		tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);

		//if it is exactly an ACC buffer size, success. 
		if(tempbuff.size() == ACCFRAME)
		{
			return true;
		}
		if(send_counter > max_sends)
		{
			UsbWakeup();
			tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);
			if(tempbuff.size() == ACCFRAME){
				return true;
			}else{
				errorcodes.push_back(0xACCBB102);
				return false;
 			}
		}
	}
}

//ID USB-2:	
stdUSB* ACC_USB::GetUsbStream()
{
	return usb;
}

//ID USB-5:
void ACC_USB::EnableTransfer(int onoff)
{
	unsigned int command;
	if(onoff == 0)//OFF
	{ 
		command = 0xFFB54000;
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB501);}	
	}else if(onoff == 1)//ON
	{
		command = 0xFFB50000;
		usbcheck=usb->sendData(command); if(usbcheck==false){errorcodes.push_back(0xACCBB102);}	
	}
}

//ID USB-7:	
void ACC_USB::UsbWakeup()
{
	unsigned int command = 0x00200000;
	usbcheck=usb->sendData(command);
	if(usbcheck==false){errorcodes.push_back(0xACCBB701);}
}
