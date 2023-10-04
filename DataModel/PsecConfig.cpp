#include <PsecConfig.h>

PsecConfig::PsecConfig()
{
    VersionNumber = 0x0004;
    LAPPD_ID = 0;
    receiveFlag = 1;
    SetDefaults();
}

PsecConfig::PsecConfig(unsigned int id)
{
    VersionNumber = 0x0004;
    LAPPD_ID = id;
    receiveFlag = 1;
    SetDefaults();
}

PsecConfig::~PsecConfig()
{}

bool PsecConfig::Send(zmq::socket_t* sock)
{
    zmq::message_t msg0(sizeof VersionNumber);
    memcpy(msg0.data(), &VersionNumber, sizeof VersionNumber);

    zmq::message_t msgID(sizeof LAPPD_ID);
    memcpy(msgID.data(), &LAPPD_ID, sizeof LAPPD_ID);

    zmq::message_t msg1(sizeof receiveFlag);
    memcpy(msg1.data(), &receiveFlag, sizeof receiveFlag);

    zmq::message_t msgRC(sizeof RunControl);
    memcpy(msgRC.data(), &RunControl, sizeof RunControl);

    zmq::message_t msg2(sizeof triggermode);
    memcpy(msg2.data(), &triggermode, sizeof triggermode);

    zmq::message_t msg3(sizeof ACC_Sign);
    memcpy(msg3.data(), &ACC_Sign, sizeof ACC_Sign);

    zmq::message_t msg4(sizeof ACDC_Sign);
    memcpy(msg4.data(), &ACDC_Sign, sizeof ACDC_Sign);      

    zmq::message_t msg5(sizeof SELF_Sign);
    memcpy(msg5.data(), &SELF_Sign, sizeof SELF_Sign);

    zmq::message_t msg6(sizeof SELF_Enable_Coincidence);
    memcpy(msg6.data(), &SELF_Enable_Coincidence, sizeof SELF_Enable_Coincidence);

    zmq::message_t msg7(sizeof SELF_Coincidence_Number);
    memcpy(msg7.data(), &SELF_Coincidence_Number, sizeof SELF_Coincidence_Number);

    zmq::message_t msg8(sizeof SELF_threshold);
    memcpy(msg8.data(), &SELF_threshold, sizeof SELF_threshold);

    zmq::message_t msg9(sizeof ACDC_mask);
    memcpy(msg9.data(), &ACDC_mask, sizeof ACDC_mask); 

    zmq::message_t msg10(sizeof PSEC_Chip_Mask_0);
    memcpy(msg10.data(), &PSEC_Chip_Mask_0, sizeof PSEC_Chip_Mask_0);

    zmq::message_t msg11(sizeof PSEC_Chip_Mask_1);
    memcpy(msg11.data(), &PSEC_Chip_Mask_1, sizeof PSEC_Chip_Mask_1);

    zmq::message_t msg12(sizeof PSEC_Chip_Mask_2);
    memcpy(msg12.data(), &PSEC_Chip_Mask_2, sizeof PSEC_Chip_Mask_2);

    zmq::message_t msg13(sizeof PSEC_Chip_Mask_3);
    memcpy(msg13.data(), &PSEC_Chip_Mask_3, sizeof PSEC_Chip_Mask_3);

    zmq::message_t msg14(sizeof PSEC_Chip_Mask_4);
    memcpy(msg14.data(), &PSEC_Chip_Mask_4, sizeof PSEC_Chip_Mask_4);

    zmq::message_t msg15(sizeof PSEC_Channel_Mask_0);
    memcpy(msg15.data(), &PSEC_Channel_Mask_0, sizeof PSEC_Channel_Mask_0);      

    zmq::message_t msg16(sizeof PSEC_Channel_Mask_1);
    memcpy(msg16.data(), &PSEC_Channel_Mask_1, sizeof PSEC_Channel_Mask_1);

    zmq::message_t msg17(sizeof PSEC_Channel_Mask_2);
    memcpy(msg17.data(), &PSEC_Channel_Mask_2, sizeof PSEC_Channel_Mask_2);

    zmq::message_t msg18(sizeof PSEC_Channel_Mask_3);
    memcpy(msg18.data(), &PSEC_Channel_Mask_3, sizeof PSEC_Channel_Mask_3);

    zmq::message_t msg19(sizeof PSEC_Channel_Mask_4);
    memcpy(msg19.data(), &PSEC_Channel_Mask_4, sizeof PSEC_Channel_Mask_4);

    zmq::message_t msg20(sizeof Validation_Start);
    memcpy(msg20.data(), &Validation_Start, sizeof Validation_Start);

    zmq::message_t msg21(sizeof Validation_Window);
    memcpy(msg21.data(), &Validation_Window, sizeof Validation_Window);

    zmq::message_t msg22(sizeof Calibration_Mode);
    memcpy(msg22.data(), &Calibration_Mode, sizeof Calibration_Mode); 

    zmq::message_t msg23(sizeof Raw_Mode);
    memcpy(msg23.data(), &Raw_Mode, sizeof Raw_Mode);

    zmq::message_t msg24(sizeof Pedestal_channel);
    memcpy(msg24.data(), &Pedestal_channel, sizeof Pedestal_channel);

    zmq::message_t msg25(sizeof Pedestal_channel_mask);
    memcpy(msg25.data(), &Pedestal_channel_mask, sizeof Pedestal_channel_mask); 

    zmq::message_t msg26(sizeof PPSRatio);
    memcpy(msg26.data(), &PPSRatio, sizeof PPSRatio); 

    zmq::message_t msg27(sizeof PPSBeamMultiplexer);
    memcpy(msg27.data(), &PPSBeamMultiplexer, sizeof PPSBeamMultiplexer); 

    zmq::message_t msg28(sizeof ResetSwitchACC);
    memcpy(msg28.data(), &ResetSwitchACC, sizeof ResetSwitchACC);

    zmq::message_t msg29(sizeof ResetSwitchACDC);
    memcpy(msg29.data(), &ResetSwitchACDC, sizeof ResetSwitchACDC);	

    zmq::message_t msg30(sizeof SMA_Beamgate);
    memcpy(msg30.data(), &SMA_Beamgate, sizeof SMA_Beamgate);

    zmq::message_t msg31(sizeof SMA_PPS);
    memcpy(msg31.data(), &SMA_PPS, sizeof SMA_PPS);

    sock->send(msg0,ZMQ_SNDMORE);
    sock->send(msgID,ZMQ_SNDMORE);
    sock->send(msg1,ZMQ_SNDMORE);
    sock->send(msgRC,ZMQ_SNDMORE);
    sock->send(msg2,ZMQ_SNDMORE);
    sock->send(msg3,ZMQ_SNDMORE);
    sock->send(msg4,ZMQ_SNDMORE);
    sock->send(msg5,ZMQ_SNDMORE);
    sock->send(msg6,ZMQ_SNDMORE);
    sock->send(msg7,ZMQ_SNDMORE);
    sock->send(msg8,ZMQ_SNDMORE);
    sock->send(msg9,ZMQ_SNDMORE);
    sock->send(msg10,ZMQ_SNDMORE);
    sock->send(msg11,ZMQ_SNDMORE);
    sock->send(msg12,ZMQ_SNDMORE);
    sock->send(msg13,ZMQ_SNDMORE);
    sock->send(msg14,ZMQ_SNDMORE);
    sock->send(msg15,ZMQ_SNDMORE);
    sock->send(msg16,ZMQ_SNDMORE);
    sock->send(msg17,ZMQ_SNDMORE);
    sock->send(msg18,ZMQ_SNDMORE);
    sock->send(msg19,ZMQ_SNDMORE);
    sock->send(msg20,ZMQ_SNDMORE);
    sock->send(msg21,ZMQ_SNDMORE);
    sock->send(msg22,ZMQ_SNDMORE);
    sock->send(msg23,ZMQ_SNDMORE);
    sock->send(msg24,ZMQ_SNDMORE);
    sock->send(msg25,ZMQ_SNDMORE);
    sock->send(msg26,ZMQ_SNDMORE);
    sock->send(msg27,ZMQ_SNDMORE);
    sock->send(msg28,ZMQ_SNDMORE);
    sock->send(msg29,ZMQ_SNDMORE);
    sock->send(msg30,ZMQ_SNDMORE);
    sock->send(msg31);

    return true;
}

