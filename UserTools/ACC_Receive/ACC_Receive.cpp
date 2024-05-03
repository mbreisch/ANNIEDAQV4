#include "ACC_Receive.h"
#include <sstream>

ACC_Receive::ACC_Receive():Tool(){}


bool ACC_Receive::Initialise(std::string configfile, DataModel &data){
    
    m_data= &data;
    m_log= m_data->Log;
    
    if(m_tool_name=="") m_tool_name="ACC_Receive";
    
    // get tool config from database
    std::string configtext;
    try
    {
        bool get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
    }catch(std::exception& e)
    {
        std::cerr<<"ACC_Receive::Initialise caught exception on config "<<e.what()<<std::endl;
        get_ok=false;
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
    if(configfile!="")  m_variables.Initialise(configfile);
    
    // get Run[Start/End]Configs
    RunStartConfigType="local";
    RunEndConfigType="local";
    m_variables.Get("RunStartConfigType",RunStartConfigType);
    m_variables.Get("RunEndConfigType",RunEndConfigType);
    m_variables.Get("RunStartConfigName",RunStartConfigName);
    m_variables.Get("RunEndConfigName",RunEndConfigName);
    
    if(RunStartConfigType=="remote"){
    	get_ok = m_data->postgres_helper.GetToolConfig(RunStartConfigName, RunStartConfig);
    	if(!get_ok){
    		Log(m_tool_name+" Failed to get remote config entry '"+RunStartConfigName+"' from database!",0,0);
    		return false;
    	}
    }
    
    if(RunEndConfigType=="remote"){
    	get_ok = m_data->postgres_helper.GetToolConfig(RunEndConfigName, RunEndConfig);
    	if(!get_ok){
    		Log(m_tool_name+" Failed to get remote config entry '"+RunEndConfigName+"' from database!",0,0);
    		return false;
    	}
    }
    
    if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
    if(m_verbose>2) m_variables.Print();
    
    running_old=false;
    
    return true;
}


bool ACC_Receive::Execute(){
  
  // on start of run, re-fetch Tool configuration
  if(m_data->reinit){
    Finalise();
    Initialise(localconfigfile,*m_data);
  }
  
  // on start or end of run, update ACC configuration
  if(m_data->running != running_old){
  
    printf("ACC_receive initialise");
    running_old=m_data->running;
    
    Store tmp;
    if(m_data->running){
    	if(RunStartConfigType=="local"){ tmp.Initialise(RunStartConfigName); }
    	else { std::stringstream ssconfig(RunStartConfig); tmp.Initialise(ssconfig); }
    	
    } else {
    	if(RunEndConfigType=="local"){ tmp.Initialise(RunEndConfigName); }
    	else { std::stringstream ss(RunEndConfig); tmp.Initialise(ss); }
    }
    
    if(m_verbose>1){tmp.Print();}
    
    std::cout<<" did it initiailiase? = "<<m_data->conf.Initialise(&tmp)<<std::endl;
    
    if(m_verbose>1){m_data->conf.Print();}
    
    
  }
  
  return true;
}


bool ACC_Receive::Finalise(){


    return true;
}
