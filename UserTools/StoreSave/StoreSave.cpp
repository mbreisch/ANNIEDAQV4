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
}

StoreSave_args::~StoreSave_args(){

  delete receive;
  receive=0;

  delete outstore;
  outstore=0;


}


StoreSave::StoreSave():Tool(){}


bool StoreSave::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  // BEN set up m_trigger_values and m_previous_values;
  m_data->run=0;
  m_data->subrun=0;
  m_trigger_values["VME"]=100;
  m_previous_values["VME"]=0;
  ////////////////////////

  m_util=new DAQUtilities(m_data->context);
  args=new StoreSave_args();

  args->receive = new zmq::socket_t(*m_data->context, ZMQ_ROUTER);
  args->receive->bind("inproc://datatostoresave");
  args->identities = &(m_data->identities);
  args->OutPath="./";
  args->OutFile="RAWDATA";
  args->run=m_data->run;
  args->subrun=m_data->subrun;

  args->out<<args->OutPath<<args->OutFile<<"R"<<args->run<<"S"<<args->subrun<<"p"<<args->part;
  args->outstore=new BoostStore(false,0);

  args->m_utils=m_util;
  
  m_util->CreateThread("Store", &Thread, args);


  return true;
}


bool StoreSave::Execute(){

  // trigger readout

  for(std::map<std::string,unsigned long>::iterator it=m_data->triggers.begin(); it!= m_data->triggers.end(); it++){
   
    if(m_trigger_values.count(it->first)){
    
      if((it->second % m_trigger_values[it->first]) >= m_previous_values[it->first]) m_previous_values[it->first]=(it->second % m_trigger_values[it->first]);
      
      else{
	m_previous_values[it->first]=0;
	args->trigger=true;
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

  if(args->trigger){
    
    args->trigger=false;

    zmq::message_t msg(0);
    zmq::message_t msg2(1);

    for(int i=0; i<args->identities->size(); i++){
      
      zmq::message_t identity(args->identities->at(i).length());
      memcpy (identity.data(), args->identities->at(i).c_str(), args->identities->at(i).length());         
      args->receive->send(identity, ZMQ_SNDMORE);
      // args->receive->send(msg, ZMQ_SNDMORE);
      args->receive->send(msg);

      bool loop=true;

      while(loop){
	
	loop=false;
	zmq::message_t key_msg;

	args->receive->recv(&key_msg);
	std::istringstream key(static_cast<char*>(key_msg.data()));

	if(key=="VME") loop=true;

	  BoostStore* tmp;
	  args->m_utils->ReceivePointer(args->receive, tmp);
	  
	  if(tmp!=0) args->outstore->Set(key.str(),*tmp);
	  
      }




    }

    args->outstore->Save(args->out.str());
    args->outstore->Close();
    args->part++;
    args->out<<args->OutPath<<args->OutFile<<"R"<<args->run<<"S"<<args->subrun<<"p"<<args->part;
    delete args->outstore;
    args->outstore=0;
    args->outstore = new BoostStore(false,0);

 
  }
  else usleep(100);


}
