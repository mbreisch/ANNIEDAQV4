#include "ACC_ETH.h" 

// >>>> ID:0 Sigint handling
static std::atomic<bool> quitacc(false); 
void ACC_ETH::got_signal(int){quitacc.store(true);}

//------------------------------------------------------------------------------------//
//--------------------------------Constructor/Deconstructor---------------------------//

// >>>> ID:1 Constructor
ACC_ETH::ACC_ETH()
{
    eth = new Ethernet("127.0.0.1","5000");
    std::cout << "Normal connected to: " << "127.0.0.1" << ":" << "5000" << std::endl;
    eth_burst = new Ethernet("127.0.0.1","5001");
    std::cout << "Burst connected to: " << "127.0.0.1" << ":" << "5001" << std::endl;
    std::cout << "----------" << std::endl;
}

// >>>> ID:2 Constructor with IP and port arguments
ACC_ETH::ACC_ETH(std::string ip, std::string port)
{
    eth = new Ethernet(ip,port);
    std::cout << "Normal connected to: " << ip << ":" << port << std::endl;
    std::string port_burst = std::to_string(std::stoi(port)+1).c_str();
    eth_burst = new Ethernet(ip,port_burst);
    std::cout << "Burst connected to: " << ip << ":" << port_burst << std::endl;
    std::cout << "----------" << std::endl;
}

// >>>> ID:3 Destructor 
ACC_ETH::~ACC_ETH()
{
	eth->CloseInterface();
    delete eth;
    eth = 0;

    eth_burst->CloseInterface();
    delete eth_burst;
    eth_burst = 0;
    
    std::cout << "ACC destructed" << std::endl;
}

//------------------------------------------------------------------------------------//
//---------------------------Setup functions for ACC/ACDC-----------------------------//

