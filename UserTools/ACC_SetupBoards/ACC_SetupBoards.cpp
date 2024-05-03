#include "ACC_SetupBoards.h"

ACC_SetupBoards::ACC_SetupBoards():Tool(){}


bool ACC_SetupBoards::Initialise(std::string configfile, DataModel &data){
	
	m_data= &data;
	m_log= m_data->Log;
	
	if(m_tool_name=="") m_tool_name="ACC_SetupBoards";
	
	// get tool config from database
	std::string configtext;
    bool get_ok=false;
    try
    {
        get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
    }catch(std::exception& e)
    {
        std::cerr<<"ACC_SetupBoards::Initialise caught exception on config "<<e.what()<<std::endl;
    }
	if(!get_ok){
		Log(m_tool_name+" Failed to get Tool config from database!",0,0);
		//return false;
	}
	// parse the configuration to populate the m_variables Store.
	std::stringstream configstream(configtext);
	if(configtext!="") m_variables.Initialise(configstream);
	
	// allow overrides from local config file
	localconfigfile=configfile;   // note for reinit
	if(configfile!="")  m_variables.Initialise(configfile);
	
	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
	
	//m_variables.Print();
	
	//system("mkdir -p Results");
	
    if(!m_variables.Get("Interface_Name",m_data->TCS.Interface_Name)) m_data->TCS.Interface_Name="USB";
    if(!m_variables.Get("Interface_IP",m_data->TCS.Interface_IP)) m_data->TCS.Interface_IP="127.0.0.1";
    if(!m_variables.Get("Interface_Port",m_data->TCS.Interface_Port)) m_data->TCS.Interface_Port="8.8.8.8";
	if(m_data->acc==nullptr)
    {
        if(strcmp(m_data->TCS.Interface_Name.c_str(),"USB") == 0)
        {
            m_data->acc = new ACC_USB();
        }else if(strcmp(m_data->TCS.Interface_Name.c_str(),"ETH") == 0)
        {
            m_data->acc = new ACC_ETH(m_data->TCS.Interface_IP, m_data->TCS.Interface_Port);
        }else
        {
            std::cout << "Invalid Interface_Name" << std::endl;
        }
    }
	
	
	if(!m_variables.Get("TimeoutResetCount",TimeoutResetCount))TimeoutResetCount = 300;
	if(!m_variables.Get("PPSWaitMultiplier",PPSWaitMultiplier))PPSWaitMultiplier = 10;

	return true;
}


bool ACC_SetupBoards::Execute()
{
	// at start of run, re-fetch Tool config
	if(m_data->reinit){
		Finalise();
		Initialise(localconfigfile,*m_data);
	}

	bool StartReset = false;
	for(std::map<int, int>::iterator it=m_data->TCS.Timeoutcounter.begin(); it!=m_data->TCS.Timeoutcounter.end(); ++it)
	{
        	if(it->second>TimeoutResetCount)
        	{
			std::cout << "Timeout of " << it->first << " with " << it->second << " against " << TimeoutResetCount << std::endl;
			StartReset = true;
		}
	}

	if(StartReset==true)
	{
        //Re-init the Setup part uf the tool
        m_data->conf.receiveFlag = 1; 

        //Print debug frame as overwrite
        vector<unsigned short> PrintFrame = m_data->acc->ReturnACCIF();
        std::fstream outfile("./configfiles/LAPPD/ACCIF.txt", std::ios_base::out | std::ios_base::trunc);
        for(int j=0; j<PrintFrame.size(); j++)
        {
            outfile << std::hex << PrintFrame[j] << std::endl; 
        }
        outfile << std::dec;
        outfile.close();
        PrintFrame.clear();

		for(std::map<int, int>::iterator it=m_data->TCS.Timeoutcounter.begin(); it!=m_data->TCS.Timeoutcounter.end(); ++it)
		{
			m_data->TCS.Timeoutcounter.at(it->first) = 0; //Reset the timeout counter
		}
		if(m_data->conf.ResetSwitchACC == 1)
		{
			m_data->acc->ResetACC();
		}
		if(m_data->conf.ResetSwitchACDC == 1)
		{
			m_data->acc->ResetACDC();
		}
    }

	bool setupret = false;
	if(m_data->conf.receiveFlag==1)
	{
		if(m_data->conf.RunControl==0 || m_data->conf.RunControl==1)
		{
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

			m_data->acc->ResetACC();
			m_data->acc->ResetACDC();
		}
		setupret = Setup();
		return setupret;
	}

	return true;
}


bool ACC_SetupBoards::Finalise()
{
	delete m_data->acc;
	m_data->acc = nullptr;
	return true;
}