bool PsecConfig::Receive(zmq::socket_t* sock)
{
    zmq::message_t msg;
    int tmp_size;

    //VersionNumber
    sock->recv(&msg);
    unsigned int TVersion;
    TVersion=*(reinterpret_cast<unsigned int*>(msg.data())); 
    if(TVersion!=VersionNumber){return false;}
	
    sock->recv(&msg);
    LAPPD_ID=*(reinterpret_cast<unsigned int*>(msg.data())); 

    //flag
    sock->recv(&msg);
    receiveFlag=*(reinterpret_cast<int*>(msg.data())); 

    //RunControl
    sock->recv(&msg);
    RunControl=*(reinterpret_cast<int*>(msg.data())); 

    //trigger
    sock->recv(&msg);
    triggermode=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    ACC_Sign=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    ACDC_Sign=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    SELF_Sign=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    SELF_Enable_Coincidence=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    SELF_Coincidence_Number=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    SELF_threshold=*(reinterpret_cast<int*>(msg.data()));

    //ACDC boards
    sock->recv(&msg);
    ACDC_mask=*(reinterpret_cast<unsigned int*>(msg.data()));

    //PSEC chips for self trigger
    sock->recv(&msg);
    PSEC_Chip_Mask_0=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Chip_Mask_1=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Chip_Mask_2=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Chip_Mask_3=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Chip_Mask_4=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Channel_Mask_0=*(reinterpret_cast<unsigned int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Channel_Mask_1=*(reinterpret_cast<unsigned int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Channel_Mask_2=*(reinterpret_cast<unsigned int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Channel_Mask_3=*(reinterpret_cast<unsigned int*>(msg.data()));
    sock->recv(&msg);
    PSEC_Channel_Mask_4=*(reinterpret_cast<unsigned int*>(msg.data()));

    //Validation time
    sock->recv(&msg);
    Validation_Start=*(reinterpret_cast<float*>(msg.data()));
    sock->recv(&msg);
    Validation_Window=*(reinterpret_cast<float*>(msg.data()));

    //Calibration mode
    sock->recv(&msg);
    Calibration_Mode=*(reinterpret_cast<int*>(msg.data()));

    //Raw mode
    sock->recv(&msg);
    Raw_Mode=*(reinterpret_cast<bool*>(msg.data()));

    //Pedestal set value channel
    sock->recv(&msg);
    Pedestal_channel=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    Pedestal_channel_mask=*(reinterpret_cast<unsigned int*>(msg.data()));

    //PPS settings
    sock->recv(&msg);
    PPSRatio=*(reinterpret_cast<unsigned int*>(msg.data()));
    sock->recv(&msg);
    PPSBeamMultiplexer=*(reinterpret_cast<int*>(msg.data()));	

    //Reset switch
    sock->recv(&msg);
    ResetSwitchACC =*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    ResetSwitchACDC=*(reinterpret_cast<int*>(msg.data()));	
	
    //SMA
    sock->recv(&msg);
    SMA_Beamgate=*(reinterpret_cast<int*>(msg.data()));
    sock->recv(&msg);
    SMA_PPS=*(reinterpret_cast<int*>(msg.data()));

    return true;
}

bool PsecConfig::Initialise(Store* store)
{
    unsigned int TVersion=0;
    store->Get("TVersion",TVersion);
    //if(TVersion!=VersionNumber){return false;}

    store->Get("LAPPD_ID",LAPPD_ID);

    //flag
    store->Get("receiveFlag",receiveFlag);

    //RunControl
    store->Get("RunControl",RunControl);

    //SMA 
    store->Get("SMA",SMA_Beamgate);
    store->Get("SMA",SMA_PPS);
 
    //trigger
    store->Get("triggermode",triggermode);
    store->Get("ACC_Sign",ACC_Sign);
    store->Get("ACDC_Sign", ACDC_Sign);
    store->Get("SELF_Sign",SELF_Sign);
    store->Get("SELF_Enable_Coincidence",SELF_Enable_Coincidence);
    store->Get("SELF_Coincidence_Number",SELF_Coincidence_Number);
    store->Get("SELF_threshold",SELF_threshold);

    //ACDC boards
    store->Get("ACDC_mask", ACDC_mask);
   
    //PSEC chips for self trigger
    store->Get("PSEC_Chip_Mask_0",PSEC_Chip_Mask_0);
    store->Get("PSEC_Chip_Mask_1",PSEC_Chip_Mask_1);
    store->Get("PSEC_Chip_Mask_2",PSEC_Chip_Mask_2);
    store->Get("PSEC_Chip_Mask_3",PSEC_Chip_Mask_3);
    store->Get("PSEC_Chip_Mask_4",PSEC_Chip_Mask_4);

    store->Get("PSEC_Channel_Mask_0",PSEC_Channel_Mask_0);
    store->Get("PSEC_Channel_Mask_1",PSEC_Channel_Mask_1);
    store->Get("PSEC_Channel_Mask_2",PSEC_Channel_Mask_2);
    store->Get("PSEC_Channel_Mask_3",PSEC_Channel_Mask_3);
    store->Get("PSEC_Channel_Mask_4",PSEC_Channel_Mask_4);
    
    //Validation time
    store->Get("Validation_Start",Validation_Start);
    store->Get("Validation_Window",Validation_Window);

    //Calibration mode
    store->Get("Calibration_Mode",Calibration_Mode);

    //Raw mode
    store->Get("Raw_Mode",Raw_Mode);

    //Pedestal set value channel
    store->Get("Pedestal_channel",Pedestal_channel);
    store->Get("Pedestal_channel_mask",Pedestal_channel_mask);

    //PPS settings
    store->Get("PPSRatio",PPSRatio);
    store->Get("PPSBeamMultiplexer",PPSBeamMultiplexer);

    //Reset switch
    store->Get("ResetSwitchACC", ResetSwitchACC);
    store->Get("ResetSwitchACDC",ResetSwitchACDC);
	
    return true;
}


bool PsecConfig::SetDefaults()
{
    //RunControl
    RunControl = -1;

    //trigger
    triggermode = 5;

    //SMA
    SMA_Beamgate = 0;
    SMA_PPS = 0;

    //triggersettings
    ACC_Sign = 0;
    ACDC_Sign = 0;
    SELF_Sign = 0;
    SELF_Enable_Coincidence = 0;
    SELF_Coincidence_Number = 0;
    SELF_threshold = 1600;

    //ACDC boards
    ACDC_mask = 0xFF;

    //PSEC chips for self trigger
    PSEC_Chip_Mask_0 = 1;
    PSEC_Chip_Mask_1 = 0;
    PSEC_Chip_Mask_2 = 0;
    PSEC_Chip_Mask_3 = 0;
    PSEC_Chip_Mask_4 = 0;
    PSEC_Channel_Mask_0 = 0x20;
    PSEC_Channel_Mask_1 = 0x00;
    PSEC_Channel_Mask_2 = 0x00;
    PSEC_Channel_Mask_3 = 0x00;
    PSEC_Channel_Mask_4 = 0x00;

    //Validation time
    Validation_Start = 0;
    Validation_Window = 0;

    //Calibration mode
    Calibration_Mode = 0;

    //Raw mode
    Raw_Mode = true;

    //Pedestal set value channel
    Pedestal_channel = 2000;
    Pedestal_channel_mask = 0x1F;

    //PPS settings
    PPSRatio = 0x0000;
    PPSBeamMultiplexer = 1;

    ResetSwitchACC = 0;
    ResetSwitchACDC = 0;

    return true;
}

bool PsecConfig::Print(){
    std::cout << "------------------General settings------------------" << std::endl;
    printf("Receive flag: %i\n", receiveFlag);
    printf("ACDC boardmask: 0x%02x\n",ACDC_mask);
    printf("Calibration Mode: %i\n",Calibration_Mode);
    printf("SMA Beamgate: %i\n",SMA_Beamgate);
    printf("SMA PPS: %i\n",SMA_PPS);
    std::cout << "------------------Trigger settings------------------" << std::endl;
    printf("Triggermode: %i\n",triggermode);
    printf("ACC trigger Sign: %i\n", ACC_Sign);
    printf("ACDC trigger Sign: %i\n", ACDC_Sign);
    printf("Selftrigger Sign: %i\n", SELF_Sign);
    printf("Coincidence Mode: %i\n", SELF_Enable_Coincidence);
    printf("Required Coincidence Channels: %d\n", SELF_Coincidence_Number);
    printf("Selftrigger threshold: %d\n", SELF_threshold);
    printf("Validation trigger start: %f ns\n", Validation_Start);
    printf("Validation trigger window: %f ns\n", Validation_Window);
    std::cout << "------------------PSEC settings------------------" << std::endl;
    printf("PSEC chipmask (chip 0 to 4) : %i|%i|%i|%i|%i\n",PSEC_Chip_Mask_0,PSEC_Chip_Mask_1,PSEC_Chip_Mask_2,PSEC_Chip_Mask_3,PSEC_Chip_Mask_4);
    printf("PSEC channelmask (for chip 0 to 4) : 0x%02x|0x%02x|0x%02x|0x%02x|0x%02x\n",PSEC_Channel_Mask_0,PSEC_Channel_Mask_1,PSEC_Channel_Mask_2,PSEC_Channel_Mask_3,PSEC_Channel_Mask_4);
    printf("PSEC pedestal value: %d\n", Pedestal_channel);
    printf("PSEC chipmask for pedestal: 0x%02x\n", Pedestal_channel_mask);
    std::cout << "------------------PSEC settings------------------" << std::endl;	
    printf("PPS divider ratio: 0x%04x\n",PPSRatio);
    printf("PPS multiplexer: %i\n",PPSBeamMultiplexer);
    std::cout << "-------------------------------------------------" << std::endl;

    return true;
}
