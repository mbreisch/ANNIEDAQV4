#include "ACC_Stream.h"

ACC_Stream::ACC_Stream():Tool(){}


bool ACC_Stream::Initialise(std::string configfile, DataModel &data){

    m_data= &data;
    m_log= m_data->Log;
    
    if(m_tool_name=="") m_tool_name="ACC_Stream";
    
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
    localconfigfile=configfile;
    if(configfile!="")  m_variables.Initialise(configfile);

    if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

    //m_variables.Print();

    std::string ip="";
    std::string port="0";

    //if(!m_variables.Get("IP",ip)) ip="127.0.0.1";
    port="89765";
    get_ok = m_variables.Get("Port",port);
    if(!get_ok) Log("ACC_Stream::Initialise got no port, falling back to default "+port,v_warning,m_verbose);
    else Log("ACC_Stream::Initialise got port "+port,v_warning,m_verbose);
    
    std::string connection="tcp://*:"+port;

    //std::cout<<"making socket from context "<<m_data->context<<std::endl;
    sock=new zmq::socket_t(*(m_data->context), ZMQ_DEALER);
    //std::cout<<"setting options of socket "<<sock<<std::endl;
    
    int zmq_linger_ms = 10;
    int zmq_sndtimeo_ms = 100;
    int zmq_rcvtimeo_ms = 100;
    int zmq_immediate = 0;
    m_variables.Get("zmq_linger_ms",zmq_linger_ms);
    m_variables.Get("zmq_sndtimeo_ms",zmq_sndtimeo_ms);
    m_variables.Get("zmq_rcvtimeo_ms",zmq_rcvtimeo_ms);
    m_variables.Get("zmq_immediate",zmq_immediate);
    sock->setsockopt(ZMQ_LINGER,zmq_linger_ms);
    sock->setsockopt(ZMQ_SNDTIMEO,zmq_sndtimeo_ms);
    sock->setsockopt(ZMQ_RCVTIMEO,zmq_rcvtimeo_ms);
    sock->setsockopt(ZMQ_IMMEDIATE,zmq_immediate);  // proobably do not want this. debug only.
    
    zmq_polltimeo_ms = 100;
    m_variables.Get("zmq_polltimeo_ms",zmq_polltimeo_ms);
    
    //std::cout<<"binding connection to "<<connection<<std::endl;
    sock->bind(connection.c_str());

    printf("ACC_Stream binding to '%s'\n",connection.c_str());
    
    // yes, items. very good variable name. very descriptive. :)
    items[0].socket = *sock;
    items[0].fd = 0;
    items[0].events = ZMQ_POLLOUT;
    items[0].revents =0;
    
    things[0].socket = *sock;
    things[0].fd = 0;
    things[0].events = ZMQ_POLLIN;
    things[0].revents = 0;

    id=0;

    //Load LAPPD_ID
    LAPPD_ID = -1;
    m_variables.Get("LAPPD_ID",LAPPD_ID);

	m_data->TCS.Buffer.insert(std::pair<int, queue<PsecData>>(LAPPD_ID,{}));
    
    return true;
}


