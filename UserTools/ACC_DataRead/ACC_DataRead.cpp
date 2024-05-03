#include "ACC_DataRead.h"

ACC_DataRead::ACC_DataRead():Tool(){}


bool ACC_DataRead::Initialise(std::string configfile, DataModel &data){
	
	m_data= &data;
	m_log= m_data->Log;
	
	if(m_tool_name=="") m_tool_name="ACC_DataRead";
	
	// get tool config from database
	std::string configtext;
    bool get_ok=false;
    try
    {
        get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
    }catch(std::exception& e)
    {
        std::cerr<<"ACC_DataRead::Initialise caught exception on config "<<e.what()<<std::endl;
    }
	if(!get_ok){
		Log(m_tool_name+" Failed to get Tool config from database!",0,0);
		//return false;
	}
	
	// parse the configuration to populate the m_variables Store.
	std::stringstream configstream(configtext);
	if(configtext!="") m_variables.Initialise(configstream);
	
	// allow overrides from local config file
	localconfigfile=configfile;
	if(configfile!="") m_variables.Initialise(configfile);
	
	//m_variables.Print();
	
	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

	//Load LAPPD_ID!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	LAPPD_ID = -1;
	m_variables.Get("LAPPD_ID",LAPPD_ID);
	m_variables.Get("Port_0",Port_0);
	m_variables.Get("Port_1",Port_1);
	LAPPD_on_ACC = {Port_0,Port_1};

	PreviousBuffer.insert(pair<int, unsigned short>(Port_0, 0));
	PreviousBuffer.insert(pair<int, unsigned short>(Port_1, 0));

	m_data->TCS.Timeoutcounter.insert(pair<int, int>(LAPPD_ID, 0));
	
	return true;
}

bool ACC_DataRead::Execute()
{
    // if start of new run, re-fetch tool config
    if(m_data->reinit)
    {
        Finalise();
        Initialise(localconfigfile,*m_data);
    }

    if(!m_data->running)
    {
        try
        {
            //  printf("running\n");
            m_data->psec.LAPPD_ID = LAPPD_ID;
            if(m_data->conf.triggermode==1)
            {
                printf("trigger mode 1\n");
                m_data->acc->GenerateSoftwareTrigger();
            }

            m_data->psec.readRetval = m_data->acc->ListenForAcdcData(m_data->conf.triggermode,LAPPD_on_ACC);
        }catch(std::exception& e)
        {
            std::cerr<<"ACC_DataRead::Execute caught exception on read "<<e.what()<<std::endl;
        }

        try
        {
            if(m_data->psec.readRetval != 0)
            {
                // printf("bad data Retval=%d\n",m_data->psec.readRetval);
                if(m_data->psec.readRetval != 404)
                {
                    if(m_data->psec.readRetval == 405)
                    {
                        m_data->vars.Set("StopLoop",1);
                    }
                    printf("not 404 but %i\n",m_data->psec.readRetval);
                    m_data->TCS.Timeoutcounter.at(LAPPD_ID) = 0;
                    m_data->psec.FailedReadCounter = m_data->psec.FailedReadCounter + 1;
                    m_data->psec.errorcodes.push_back(0xAD02EE01);
                    unsigned int dumpIndex = 0x00 | (1<<LAPPD_on_ACC[0]) | (1<<LAPPD_on_ACC[1]);
                    m_data->acc->DumpData(dumpIndex);
                    if(strcmp(m_data->TCS.Interface_Name.c_str(),"USB") == 0){m_data->acc->EmptyUsbLine();}
                }else
                {
                    printf("404\n");
					m_data->TCS.Timeoutcounter.at(LAPPD_ID) = m_data->TCS.Timeoutcounter.at(LAPPD_ID) + 1;
                }
                m_data->psec.ReceiveData.clear();
                m_data->acc->ClearData();
            }else
            {
                printf("good data\n");
                m_data->TCS.Timeoutcounter.at(LAPPD_ID) = 0;
                m_data->psec.AccInfoFrame = m_data->acc->ReturnACCIF(); printf("L-ACC: %d\n",m_data->psec.AccInfoFrame);
                m_data->psec.ReceiveData = m_data->acc->ReturnRawData(); printf("D-ACC: %d\n",m_data->psec.ReceiveData);
                m_data->psec.BoardIndex = m_data->acc->ReturnBoardIndices(); printf("B-ACC: %d\n",m_data->psec.BoardIndex);
                m_data->acc->ClearData();
            }                
        }catch(std::exception& e)
        {
            std::cerr<<"ACC_DataRead::Execute caught exception on read handling "<<e.what()<<std::endl;
            return false;
        }      
                        
        //Get Timestamp
        unsigned long long timeSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        m_data->psec.Timestamp = to_string(timeSinceEpoch-UTCCONV);

        try
        {
            //Get errors
            vector<unsigned int> tmpERR = m_data->acc->ReturnErrors();
            if(tmpERR.size()!=0)
            {
                m_data->psec.errorcodes.insert(std::end(m_data->psec.errorcodes), std::begin(tmpERR), std::end(tmpERR));
            }
            m_data->acc->ClearErrors();
            tmpERR.clear();
        }catch(std::exception& e)
        {
            std::cerr<<"ACC_DataRead::Execute caught exception errorcode grab "<<e.what()<<std::endl;
            return false;
        }
    }else
	{
		// printf("Not running\n");
	}

    return true;
}


bool ACC_DataRead::Finalise()
{
    return true;
}
