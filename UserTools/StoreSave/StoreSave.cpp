#include "StoreSave.h"

StoreSave_args::StoreSave_args():Thread_args(){

  receive=0;
  outstore=0;
  OutPath="";
  OutFile="";
  run=0;
  subrun=0;
  part=0;

  trigger=false;

  m_utils=0;

  m_logger=0;
}

StoreSave_args::~StoreSave_args(){

  delete receive;
  receive=0;

  delete outstore;
  outstore=0;

  m_logger=0;

}


StoreSave::StoreSave():Tool(){}


bool StoreSave::Initialise(std::string configfile, DataModel &data){

  m_data= &data;
  m_log= m_data->Log;

  if(m_tool_name=="") m_tool_name="StoreSave";
  
  // get tool config from database
  std::string configtext;
  bool get_ok = m_data->postgres_helper.GetToolConfig(m_tool_name, configtext);
  if(!get_ok){
    Log(m_tool_name+" Failed to get Tool config from database!",0,0);
    return false;
  }
  // parse the configuration to populate the m_variables Store.
  std::stringstream configstream(configtext);
  if(configtext!="") m_variables.Initialise(configstream);
  
  // allow overrides from local config file
  if(configfile!="")  m_variables.Initialise(configfile);

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  //m_variables.Print();

  // BEN set up m_trigger_values and m_previous_values;
  //  m_data->run=0;
  //m_data->subrun=0;
  m_trigger_values["VME"]=10000;
  m_variables.Get("VME_triggers_threshold",m_trigger_values["VME"]);
  m_previous_values["VME"]=0;
  m_trigger_values["CAMAC"]=10000;
  m_variables.Get("CAMAC_triggers_threshold",m_trigger_values["CAMAC"]);
  m_previous_values["CAMAC"]=0;
  m_trigger_values["LAPPD"]=10000;
  m_variables.Get("LAPPD_triggers_threshold",m_trigger_values["LAPPD"]);
  m_previous_values["LAPPD"]=0;
  ////////////////////////

  m_util=new DAQUtilities(m_data->context);
  args=new StoreSave_args();

  args->receive = new zmq::socket_t(*m_data->context, ZMQ_ROUTER);
  args->receive->bind("inproc://datatostoresave");
  args->identities = &(m_data->identities);
  args->OutPath="./";
  args->OutFile="RAWDATA";
  m_variables.Get("OutPath",args->OutPath);
  m_variables.Get("OutFile",args->OutFile);
  args->run=m_data->run;
  args->subrun=m_data->subrun;

  args->out<<args->OutPath<<args->OutFile<<"R"<<args->run<<"S"<<args->subrun<<"p"<<args->part;
  args->outstore=new BoostStore(false,0);

  args->m_utils=m_util;
  args->m_logger=m_log;

  m_util->CreateThread("Store", &Thread, args);


  return true;
}


bool StoreSave::Execute(){

  // trigger readout

  for(std::map<std::string,unsigned long>::iterator it=m_data->triggers.begin(); it!= m_data->triggers.end(); it++){
   
    if(m_trigger_values.count(it->first)){
    
      if((it->second % m_trigger_values[it->first]) >= m_previous_values[it->first]) m_previous_values[it->first]=(it->second % m_trigger_values[it->first]);
      
      else{
	std::cout<<"triggered readout "<<it->first<<std::endl;
	m_previous_values[it->first]=0;
	std::cout<<"trigger before "<< args->trigger<<std::endl;
	args->mtx.lock();
	args->trigger=true;
	args->mtx.unlock();
	std::cout<<"trigger after "<< args->trigger<<std::endl;
      }
      
    }
  
  }

  return true;
}


bool StoreSave::Finalise(){

  args->trigger=true;

  while(args->trigger) usleep(100);
  m_util->KillThread(args);

  delete args;
  args=0;

  delete m_util;
  m_util=0;

  m_data->identities.clear();

  return true;
}

void StoreSave::Thread(Thread_args* arg){

  StoreSave_args* args=reinterpret_cast<StoreSave_args*>(arg);
  // std::cout<<"d-1"<<std::endl;
  args->mtx.lock();
  if(args->trigger) printf("d0\n");
    // std::cout<<"d0 Trig="<<args->trigger<<std::endl;

  if(args->trigger){
    printf("d1\n");
    args->trigger=false;

    zmq::message_t msg(0);
    zmq::message_t msg2(1);
    printf("d2\n");
    for(int i=0; i<args->identities->size(); i++){
      printf("d3 %s : i=%d\n", args->identities->at(i).c_str(), i);  
      zmq::message_t identity(args->identities->at(i).length());
      memcpy (identity.data(), args->identities->at(i).c_str(), args->identities->at(i).length());         
      args->receive->send(identity, ZMQ_SNDMORE);
      // args->receive->send(msg, ZMQ_SNDMORE);
      args->receive->send(msg);
      printf("d4\n");
      bool loop=true;


      while(loop){
	printf("d5\n");
	loop=false;

	zmq::message_t ident;
	args->receive->recv(&ident);

	zmq::message_t key_msg;

	args->receive->recv(&key_msg);
	std::istringstream key(static_cast<char*>(key_msg.data()));
	printf("d6 %s\n", key.str().c_str());
	if(key.str()=="VME") loop=true;
	printf("d7\n");
	BoostStore* tmp=0;
	  args->m_utils->ReceivePointer(args->receive, tmp);
	  printf("d8\n");

	  if(tmp!=0){
	    args->outstore->Set(key.str(), tmp);
	    printf("d9\n");	
	  }
	  printf("d10\n");
      }
      printf("d11\n");



    }
    printf("d12 %s\n",  args->out.str().c_str());
    args->outstore->Save(args->out.str());
    printf("d13\n");
    args->outstore->Close();
    printf("d14\n");
    args->part++;
    printf("d15\n");
    args->out.str("");
    args->out<<args->OutPath<<args->OutFile<<"R"<<args->run<<"S"<<args->subrun<<"p"<<args->part;
    printf("d16\n"); 
    delete args->outstore;
    printf("d17\n");   
    args->outstore=0;
    printf("d18\n"); 
    args->outstore = new BoostStore(false,0);
    printf("d19\n"); 
    args->mtx.unlock();
    printf("d20\n");
  }
  else{
    args->mtx.unlock();
    // usleep(100);
  }
}
