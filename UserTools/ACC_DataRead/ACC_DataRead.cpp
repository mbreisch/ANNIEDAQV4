#include "ACC_DataRead.h"

ACC_DataRead::ACC_DataRead():Tool(){}


bool ACC_DataRead::Initialise(std::string configfile, DataModel &data){
	
	m_data= &data;
	m_log= m_data->Log;
	
	if(m_tool_name=="") m_tool_name="ACC_DataRead";
	
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
	if(configfile!="") m_variables.Initialise(configfile);
	
	//m_variables.Print();
	
	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
	
	return true;
}


bool ACC_DataRead::Execute(){
	
	if(m_data->running){
		
		//  printf("running\n");
		
		if(m_data->conf.triggermode==1){
			//printf("trigger mode 1\n");
			m_data->acc->softwareTrigger();
		}
		
		m_data->psec.readRetval = m_data->acc->listenForAcdcData(m_data->conf.triggermode);
		
		if(m_data->psec.readRetval != 0){
			
			// printf("bad data Retval=%d\n",m_data->psec.readRetval);
			
			if(m_data->psec.readRetval != 404){
				//printf("404\n");
				m_data->psec.FailedReadCounter = m_data->psec.FailedReadCounter + 1;}
			m_data->psec.ReceiveData.clear();
			m_data->acc->clearData();
			
		} else {
			
			printf("good data\n");
			
			m_data->psec.AccInfoFrame = m_data->acc->returnACCIF();
			m_data->psec.ReceiveData = m_data->acc->returnRaw();
			m_data->psec.BoardIndex = m_data->acc->returnBoardIndices();
			m_data->acc->clearData();
			
		}
		
		//Get Timestamp
		unsigned long long timeSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		m_data->psec.Timestamp = to_string(timeSinceEpoch-UTCCONV);
		
		//Get errors
		vector<unsigned int> tmpERR = m_data->acc->returnErrors();
		m_data->psec.errorcodes.insert(std::end(m_data->psec.errorcodes), std::begin(tmpERR), std::end(tmpERR));
		m_data->acc->clearErrors();
		tmpERR.clear();
		
	}
	
	return true;
}


bool ACC_DataRead::Finalise(){
	delete m_data->acc;
	m_data->acc=0;
	return true;
}
