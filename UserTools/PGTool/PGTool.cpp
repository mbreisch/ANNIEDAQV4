#include "PGTool.h"

PGTool::PGTool():Tool(){}


bool PGTool::Initialise(std::string configfile, DataModel &data){

  m_data= &data;
  m_log= m_data->Log;

  // get tool config from database
  std::string configtext;
  std::cout<<"PGTool name is "<<m_tool_name<<std::endl;
  if(m_tool_name=="") m_tool_name="PGTool";
  bool get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
  if(!get_ok){
    Log(m_tool_name+" Failed to get Tool config from database!",v_error,verbosity);
    return false;
  }
  // parse the configuration to populate the m_variables Store.
  std::stringstream configstream(configtext);
  if(configtext!="") m_variables.Initialise(configstream);
  
  // allow overrides from local config file	
  if(configfile!="")  m_variables.Initialise(configfile);
  
  m_variables.Print();

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  return true;
}


bool PGTool::Execute(){
  
  // get current run number
  int runnum;
  std::string query_string = "SELECT max(runnum) FROM run;";
  get_ok = m_data->postgres.ExecuteQuery(query_string, runnum);
  if(get_ok){
    std::cout<<"current run is "<<runnum<<std::endl;
  } else {
    std::cerr<<"Failed to get current run from DB"<<std::endl;
    return false;
  }
  
  // get subrun number
  int subrunnum;
  query_string = "SELECT max(subrunnum) FROM run WHERE runnum="+std::to_string(runnum);
  get_ok = m_data->postgres.ExecuteQuery(query_string, subrunnum);
  if(get_ok){
    std::cout<<"current subrun is "<<subrunnum<<std::endl;
  } else {
    std::cerr<<"failed to get subrrun num from DB"<<std::endl;
    return false;
  }
  
  // get runtype -> first need to get runconfig ID
  int runconfigID;
  query_string = "SELECT runconfig FROM run WHERE runnum="+std::to_string(runnum);
  get_ok = m_data->postgres.ExecuteQuery(query_string, runconfigID);
  if(get_ok){
    std::cout<<"this run uses configuration id "<<runconfigID<<std::endl;
  } else {
    std::cerr<<"failed to get runconfig ID from DB"<<std::endl;
    return false;
  }
  
  // now look up run type (i.e. runconfig name) from the runconfig ID
  std::string runtype;
  query_string = "SELECT name FROM runconfig WHERE id="+std::to_string(runconfigID);
  get_ok = m_data->postgres.ExecuteQuery(query_string, runtype);
  if(get_ok){
    std::cout<<"corresponding run type is "<<runtype<<std::endl;
    // note that there is also a version number available, e.g. 'AmBe v2.2'
  } else {
    std::cerr<<"failed to get runtype from DB"<<std::endl;
    return false;
  }
  
  // make a new entry corresponding to a new run/subrun
  std::vector<std::string> field_names{"runnum","subrunnum","start","runconfig","notes"};
  ++subrunnum;  // let's make a new subrun entry
  std::string notes="I guess this field doesn't need to be populated for every subrun";
  std::string error_ret="";
  get_ok = m_data->postgres.Insert("run",                       // table name
                                   field_names,                 // field names
                                   &error_ret,                  // error return string
                                   // variadic argument list of field values
                                   runnum,                      // run
                                   subrunnum,                   // subrun
                                   "now()",                     // start time
                                   runconfigID,                 // runconfig
                                   notes);                      // notes
  
  if(!get_ok) std::cerr<<"Failed to insert into database with error "<<error_ret<<std::endl;
  
  // send log message - i'll upate the middleman based on your remote logging message format
  // so that you can just use the usual remote logging method
  // TODO when you get me the remote logging format
  Log("Just started a new subrun",v_message,verbosity);

  return true;
}


bool PGTool::Finalise(){

  return true;
}
