#include "ACC_SetupBoards.h"

ACC_SetupBoards::ACC_SetupBoards():Tool(){}


bool ACC_SetupBoards::Initialise(std::string configfile, DataModel &data){
	
	m_data= &data;
	m_log= m_data->Log;
	
	if(m_tool_name=="") m_tool_name="ACC_SetupBoards";
	
	// get tool config from database
	std::string configtext;
	bool get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
	if(!get_ok){
		Log(m_tool_name+" Failed to get Tool config from database!",0,0);
		//return false;
	}
	// parse the configuration to populate the m_variables Store.
	std::stringstream configstream(configtext);
	if(configtext!="") m_variables.Initialise(configstream);
	
	// allow overrides from local config file
	if(configfile!="")  m_variables.Initialise(configfile);
	
	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
	
	//m_variables.Print();
	
	//system("mkdir -p Results");
	
	m_data->acc = new ACC();
	
	TimeoutResetCount = 300;
	m_variables.Get("TimeoutResetCount",TimeoutResetCount);
	
	return true;
}


bool ACC_SetupBoards::Execute(){
	
	//if(m_data->conf.receiveFlag==0){return true;}
	for(std::map<int, int>::iterator it=m_data->TCS.Timeoutcounter.begin(); it!=m_data->TCS.Timeoutcounter.end(); ++it)
	{
        if(it->second>=TimeoutResetCount)
        {
            m_data->TCS.Timeoutcounter.at(it->first) = 0; //Reset the timeout counter 
            
            //IF THERE ARE PROBLEMS 
            //COMMENT FROM HERE -----------
            m_data->conf.receiveFlag = 1; //Re-init the Setup part uf the tool
            m_data->conf.RunControl = 0; //Re-clear the buffers
            
            //Print debug frame as overwrite
            vector<unsigned short> PrintFrame = m_data->acc->getACCInfoFrame();
            std::fstream outfile("./configfiles/ReadOutChain/ACCIF.txt", std::ios_base::out | std::ios_base::trunc);
            outfile << "Caused by LAPPD ID " << it->first << std::endl;
            for(int j=0; j<PrintFrame.size(); j++)
            {
                outfile << std::hex << PrintFrame[j] << std::endl; 
            }
            outfile << std::dec;
            outfile.close();
            PrintFrame.clear();
            break;
            //TO HERE -------------
        }
    }
        
	
	bool setupret = false;
	if(m_data->conf.receiveFlag==1)
	{
		if(m_data->conf.RunControl==0 || m_data->conf.RunControl==1)
		{
			//queue<PsecData>().swap(m_data->TCS.Buffer);
            for(std::map<int, queue<PsecData>>::iterator ib=m_data->TCS.Buffer.begin(); ib!=m_data->TCS.Buffer.end(); ++ib)
	        {
                queue<PsecData>().swap(m_data->TCS.Buffer.at(ib->first));
            }       
			m_data->psec.errorcodes.clear();
			m_data->psec.ReceiveData.clear();
			m_data->psec.BoardIndex.clear();
			m_data->psec.AccInfoFrame.clear();
			m_data->psec.RawWaveform.clear();
			m_data->conf.RunControl=-1;
		}
		
		if(m_data->conf.ResetSwitchACC == 1)
		{
			m_data->acc->resetACC();
		}
		if(m_data->conf.ResetSwitchACDC == 1)
		{
			m_data->acc->resetACDC();
		}
		
		setupret = Setup();
		return setupret;
	}
	
	return true;
}


bool ACC_SetupBoards::Finalise(){
	return true;
}