// >>>> ID 4: Main init function that controls generalk setup as well as trigger settings
int ACC_ETH::InitializeForDataReadout(unsigned int boardmask, int triggersource)
{
    if(triggersource<0 || triggersource>9)
    {
        std::cout << "Invalid trigger source chosen, please choose between and including 0 and 9" << std::endl;
        std::cout << "Chosen was :" << triggersource << std::endl;
        errorcodes.push_back(0xACCE0401);
        return -401;
    }

    //Get the connected ACDCs
    command_address = CML_ACC.ACDC_Board_Detect;
    command_value = 0;

    bool ack = eth->SendData(CML_ACC.RX_Buffer_Reset_Request,0xFF,"w");
    if(!ack)
    {
        errorcodes.push_back(0xACCE0402);
    }

    uint64_t DetectedBoards = eth->RecieveDataSingle(command_address,command_value);

    if((DetectedBoards>>16) == 0xeeee)
    {
        errorcodes.push_back(0xACCE0403);
        return -402;
    }

    for(int bi=0; bi<MAX_NUM_BOARDS; bi++)
    {
        if(DetectedBoards & (1<<bi))
        {
            AcdcIndices.push_back(bi);
        }
    }

    std::cout << "Connected boards: " << AcdcIndices.size() << " at ACC-port | ";
    for(int k: AcdcIndices)
    {
        std::cout << k << " | ";
    }
    std::cout << std::endl;

	// Set trigger conditions
    bool ret;
    SetTriggerSource(boardmask,triggersource);

    switch(triggersource)
	{ 	
		case 0: //OFF
            std::cout << "WARNING! Trigger is set to off" << std::endl;
			break;
		case 1: //Software trigger
			break;
		case 2: //SMA trigger ACC 
            command_address = CML_ACC.SMA_Polarity_Select;
            command_value = ACC_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0404);
                printf("Could not send command 0x%08llX with value %i to set ACC SMA sign!\n",command_address,command_value);
            }
			break;
		case 3: //SMA trigger ACDC 
            command_address = CML_ACC.ACDC_Command;
            command_value = CML_ACDC.SMA_Polarity_Select | (boardmask<<24) | ACDC_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0405);
                printf("Could not send command 0x%08llX with value %i to set ACDC SMA sign!\n",command_address,command_value);
            }
			break;
		case 4: //Self trigger
			goto selfsetup;
			break;				
		case 5: //Self trigger with SMA validation on ACC
            command_address = CML_ACC.SMA_Polarity_Select;
            command_value = ACC_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0406);
                printf("Could not send command 0x%08llX with value %i to set ACC SMA sign!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Start;
            command_value = validation_start;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0406);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window start!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Length;
            command_value = validation_window;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0407);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window length!\n",command_address,command_value);
            }

			goto selfsetup;
			break;
		case 6: //Self trigger with SMA validation on ACDC
            command_address = CML_ACC.ACDC_Command;
            command_value = CML_ACDC.SMA_Polarity_Select | (boardmask<<24) | ACDC_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0408);
                printf("Could not send command 0x%08llX with value %i to set ACDC SMA sign!\n",command_address,command_value);
            }
	
            command_address = CML_ACC.Beamgate_Window_Start;
            command_value = validation_start;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0408);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window start!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Length;
            command_value = validation_window;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0409);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window length!\n",command_address,command_value);
            }

			goto selfsetup;
			break;
		case 7:
            command_address = CML_ACC.SMA_Polarity_Select;
            command_value = ACC_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE040a);
                printf("Could not send command 0x%08llX with value %i to set ACC SMA sign!\n",command_address,command_value);
            }

            command_address = CML_ACC.ACDC_Command;
            command_value = CML_ACC.SMA_Polarity_Select | (boardmask<<24) | ACDC_sign; 
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE040b);
                printf("Could not send command 0x%08llX with value %i to set ACDC SMA sign!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Start;
            command_value = validation_start;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE040c);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window start!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Length;
            command_value = validation_window;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE040d);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window length!\n",command_address,command_value);
            }
			break;
		case 8:
            command_address = CML_ACC.SMA_Polarity_Select;
            command_value = ACC_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE040e);
                printf("Could not send command 0x%08llX with value %i to set ACC SMA sign!\n",command_address,command_value);
            }

            command_address = CML_ACC.ACDC_Command;
            command_value = CML_ACC.SMA_Polarity_Select | (boardmask<<24) | ACDC_sign; 
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE040f);
                printf("Could not send command 0x%08llX with value %i to set ACDC SMA sign!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Start;
            command_value = validation_start;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0410);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window start!\n",command_address,command_value);
            }

            command_address = CML_ACC.Beamgate_Window_Length;
            command_value = validation_window;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0411);
                printf("Could not send command 0x%08llX with value %i to set Beamgate window length!\n",command_address,command_value);
            }
			break;
		case 9: 
			break;
		default: // ERROR case
			break;
		selfsetup:
 			command_address = CML_ACC.ACDC_Command;

			if(SELF_psec_chip_mask.size()!=SELF_psec_channel_mask.size())
			{
				std::cout << "Selftrigger mask mismatch" << std::endl;
			}
			
            std::vector<uint64_t> CommandMask = {
                                                    CML_ACDC.Set_Selftrigger_Mask_0,
                                                    CML_ACDC.Set_Selftrigger_Mask_1,
                                                    CML_ACDC.Set_Selftrigger_Mask_2,
                                                    CML_ACDC.Set_Selftrigger_Mask_3,
                                                    CML_ACDC.Set_Selftrigger_Mask_4
                                                };
			for(int i=0; i<(int)SELF_psec_chip_mask.size(); i++)
			{		
                printf("For Chip %i with command 0x%08x tha mask is 0x%08x\n",i,CommandMask[i],SELF_psec_channel_mask[i]);
				command_value = (CommandMask[i] | (boardmask << 24)) | SELF_psec_channel_mask[i]; 
                ret = eth->SendData(command_address,command_value,"w");
                if(!ret)
                {
                    errorcodes.push_back(0xACCE0412);
                    printf("Could not send command 0x%08llX with value %i to set !\n",command_address,command_value);
                }
			}

            command_value = (CML_ACDC.Set_Selftrigger_Sign | (boardmask<<24)) | SELF_sign;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0413);
                printf("Could not send command 0x%08llX with value %i to set Selftrigger sign!\n",command_address,command_value);
            }

            command_value = (CML_ACDC.Set_Selftrigger_Use_Coincidence | (boardmask<<24)) | SELF_coincidence_onoff;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0414);
                printf("Could not send command 0x%08llX with value %i to set Selftrigger Coincidence Switch!\n",command_address,command_value);
            }

            command_value = (CML_ACDC.Set_Selftrigger_Coincidence_Min | (boardmask<<24)) | SELF_number_channel_coincidence;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0415);
                printf("Could not send command 0x%08llX with value %i to set Selftrigger Coincidence number!\n",command_address,command_value);
            }

            command_value = (CML_ACDC.Set_Selftrigger_Threshold | (boardmask<<24)) | (0x1F << 12) | SELF_threshold;
            ret = eth->SendData(command_address,command_value,"w");
            if(!ret)
            {
                errorcodes.push_back(0xACCE0416);
                printf("Could not send command 0x%08llX with value %i to set Selftrigger threshold!\n",command_address,command_value);
            }
	}

    command_address = CML_ACC.PPS_Beamgate_Multiplex;
    command_value = PPSBeamMultiplexer;
    ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE0417);
        printf("Could not send command 0x%08llX with value %i to set Beamgate/PPS Multiplexer!\n",command_address,command_value);
    }

    command_address = CML_ACC.PPS_Divide_Ratio;
    command_value = PPSRatio;
    ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE0418);
        printf("Could not send command 0x%08llX with value %i to set PPS Multiplier!\n",command_address,command_value);
    }

    //Sets up the burst mode
    eth_burst->SwitchToBurst();
    usleep(1000);
    eth_burst->SetBurstState(true);

	return 0;
}