bool ACC_Stream::Execute(){
  
  // at start of  new run, re-fetch Tool config
  if(m_data->reinit){
    Log("ACC_Stream detected reinitialise, Finalising...",v_warning,m_verbose);
    Finalise();
    Log("ACC_Stream detected reinitialise, Re-Initialising...",v_warning,m_verbose);
    Initialise(localconfigfile,*m_data);
    Log("ACC_Stream detected reinitialise, Reinitialised",v_warning,m_verbose);
  }

  // receive any outstanding acknowledgements
  Log("ACC_Stream polling for acks...",v_debug,m_verbose);
  zmq::poll(&things[0], 1, zmq_polltimeo_ms);
  // XXX note: previously if we didn't get an ack quickly enough after sending a message,
  // we'd buffer it and send it again next time. But if the ack arrives in the meantime,
  // and we don't check for received acks before we resend our old message,
  // we could get out of sync with the main DAQ and end up sending every message twice.
  // first (and always, why not) receive any acks.
  if(things[0].revents & ZMQ_POLLIN){
      printf("got akn\n");
      // got an ack
      zmq::message_t akn;
      sock->recv(&akn);
      int ackid;
      memcpy(&ackid, akn.data(), sizeof(ackid));

      printf("akn=%d\n",ackid);
      // check ack is for the current id
      if(ackid==id){
          // message acknowledged successfully
          // Remove the entry that was sent
          m_data->TCS.Buffer.at(LAPPD_ID).pop();
          // increment the id for next send
          ++id;
      } else {
          // wrong acknowledged id...?
          std::cerr<<"Mismatch between sent id ("<<id
                   <<") and acknowledged id ("<<ackid<<")"<<std::endl;
      }
  
  } // else no outstanding acks
  
  if(m_data->running){
    // to avoid a lot of code duplication (and allow more time for the ack)
    // always move the current readout to the buffer if it is valid,
    // then all we ever do is send from the buffer if we can and it has data
    if(m_data->psec.readRetval!=404){
      Log("ACC_Stream got 404...",v_debug,m_verbose);
      m_data->psec.RawWaveform = m_data->psec.ReceiveData;
      m_data->TCS.Buffer.at(LAPPD_ID).push(m_data->psec);
    }
    
    // check for outgoing listener to send new data to
    int poll_ok = zmq::poll(&items[0], 1, zmq_polltimeo_ms);
    if(poll_ok<0){
        Log("ACC_Stream zmq poll error!",0,0);
    }
    
    // else check Poll result
    else if(items[0].revents & ZMQ_POLLOUT)
    {
        
        // check if the buffer has data
        if(!m_data->TCS.Buffer.at(LAPPD_ID).empty()){
            
            // We have data waiting to be sent
            printf("sending data\n");
            
            // send our oldest buffer entry
            // first an id so that it may be acknowledged
            zmq::message_t msgid(sizeof(id));
            memcpy(msgid.data(), &id, sizeof(id));
            bool send_ok = sock->send(msgid, ZMQ_SNDMORE);
            
            if(!send_ok){
               Log("ACC_Stream error sending ID!",0,0);
            } else {
                    
                // then the entry itself
                std::cout<<"sending buffered data!!!!!!!!!!!!!!!!!  : id="<<id<<std::endl;
                send_ok = m_data->TCS.Buffer.at(LAPPD_ID).front().Send(sock);
                if(!send_ok){
                    Log("ACC_Stream error sending PsecData at front of queue!",0,0);
                }
                std::cout<<"send ok="<<send_ok<<std::endl;
                
            } // end if sent id ok
            
            // verbose print
            if(m_verbose>1){m_data->TCS.Buffer.at(LAPPD_ID).front().Print();}
            
            // we'll check for ack and pop it off on next Execute
            
        } else {
           std::cout<<"no data to send"<<std::endl;
        }
    } else {
        // else cannot send, no listener
        printf("no listener on data send socket!\n");
    }

    //At the end always clear
    Log("ACC_Stream clearing errors...",v_debug+3,m_verbose);
    m_data->psec.errorcodes.clear();
    m_data->psec.ReceiveData.clear();
    m_data->psec.BoardIndex.clear();
    m_data->psec.AccInfoFrame.clear();
    m_data->psec.RawWaveform.clear();

    std::stringstream tmp;
    tmp<<"U="<<m_data->TCS.Buffer.at(LAPPD_ID).size();
    m_data->vars.Set("Status", tmp.str());
  
  } else {
    // if not running, do not carry over old data
    Log("ACC_Stream clearing old data...",v_debug+3,m_verbose);
    if(m_data->TCS.Buffer[LAPPD_ID].size() > 0) m_data->TCS.Buffer.at(LAPPD_ID) = {};
    Log("ACC_Stream cleared",v_debug+3,m_verbose);
  }
  return true;
}


bool ACC_Stream::Finalise(){
    
    Log("ACC_Stream deleting zmq socket",v_debug,m_verbose);
    delete sock;
    sock=0;
    Log("ACC_Stream finalise done",v_debug,m_verbose);

    return true;
}