bool ACC_SetupBoards::Setup(){
	
	bool ret=false;
	
	//Set timeout value
	int timeout;
	m_variables.Get("Timeout",timeout);
	m_data->acc->setTimeoutInMs(timeout);
	
	//polarity
	m_data->acc->setSign(m_data->conf.ACC_Sign, 2);
	m_data->acc->setSign(m_data->conf.ACDC_Sign, 3);
	m_data->acc->setSign(m_data->conf.SELF_Sign, 4);
	
	//self trigger options
	m_data->acc->setEnableCoin(m_data->conf.SELF_Enable_Coincidence);
	
	unsigned int coinNum;
	stringstream ss;
	ss << std::hex << m_data->conf.SELF_Coincidence_Number;
	coinNum = std::stoul(ss.str(),nullptr,16);
	m_data->acc->setNumChCoin(coinNum);
	
	unsigned int threshold;
	stringstream ss2;
	ss2 << std::hex << m_data->conf.SELF_threshold;
	threshold = std::stoul(ss2.str(),nullptr,16);
	m_data->acc->setThreshold(threshold);
	
	//psec masks combine
	std::vector<int> PsecChipMask = {m_data->conf.PSEC_Chip_Mask_0,m_data->conf.PSEC_Chip_Mask_1,m_data->conf.PSEC_Chip_Mask_2,m_data->conf.PSEC_Chip_Mask_3,m_data->conf.PSEC_Chip_Mask_4};
	std::vector<unsigned int> VecPsecChannelMask = {m_data->conf.PSEC_Channel_Mask_0,m_data->conf.PSEC_Channel_Mask_1,m_data->conf.PSEC_Channel_Mask_2,m_data->conf.PSEC_Channel_Mask_3,m_data->conf.PSEC_Channel_Mask_4};
	m_data->acc->setPsecChipMask(PsecChipMask);
	m_data->acc->setPsecChannelMask(VecPsecChannelMask);
	
	//validation window
	unsigned int validationStart;
	stringstream ss31;
	ss31 << std::hex << (int)m_data->conf.Validation_Start/25;
	validationStart = std::stoul(ss31.str(),nullptr,16);
	m_data->acc->setValidationStart(validationStart);
	
	unsigned int validationWindow;
	stringstream ss32;
	ss32 << std::hex << (int)m_data->conf.Validation_Window/25;
	validationWindow = std::stoul(ss32.str(),nullptr,16);
	m_data->acc->setValidationWindow(validationWindow);
	
	
	//pedestal set
	////set value
	unsigned int pedestal;
	stringstream ss4;
	ss4 << std::hex << m_data->conf.Pedestal_channel;
	pedestal = std::stoul(ss4.str(),nullptr,16);
	////set mask
	m_data->acc->setPedestals(m_data->conf.ACDC_mask,m_data->conf.Pedestal_channel_mask,pedestal);
	
	
	//pps settings
	unsigned int ppsratio;
	stringstream ss5;
	ss5 << std::hex << m_data->conf.PPSRatio;
	ppsratio = std::stoul(ss5.str(),nullptr,16);
	m_data->acc->setPPSRatio(ppsratio);
	
	m_data->acc->setPPSBeamMultiplexer(m_data->conf.PPSBeamMultiplexer);

	if(m_data->conf.SMA == 0)
	{
		m_data->acc->setSMA_OFF()
	}else if(m_data->conf.SMA == 1)
	{
		m_data->acc->setSMA_ON()
	}
	
	int retval;
	retval = m_data->acc->initializeForDataReadout(m_data->conf.triggermode, m_data->conf.ACDC_mask, m_data->conf.Calibration_Mode);
	if(retval != 0)
	{
		m_data->psec.errorcodes.push_back(0xAA02EE01);
		ret = false;
	}else
	{
		ret = true;
		//std::cout << "Initialization successfull!" << std::endl;
	}
	
	m_data->conf.receiveFlag = 2;
	m_data->acc->emptyUsbLine();
	m_data->acc->dumpData(0xFF);
	
	vector<unsigned int> tmpERR = m_data->acc->returnErrors();
	if(tmpERR.size()==1 && tmpERR[0]==0x00000000)
	{
		m_data->psec.errorcodes.insert(std::end(m_data->psec.errorcodes), std::begin(tmpERR), std::end(tmpERR));
	}
	m_data->acc->clearErrors();
	tmpERR.clear();
	
	return ret;
}