// >>>> ID 5: Set up the trigger source
int ACC_ETH::SetTriggerSource(unsigned int boardmask, int triggersource)
{	
    bool ret;
    for(unsigned int i=0; i<MAX_NUM_BOARDS; i++)
    {
        if(boardmask & (1<<i))
        {
            command_address = CML_ACC.Trigger_Mode_Select | i; 
        }else
        {
            continue;
        }
        command_value = triggersource;

        ret = eth->SendData(command_address,command_value,"w");
        if(!ret)
        {
            errorcodes.push_back(0xACCE0501);
            printf("Could not send command 0x%08llX with value %i to set trigger source!\n",command_address,command_value);
        }
    }

    command_address = CML_ACC.ACDC_Command;
    command_value = CML_ACDC.Set_Triggermode | (boardmask<<24) | triggersource;
    ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE0502);
        printf("Could not send command 0x%08llX with value %i to set trigger source on acdc!\n",command_address,command_value);
    }
    
    return 0;
}

//------------------------------------------------------------------------------------//
//---------------------------Read functions listening for data------------------------//

// >>>> ID 6: Main listen fuction for data readout. Runs for # before retuning a timeout
int ACC_ETH::ListenForAcdcData(int triggersource, vector<int> LAPPD_on_ACC)
{
	vector<int> BoardsReadyForRead;
	map<int,int> ReadoutSize;

	//setup a sigint capturer to safely
	//reset the boards if a ctrl-c signal is found
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = got_signal;
	sigfillset(&sa.sa_mask);
	sigaction(SIGINT,&sa,NULL);

    //Enalble Data Transfer
    bool ret = eth->SendData(CML_ACC.ACDC_Command,CML_ACDC.Enable_Transfer | (0xff<<24) ,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE0601);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
  	
	//duration variables
	auto start = chrono::steady_clock::now(); //start of the current event listening. 
	auto now = chrono::steady_clock::now(); //just for initialization 
	auto printDuration = chrono::milliseconds(10000); //prints as it loops and listens
	auto lastPrint = chrono::steady_clock::now();
	auto timeoutDuration = chrono::milliseconds(timeoutvalue); // will exit and reinitialize

    uint64_t acdcboads = eth->RecieveDataSingle(CML_ACC.ACDC_Board_Detect,0x0);
    uint64_t plllock = eth->RecieveDataSingle(CML_ACC.PLL_Lock_Readback,0x0);
    uint64_t firmwareversion = eth->RecieveDataSingle(CML_ACC.Firmware_Version_Readback,0x0);
    uint64_t external_clock = eth->RecieveDataSingle(CML_ACC.External_CLock_Lock_Readback,0x0);

	while(true)
	{ 
		//Clear the boards read vector
		BoardsReadyForRead.clear(); 
		ReadoutSize.clear();
        LastACCBuffer.clear();
		
		//Time the listen fuction
		now = chrono::steady_clock::now();
		if(chrono::duration_cast<chrono::milliseconds>(now - lastPrint) > printDuration)
		{	
			string err_msg = "Have been waiting for a trigger for ";
			err_msg += to_string(chrono::duration_cast<chrono::milliseconds>(now - start).count());
			err_msg += " seconds";
			WriteErrorLog(err_msg);
			for(int i=0; i<MAX_NUM_BOARDS; i++)
			{
				string err_msg = "Buffer for board ";
				err_msg += to_string(i);
				err_msg += " has ";
				err_msg += to_string(LastACCBuffer.at(16+i));
				err_msg += " words";
				WriteErrorLog(err_msg);
			}
			lastPrint = chrono::steady_clock::now();
		}

		if(chrono::duration_cast<chrono::milliseconds>(now - start) > timeoutDuration)
		{
			return 404;
		}

		//If sigint happens, return value of 3
		if(quitacc.load())
		{
			return 405;
		}

        //Determine buffers and create info frame
        uint64_t buffers_0123 = eth->RecieveDataSingle(CML_ACC.RX_Buffer_Size_Ch0123_Readback,0x0);
        uint64_t buffers_4567 = eth->RecieveDataSingle(CML_ACC.RX_Buffer_Size_Ch4567_Readback,0x0);
        uint64_t datadetect = eth->RecieveDataSingle(CML_ACC.Data_Frame_Receive,0x0);

        LastACCBuffer = {   
                            0x1234, // 0
                            0xAAAA, // 1
                            firmwareversion, // 2
                            plllock, // 3
                            external_clock, // 4
                            acdcboads, // 5
                            datadetect, // 6
                            (buffers_0123 & 0xffff), // 7
                            (buffers_0123 & 0xffff<<16)>>16, // 8
                            (buffers_0123 & 0xffff<<32)>>32, // 9
                            (buffers_0123 & 0xffff<<48)>>48, // 10
                            (buffers_4567 & 0xffff), // 11
                            (buffers_4567 & 0xffff<<16)>>16, // 12
                            (buffers_4567 & 0xffff<<32)>>32, // 13
                            (buffers_4567 & 0xffff<<48)>>48, // 14
                            0xAAAA, // 15
                            0x4321 // 16
                        };

		//go through all boards on the acc info frame and if 7795 words were transfered note that board
		for(int k: LAPPD_on_ACC)
		{
            if(datadetect & (1<<k))
            {
                //Data is seen
                if(LastACCBuffer.at(k+7) == PSECFRAME)
                {
                    //Data matches
                    BoardsReadyForRead.push_back(k);
					ReadoutSize[k] = PSECFRAME;
                }else if(LastACCBuffer.at(k+7) < PSECFRAME)
                {
                    //No data matches
                    // std::stringstream stream;
                    // stream << "0x" << std::hex << std::uppercase << ((allbuffers>>k*16) & 0xffff) ;
                    // std::string HexString = stream.str();
                    // std::string err_msg = "Seen data bit for " + std::to_string(k) + " but there was no matching buffer: " + HexString;
                    // WriteErrorLog(err_msg);
                }else
                {
                    printf("1-4: 0x%016llx | 5-8: 0x%016llx\n",buffers_0123,buffers_4567);

                    ret = eth->SendData(CML_ACC.Read_ACDC_Data_Buffer, k,"w");
                    if(!ret){printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);}  

                    printf("Reading %i with %i\n",k,LastACCBuffer.at(k+7));
                    vector<uint64_t> acdc_buffer = eth_burst->RecieveBurst(7796,1,0);
                    printf("Got %i words back\n",acdc_buffer.size());

                    std::string name = "./OneOffBuffer" + to_string(k) + ".txt";
                    ofstream file(name.c_str(),ios_base::out | ios_base::trunc);
                    for(int i=0;i<acdc_buffer.size();i++){file<<i<<" "<<std::hex<<acdc_buffer.at(i)<<std::dec<<endl;}
                    file.close();

                    return 406;
                }
            }else
            {
                //Else is seen
                if(LastACCBuffer.at(k+7) == PPSFRAME)
                {
                    //PPS matches
                    BoardsReadyForRead.push_back(k);
					ReadoutSize[k] = PPSFRAME;
                }
            }
		}

		//old trigger
		if(BoardsReadyForRead==LAPPD_on_ACC)
		{
            out_acc_if = LastACCBuffer;
            out_boardid = BoardsReadyForRead;
			break;
		}
	}

    //check for mixed buffersizes
    if(ReadoutSize[LAPPD_on_ACC[0]]!=ReadoutSize[LAPPD_on_ACC[1]])
    {
        std::string err_msg = "ERR: Read buffer sizes did not match: " + to_string(ReadoutSize[LAPPD_on_ACC[0]]) + " vs " + to_string(ReadoutSize[LAPPD_on_ACC[1]]);
        printf("%s\n", err_msg);
        errorcodes.push_back(0xACCE0602);
        // return 407;       
    }


	//each ACDC needs to be queried individually
	//by the ACC for its buffer. 
	for(int bi: BoardsReadyForRead)
	{
	    vector<unsigned short> acdc_buffer;

        //Here should be the read...
        ret = eth->SendData(CML_ACC.Read_ACDC_Data_Buffer, bi,"w");
        if(!ret)
        {
            errorcodes.push_back(0xACCE0603);
            printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
        }

        acdc_buffer = CorrectData(eth_burst->RecieveBurst(ReadoutSize[bi],1,0));

		// Handles buffers =/= 7795 words
		if((int)acdc_buffer.size() != ReadoutSize[bi])
		{
			std::string err_msg = "Couldn't read " + to_string(ReadoutSize[bi]) + " words as expected! Tryingto fix it! Size was: " + to_string(acdc_buffer.size());
			printf("%s\n",err_msg);
            errorcodes.push_back(0xACCE0604);
            ofstream corpt_file("./corrupt_buffer",ios_base::out | ios_base::trunc);
            for(int l=0; l<acdc_buffer.size();l++){corpt_file<<l<<" "<<std::hex<<acdc_buffer.at(l)<<std::dec<<std::endl;}
            corpt_file.close();
			return 408;
		}

		if(acdc_buffer[0] != 0x1234)
		{
			acdc_buffer.clear();
            errorcodes.push_back(0xACCE0605);
            return 409;
		}

        vector<unsigned short> transfer_vector;
        for (const uint64_t& value : acdc_buffer) 
        {
            if(value <= std::numeric_limits<unsigned short>::max()) 
            {
                transfer_vector.push_back(static_cast<unsigned short>(value));
            }else
            {
                std::cout << "Value " << value << " is too large for unsigned short, skipping." << std::endl;
            }
        }

		out_raw_data.insert(out_raw_data.end(), transfer_vector.begin(), transfer_vector.end());
        transfer_vector.clear();
        acdc_buffer.clear();
	}

	return 0;
}


