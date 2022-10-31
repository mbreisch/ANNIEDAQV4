#include "PGStarter.h"

PGStarter::PGStarter():Tool(){}


bool PGStarter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  
  m_data= &data;
  m_log= m_data->Log;
  
  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
  
  // if we're the main DAQ we will make a new entry in the run table
  // at the start of each run. But only the main DAQ toolchain should do this.
  std::string systemname="";
  daqtoolchain=false;
  if(!m_data->vars.Get("SystemName",systemname)){
    Log("PGStarter did not find a SystemName in ToolChainConfig!",v_error,verbosity);
    return false;
  } else {
    daqtoolchain = (systemname=="daq");
  }
  
  // get config file of settings for connecting to the database
  std::string pgsettingsfile="";
  get_ok = m_variables.Get("pgsettingsfile",pgsettingsfile);
  if(not get_ok){
    Log("PGStarter couldn't find pgsettingsfile in m_variables! Need to know postgres connection settings!",
        v_warning,verbosity);
    // i guess we can continue with defaults of everything
  }
  
  get_ok = m_data->pgclient.Initialise(pgsettingsfile);
  if(not get_ok){
    Log("PGClient failed to Initialise!",v_error,verbosity);
    return false;
  }
  
  // after Initilising the pgclient needs ~15 seconds for the middleman to connect
  std::this_thread::sleep_for(std::chrono::seconds(15));
  // hopefully the middleman has found us by now
  
  // for now until the PGHelper does it for us we need to extract the results from
  // the returned json using a Store
  Store resultstore;
  
  // make a query to get the run number
  // TODO update the PGHelper to work with the new PGClient to make these cleaner
  std::string resultstring="";
  std::string err="";
  std::string dbname="rundb";
  std::string query_string="SELECT max(runnum) AS run FROM run;";
  int timeout_ms=5000;
  get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
  if(not get_ok){
    Log("PGStarter failed to get run number with return "+std::to_string(get_ok)
       +" and error "+err,v_error,verbosity);
    return false;
  }
  int runnum=0;
  resultstore.JsonParser(resultstring);
  get_ok = resultstore.Get("run",runnum);
  if(not get_ok){
    Log("PGStarter failed to get runnum from query response '"+resultstring+"'",v_error,verbosity);
    return false;
  }
  
  bool new_run=true;
  // see if the max run num is the same as the run already in the datamodel
  if(runnum>0 && m_data->run == runnum){
    // we've already initialised the datamodel run number - so if we're
    // calling Initialise again, it's for a new subrun
    new_run=false;
  };
  
  // make a query to get the subrrun for this runnum
  resultstring="";
  err="";
  query_string="SELECT max(subrunnum) AS subrun FROM run WHERE runnum="+std::to_string(runnum)+";";
  get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
  if(not get_ok){
    Log("PGStarter failed to get subrun number with return "+std::to_string(get_ok)
       +" and error "+err,v_error,verbosity);
    return false;
  }
  int subrunnum=0;
  resultstore.JsonParser(resultstring);
  get_ok = resultstore.Get("subrun",subrunnum);
  if(not get_ok){
    Log("PGStarter failed to get subrunnum from query response '"+resultstring+"'",v_error,verbosity);
    return false;
  }
  
  // if it's a new run we need to look up the run configuration.
  // if we're the main DAQ we do this from the command line args
  if(new_run && daqtoolchain){
    
    // get type of run from command line arg 1
    std::string runtype="";
    m_data->vars.Get("$1",runtype);
    // we need to extract the runtype from the command line argument.
    // this may be something like 'Beam', but may also be './Beam'
    // or './configfiles/Beam/ToolChainConfig', so strip as appropriate
    
    // first strip everything before the first slash
    size_t apos = runtype.find('/');
    if(apos!=std::string::npos){
    	runtype=runtype.substr(apos+1,std::string::npos);
    }
    // next strip any trailing '/ToolChainConfig' if present
    apos=runtype.find("/ToolChainConfig");
    if(apos!=std::string::npos){
    	runtype=runtype.substr(0,apos);
    }
    // and finally strip any leading 'configfiles/'
    apos=runtype.find("configfiles/");
    
    m_data->RunType=runtype;
    
    // we then need to look up the runconfig ID of the latest version of this run type.
    // the run config ID uniquely identifies the run configuration,
    // and is needed by other Tools to get their configuration parameters
    resultstring="";
    err="";
    //query_string="SELECT version FROM runconfig INNER JOIN run ON run.runconfig = runconfig.id WHERE name='"
    //             + m_data->RunType+"' AND runnum = (SELECT max(runnum) FROM run)";
    query_string="SELECT id FROM runconfig WHERE name='"+m_data->RunType+"' AND version="
                 "(SELECT max(version) FROM runconfig WHERE name='"+m_data->RunType+"');";
    get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
    if(not get_ok){
      Log("PGStarter failed to get runtype id and version for type "+runtype
         +" with return "+std::to_string(get_ok)+" and error "+err,v_error,verbosity);
      return false;
    }
    resultstore.JsonParser(resultstring);
    int runconfig;
    get_ok = resultstore.Get("id",runconfig);
    if(not get_ok){
      Log("PGStarter failed to get run config from query response '"+resultstring+"'",v_error,verbosity);
      return false;
    }
    m_data->RunConfig=runconfig;
    
    // we make a new entry in the run table at the start of each new run or subrun;
    // increment either runnum or subrunnum as appropriate
    if(new_run){
      ++runnum;
      subrunnum=0;
    } else {
      ++subrunnum;
    }
    m_data->run = runnum;
    m_data->subrun = subrunnum;
    
    // and create a new record in the run table for this run/subrun
    query_string = "INSERT INTO run ( runnum, subrunnum, start, runconfig ) VALUES ( "
                   +std::to_string(m_data->run)+", "+std::to_string(m_data->subrun)
                   +", 'now()', "+std::to_string(m_data->RunConfig)+");";
    get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
    if(not get_ok){
      Log("PGStarter failed to make new entry with query '"+query_string+"'"
          ", returned with error '"+err+"'",v_error,verbosity);
      return false;
    }
    
    
  } else if(new_run){
    
    // if we are not the main toolchain we need to get the run configuration id
    // corresponding to the current run (i.e. latest run entry)
    query_string = "SELECT runconfig FROM run WHERE runnum=(SELECT MAX(runnum) FROM run)";
    get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
    if(not get_ok){
      Log("PGStarter failed to get runconfig number with return "+std::to_string(get_ok)
         +" and error "+err,v_error,verbosity);
      return false;
    }
    int runconfig;
    resultstore.JsonParser(resultstring);
    get_ok = resultstore.Get("runconfig",runconfig);
    if(not get_ok){
      Log("PGStarter failed to get runconfig from query response '"+resultstring+"'",v_error,verbosity);
      return false;
    }
    
    // next we use the runconfig to get the system configuration id from the runconfig entry
    query_string = "SELECT "+systemname+" FROM runconfig WHERE id="+std::to_string(runconfig);
    get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
    if(not get_ok){
      Log("PGStarter failed to get "+systemname+" config ID with return "+std::to_string(get_ok)
         +" and error "+err,v_error,verbosity);
      return false;
    }
    resultstore.JsonParser(resultstring);
    int systemconfigid=0;
    get_ok = resultstore.Get(systemname,systemconfigid);
    if(not get_ok){
      Log("PGStarter failed to get '"+systemname+"' from query response '"+resultstring+"'",v_error,verbosity);
      return false;
    }
    
    // finally we get the toolsconfig for this system
    query_string = "SELECT toolsconfig FROM "+systemname+" WHERE id="+std::to_string(systemconfigid);
    get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
    if(not get_ok){
      Log("PGStarter failed to get toolsconfig with return "+std::to_string(get_ok)
         +" and error "+err,v_error,verbosity);
      return false;
    }
    resultstore.JsonParser(resultstring);
    std::string toolsconfigstring;
    get_ok = resultstore.Get("toolsconfig",toolsconfigstring);
    if(not get_ok){
      Log("PGStarter failed to get toolsconfig from query response '"+resultstring+"'",v_error,verbosity);
      return false;
    }
    
    // the toolsconfig itself is a json representing a map of Tool to config file version number.
    // a combination of system name, tool name and version number uniquely identifies a configuration string
    // TODO MARCUS: PGHelper for now just assumes we want the latest version, in which case all we need
    // is the system name (from m_data->vars) and tool name (hard-coded in each tool)
    // so all this was pretty pointless: but in the future PGHelper should use the toolsconfig string
    // to get the version number.
    
  }
  
  
  Log("This run is run number "+std::to_string(m_data->run)
     +", subrun "+std::to_string(m_data->subrun),
     v_message,verbosity);
  
  return true;
}


bool PGStarter::Execute(){

  return true;
}


bool PGStarter::Finalise(){

  if(daqtoolchain){
	  // update the record in the run table for this run/subrun with the number of events taken
	  std::string dbname="rundb";
	  std::string resultstring;
	  std::string err;
	  int timeout_ms=5000;
	  std::string query_string = "UPDATE run SET stop='now()', numevents="+std::to_string(m_data->NumEvents)
	                            +" WHERE runnum="+std::to_string(m_data->run)
	                            +" AND subrunnum="+std::to_string(m_data->subrun)+";";
	  get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
	  if(not get_ok){
	    Log("PGStarter failed to update end of run info with query '"+query_string+"'"
	        ", returned with error '"+err+"'",v_error,verbosity);
	    return false;
	  }
  }
  
  m_data->pgclient.Finalise();
  
  return true;
}
