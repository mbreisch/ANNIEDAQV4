#include "PGStarter.h"

PGStarter::PGStarter():Tool(){}


bool PGStarter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  
  m_data= &data;
  m_log= m_data->Log;
  
  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
  
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
  
  // get type of run from command line arg 1
  std::string runtype;
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
  
  // we also need to look up the latest version num for this run type.
  // the run type and version number together uniquely identify the run configuration
  // and are needed by other Tools to get their configuration parameters
  resultstring="";
  err="";
  //query_string="SELECT version FROM runconfig INNER JOIN run ON run.runconfig = runconfig.id WHERE name='"
  //             + m_data->RunType+"' AND runnum = (SELECT max(runnum) FROM run)";
  query_string="SELECT id,version FROM runconfig WHERE name='"+m_data->RunType+"' AND version="
               "(SELECT max(version) FROM runconfig WHERE name='"+m_data->RunType+"');";
  get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
  if(not get_ok){
    Log("PGStarter failed to get runtype id and version for type "+runtype
       +" with return "+std::to_string(get_ok)+" and error "+err,v_error,verbosity);
    return false;
  }
  resultstore.JsonParser(resultstring);
  get_ok  = resultstore.Get("version",m_data->RunTypeVersion);
  get_ok &= resultstore.Get("id",m_data->RunTypeID);
  if(not get_ok){
    Log("PGStarter failed to get run config from query response '"+resultstring+"'",v_error,verbosity);
    return false;
  }
  
  // Initialise is called at the start of a new run or subrun;
  // increment either runnum or subrunnum as appropriate
  if(new_run){
    ++runnum;
    subrunnum=0;
  } else {
    ++subrunnum;
  }
  m_data->run = runnum;
  m_data->subrun = subrunnum;
  
  // create a new record in the run table for this run/subrun
  query_string = "INSERT INTO run ( runnum, subrunnum, start, runconfig ) VALUES ( "
                 +std::to_string(m_data->run)+", "+std::to_string(m_data->subrun)
                 +", 'now()', "+std::to_string(m_data->RunTypeID)+");";
std::cout<<"inserting"<<std::endl;
  get_ok = m_data->pgclient.SendQuery(dbname, query_string, &resultstring, &timeout_ms, &err);
std::cout<<"returned "<<get_ok<<std::endl;
  if(not get_ok){
    Log("PGStarter failed to make new entry with query '"+query_string+"'"
        ", returned with error '"+err+"'",v_error,verbosity);
    return false;
  }

  
  Log("This run is run number "+std::to_string(m_data->run)
     +", subrun "+std::to_string(m_data->subrun)
     +", of runtype "+m_data->RunType
     +" configuration version "+std::to_string(m_data->RunTypeVersion),
     v_message,verbosity);
  
  return true;
}


bool PGStarter::Execute(){

  return true;
}


bool PGStarter::Finalise(){
  
  m_data->pgclient.Finalise();
  
  return true;
}