//------------------------------------------------------------------------------------//
//---------------------------Active functions for informations------------------------//

// >>>> ID 7: Special function to check connected ACDCs for their firmware version 
void ACC_ETH::VersionCheck()
{
    ResetACDC();
    ResetACC();

    //Sets up the burst mode
    eth_burst->SwitchToBurst();
    usleep(100);
    eth_burst->SetBurstState(true);

    eth->SendData(CML_ACC.RX_Buffer_Reset_Request,0xff,"w");

    //Get ACC Info
    uint64_t acc_fw_version = eth->RecieveDataSingle(CML_ACC.Firmware_Version_Readback,0x1);
    //printf("V: 0x%016llx\n",acc_fw_version);
    
    uint64_t acc_fw_date = eth->RecieveDataSingle(CML_ACC.Firmware_Date_Readback,0x1);
    //printf("D: 0x%016llx\n",acc_fw_date);
    
    unsigned int acc_fw_year = (acc_fw_date & 0xffff<<16)>>16;
    unsigned int acc_fw_month = (acc_fw_date & 0xff<<8)>>8;
    unsigned int acc_fw_day = (acc_fw_date & 0xff);

    std::cout << "ACC got the firmware version: " << std::hex << acc_fw_version << std::dec;
    std::cout << " from " << std::hex << acc_fw_year << std::dec << "/" << std::hex << acc_fw_month << std::dec << "/" << std::hex << acc_fw_day << std::dec << std::endl;
    
    uint64_t acdcs_detected = eth->RecieveDataSingle(CML_ACC.ACDC_Board_Detect,0x0);    
    printf("Detected boards 0x%016llx\n",acdcs_detected);

    eth->SendData(CML_ACC.ACDC_Command,CML_ACDC.Disable_Transfer | (0xff<<24),"w");
    usleep(100000);
    eth->SendData(CML_ACC.RX_Buffer_Reset_Request,0xff,"w");
    usleep(100000);
    eth->SendData(CML_ACC.ACDC_Command,CML_ACDC.ID_Frame_Request | (0xff<<24),"w");
    usleep(100000);

    //Get ACDC Info
    for(int bi=0; bi<MAX_NUM_BOARDS; bi++)
    {
    	if(acdcs_detected & (1<<bi))
    	{
    		uint64_t retval = eth->RecieveDataSingle(CML_ACC.RX_Buffer_Size_Readback | bi, 0x1);
    		//printf("Board %i got 0x%016llx\n",bi,retval);

            if(retval==32)
            {
    		    bool ret = eth->SendData(CML_ACC.Read_ACDC_Data_Buffer,bi,"w");
    		
                vector<unsigned short> return_vector = CorrectData(eth_burst->RecieveBurst(ACDCFRAME,1,0));
                // for(auto k: return_vector)
                // {
                //     printf("%016llx\n",k);
                // }
                if(return_vector.size()==32)
                {
                    if(return_vector.at(1)==0xbbbb)
                    {
                        std::cout << "Board " << bi << " got the firmware version: " << std::hex << return_vector.at(2) << std::dec;
                        std::cout << " from " << std::hex << return_vector.at(3) << std::dec << "/" << std::hex << ((return_vector.at(4) & 0xff<<8)>>8) << std::dec << "/" << std::hex << (return_vector.at(4) & 0xff) << std::dec << std::endl;
                    }else
                    {
                        std::cout << "Board " << bi << " got the wrong info frame" << std::endl;
                    }
		        }else
                {
                    std::cout << "Size matches but redback vector does not" << std::endl;
                }
            }else
            {
                std::cout << "The ACDC IF buffer has not 32 words but " << retval << std::endl;
            }
        }else
        {
            std::cout << "ACDC boards " << bi << " was not detected" << endl;
        }
        eth->SendData(CML_ACC.RX_Buffer_Reset_Request,(1<<bi),"w");
    }
}