bool ACC_SetupBoards::Setup()
{
	bool ret=false;
	
	//Set timeout value
	int timeout;
	m_variables.Get("Timeout",timeout);
	m_data->acc->SetTimeoutInMs(timeout);
	
	//polarity
	m_data->acc->SetSign(m_data->conf.ACC_Sign, 2);
	m_data->acc->SetSign(m_data->conf.ACDC_Sign, 3);
	m_data->acc->SetSign(m_data->conf.SELF_Sign, 4);
	
	//self trigger options
	m_data->acc->SetEnableCoin(m_data->conf.SELF_Enable_Coincidence);
	
	unsigned int coinNum;
	stringstream ss;
	ss << std::hex << m_data->conf.SELF_Coincidence_Number;
	coinNum = std::stoul(ss.str(),nullptr,16);
	m_data->acc->SetNumChCoin(coinNum);
	
	unsigned int threshold;
	stringstream ss2;
	ss2 << std::hex << m_data->conf.SELF_threshold;
	threshold = std::stoul(ss2.str(),nullptr,16);
	m_data->acc->SetThreshold(threshold);
	
	//psec masks combine
	std::vector<int> PsecChipMask = {m_data->conf.PSEC_Chip_Mask_0,m_data->conf.PSEC_Chip_Mask_1,m_data->conf.PSEC_Chip_Mask_2,m_data->conf.PSEC_Chip_Mask_3,m_data->conf.PSEC_Chip_Mask_4};
	std::vector<unsigned int> VecPsecChannelMask = {m_data->conf.PSEC_Channel_Mask_0,m_data->conf.PSEC_Channel_Mask_1,m_data->conf.PSEC_Channel_Mask_2,m_data->conf.PSEC_Channel_Mask_3,m_data->conf.PSEC_Channel_Mask_4};
	m_data->acc->SetPsecChipMask(PsecChipMask);
	m_data->acc->SetPsecChannelMask(VecPsecChannelMask);
	
	//validation window
	unsigned int validationStart;
	stringstream ss31;
	ss31 << std::hex << (int)m_data->conf.Validation_Start/25;
	validationStart = std::stoul(ss31.str(),nullptr,16);
	m_data->acc->SetValidationStart(validationStart);
	
	unsigned int validationWindow;
	stringstream ss32;
	ss32 << std::hex << (int)m_data->conf.Validation_Window/25;
	validationWindow = std::stoul(ss32.str(),nullptr,16);
	m_data->acc->SetValidationWindow(validationWindow);
	
	
	//pedestal set
	////set value
	unsigned int pedestal;
	stringstream ss4;
	ss4 << std::hex << m_data->conf.Pedestal_channel;
	pedestal = std::stoul(ss4.str(),nullptr,16);
	////set mask
	m_data->acc->SetPedestals(m_data->conf.ACDC_mask,m_data->conf.Pedestal_channel_mask,pedestal);
	
	
	//pps settings
	unsigned int ppsratio;
	stringstream ss5;
	ss5 << std::hex << m_data->conf.PPSRatio;
	ppsratio = std::stoul(ss5.str(),nullptr,16);
	m_data->acc->SetPPSRatio(ppsratio);
	
	//SetMaxTimeoutValue
	//TimeoutResetCount = (PPSWaitMultiplier*m_data->conf.PPSRatio)/(m_data->TCS.Timeoutcounter.size()*(timeout/1000.0));
	std::cout << "Created new timeout value based on " << m_data->conf.PPSRatio << " with " << TimeoutResetCount << std::endl;

	m_data->acc->SetPPSBeamMultiplexer(m_data->conf.PPSBeamMultiplexer);

	m_data->acc->SetSMA_Debug(m_data->conf.SMA_PPS,m_data->conf.SMA_Beamgate);

	int retval;
	retval = m_data->acc->InitializeForDataReadout(m_data->conf.ACDC_mask, m_data->conf.triggermode);
	if(retval != 0)
	{
		m_data->psec.errorcodes.push_back(0xAA02EE01);
		ret = false;
	}else
	{
		ret = true;
		std::cout << "Initialization successfull!" << std::endl;
	}
	
	m_data->conf.receiveFlag = 2;
    if(strcmp(m_data->TCS.Interface_Name.c_str(),"USB") == 0){m_data->acc->EmptyUsbLine();}
	m_data->acc->DumpData(0xFF);
	
	vector<unsigned int> tmpERR = m_data->acc->ReturnErrors();
	if(tmpERR.size()==1 && tmpERR[0]==0x00000000)
	{
		m_data->psec.errorcodes.insert(std::end(m_data->psec.errorcodes), std::begin(tmpERR), std::end(tmpERR));
	}
	m_data->acc->ClearErrors();
	tmpERR.clear();
	
	return ret;
}