//------------------------------------------------------------------------------------//
//-------------------------------------Help functions---------------------------------//

// >>>> ID 8: Fires the software trigger
void ACC_ETH::GenerateSoftwareTrigger()
{
    //Software trigger
	command_address = CML_ACC.Generate_Software_Trigger;
    command_value = 0x1;

    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE0801);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
}

// >>>> ID 9: Tells ACDCs to clear their buffer
void ACC_ETH::DumpData(unsigned int boardmask)
{
    std::cout << "Dumping all data" << std::endl;
    command_address = CML_ACC.RX_Buffer_Reset_Request; 
    command_value = boardmask;

    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE0901);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
    usleep(100);
}

// >>>> ID 10: Resets the ACDCs
void ACC_ETH::ResetACDC()
{
	command_address = CML_ACC.ACDC_Command;
    command_value = CML_ACDC.Global_Reset | (0xff<<24);

    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE1001);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
    usleep(5000000);
}

// >>>> ID 11: Resets the ACC
void ACC_ETH::ResetACC()
{
	command_address = CML_ACC.Global_Reset;
    command_value = 0x1;

    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE1101);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
    usleep(5000000);
}

// >>>> ID 12: Sets SMA Debug settings
void ACC_ETH::SetSMA_Debug(unsigned int PPS, unsigned int Beamgate)
{
	command_address = CML_ACC.PPS_Input_Use_SMA;
    command_value = PPS;

    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE1201);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }

	usleep(1000000);

    command_address = CML_ACC.Beamgate_Trigger_Use_SMA;
    command_value = Beamgate;

    ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE1202);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
    
	usleep(1000000);
}


// >>>> ID 13: Pedestal setting procedure.*/
bool ACC_ETH::SetPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc)
{
    command_address = CML_ACC.ACDC_Command;
	command_value = (CML_ACDC.Set_Pedestal_Offset | (boardmask << 24)) | (chipmask << 12) | adc;
    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret)
    {
        errorcodes.push_back(0xACCE1301);
        printf("Could not send command 0x%08llX with value %i to enable transfer!\n",command_address,command_value);
    }
	return true;
}

// >>>> ID 14: Sets PPS Test mode, PPS is generated on the ACCs internal clock
void ACC_ETH::PPS_TestMode(int state)
{
    command_address = CML_ACC.PPS_Test;
    if(state==1)
    {
        command_value = 0x6;
    }else
    {
	    command_value = 0x0;
    }
    bool ret = eth->SendData(command_address,command_value,"w");
    if(!ret){printf("Could not send command 0x%08llX with value %i to set pps test mode!\n",command_address,command_value);}
}

// >>>> ID 16: Write function for the error log
void ACC_ETH::WriteErrorLog(string errorMsg)
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

// >>>> ID 17: Correct the order of the input vector and cast it to short
std::vector<unsigned short> ACC_ETH::CorrectData(std::vector<uint64_t> input_data)
{
    std::vector<unsigned short> corrected_data;

    if(input_data.size()==PSECFRAME+4 || input_data.size()==ACCFRAME+4 || input_data.size()==ACDCFRAME+4 || input_data.size()==PPSFRAME+4)
    {
        input_data.erase(input_data.begin(), input_data.begin()+4);
    }

    if(input_data.size()==32 || input_data.size()==16)
    {
        for(int i_sort=0; i_sort<input_data.size(); i_sort+=4)
        {
            corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-0)));
            corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-1)));
            corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-2)));
            corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-3)));
        }
    }else if(input_data.size()==7795)
    { 
        input_data.push_back(0x0);
        try
        {
            int i_sort;
            for(i_sort=0; i_sort<input_data.size(); i_sort+=4)
            {
                if(input_data.size()-i_sort<4)
                {
                    int how_much_is_left = input_data.size() - i_sort; 
                    for(int i_sort2=how_much_is_left; i_sort2>0; i_sort2--)
                    {
                        corrected_data.push_back(static_cast<unsigned short>(input_data.at(i_sort+i_sort2-1)));
                    }
                    break;
                }
                corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-0)));
                corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-1)));
                corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-2)));
                corrected_data.push_back(static_cast<unsigned short>(input_data.at(3+i_sort-3)));
            }  
        }catch(const std::exception& e)
        {
            std::cerr << "Error in 7795 reordering and casting: " << e.what() << '\n';
        }
        corrected_data.pop_back();
    }

    return corrected_data;
}