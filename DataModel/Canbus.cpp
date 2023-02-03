#include <Canbus.h>

/*ID 6: Constructor*/
Canbus::Canbus()
{}

/*ID6.1 DeConstructor*/
Canbus::~Canbus()
{
    YAPI::FreeAPI();
}

/*ID 7: Connect to CANBUS socket*/
bool Canbus::Connect()
{ 
	//Create a socket
	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
	    errorcode.push_back(0xCA07EE01);
	    return false;
	}
	
	strcpy(ifr.ifr_name, "can0");
	ioctl(s, SIOCGIFINDEX, &ifr);
	
	memset(&addr, 0, sizeof(addr));
	
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	
  	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		errorcode.push_back(0xCA07EE02);
		return false;
	}
	
	return true;
}

/*ID 8: Disconnect from CANBUS socket*/
bool Canbus::Disconnect()
{ 
	close(s);
	return true;
}

/*ID 9: Send function to send CANBUS messages*/
int Canbus::SendMessage(unsigned int id, unsigned long long msg){
	int mtu;
	int enable_canfd = 1;
	int retval; 
	
	retval = createCanFrame(id,msg,&frame);
	if(!retval)
	{
		errorcode.push_back(0xCA09EE01);
		return -11;
	}

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
		errorcode.push_back(0xCA09EE02);
		return -12;
	}
	
	strcpy(ifr.ifr_name, "can0");
	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
	if (!ifr.ifr_ifindex)
	{
		errorcode.push_back(0xCA09EE03);
		return -12;
	}
	
	memset(&addr, 0, sizeof(addr));
	
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	
	if (retval > (int)CAN_MTU)
	{
		/* check if frame fits into the CAN netdevice */
		if (ioctl(s, SIOCGIFMTU, &ifr) < 0)
		{
		  errorcode.push_back(0xCA09EE04);
		  return -21;
		}
		mtu = ifr.ifr_mtu;

		if (mtu != CANFD_MTU)
		{
		  errorcode.push_back(0xCA09EE05);
		  return -21;
		}

		/* interface is ok - try to switch the socket into CAN FD mode */
		if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)))
		{
		  errorcode.push_back(0xCA09EE06);
		  return 1;
		}

	}
	
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);	
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		errorcode.push_back(0xCA09EE07);
		return -1;
	}
	
	if (write(s, &frame, retval) != retval) {
		errorcode.push_back(0xCA09EE08);
		return -2;
	}
	
	Disconnect();
	return 0;	
}

/*ID 10: Send function to receive CANBUS messages*/
char* Canbus::ReceiveMessage(unsigned int id, unsigned long long msg){
	
	//Prepare message variables
	char rec_id[5];
	char rec_temp[3];
	char *rec_message; rec_message = (char*) malloc(256);
	
	//Connect to the socket
	if(!Connect())
	{
		errorcode.push_back(0xCA10EE01);	
		return rec_message;	
	}

	//Prepare help variables for select()
	int counter=0;
	fd_set rfds;
	struct timeval tv;
	int retval;
	
	
	//Prepare the byte counter
	nbytes=0;

	//Start a listen loop
	bool chk = false;
	while(true)
	{
		//If enough attempts at a read were made re-send the message
		if(counter==100)
		{
			if(chk==true){return rec_message;}

			//Preperation
			Disconnect();
			counter=0;
			//Send the message again
			if((retval=SendMessage(id,msg))!=0)
			{
				errorcode.push_back(0xCA10EE02);
				return rec_message;
			}
			//Reconnect for read
			if(!Connect())
			{
				errorcode.push_back(0xCA10EE03);
				return rec_message;	
			}
			chk = true;
		}
		
		//Get the select ready
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		//Wait for x [s] and y [us]
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		
		//Monitor the 's' socket, for a time tv  
		retval = select(s+1, &rfds, NULL, NULL, &tv);

		//Depending on the retval do things:
		if (retval == -1)
		{
			errorcode.push_back(0xCA10EE04);
		}else if(retval)
		{
			nbytes = read(s, &frame, sizeof(struct canfd_frame));
		}

		if(nbytes<=0)
		{
			counter++;
			continue;
		}

		sprintf(rec_id,"%03X%c",frame.can_id,'#');
		rec_id[4] = '\0';
		strcpy(rec_message,rec_id);
		unsigned int num =  frame.len;
		for (int i = 0; i < num; i++)
		{
			sprintf(rec_temp,"%02X",frame.data[i]);
			strcat(rec_message,rec_temp);
		}
		unsigned int pID = parseResponseID(rec_message);

		if(id == pID)
		{
			memset(rec_id, 0, sizeof rec_id);
			memset(rec_temp, 0, sizeof rec_temp);
			memset(rec_message, 0, sizeof rec_message);
			counter++;
			nbytes=0;
			continue;
		}else{
			break;
		}	
	}	
	Disconnect();
	
	return rec_message;
}

/*ID 15: Get the light levels from the photodiode*/
float Canbus::GetPhotodiode()
{
	float light = -1;
	unsigned int id = 0x00D;
	unsigned long long msg = 0x000D000D000D000D;
	int retval;
	
	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA15EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}
	
	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);	
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x0D0)
	{
		unsigned int lighth = ((retMSG & 0xFFFF000000000000) >> 48);
		light = lighth*CONVERSION;

		return light;
	}else
	{
		return -5;		
	}	

	return -7;
}

/*ID 19: Get the triggerboard DAC 0 value */
float Canbus::GetTriggerDac0(float VREF)
{
	int retval;
	unsigned int id = 0x0BC;
	unsigned long long msg = 0x0000000000000000;

	//send & read
	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA19EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}
	
	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);	
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x0CB)
	{
		if((retMSG>>56)==0xC0)
		{
			unsigned int value = (retMSG & 0x00FFF00000000000) >> 44;
			float result = value*VREF/4095;

			return result;
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after DAC0 check\n");
		return -5;		
	}
	return -7;
}

/*ID 20: Get the triggerboard DAC 1 value */
float Canbus::GetTriggerDac1(float VREF)
{
	int retval;
	unsigned int id = 0x0EF;
	unsigned long long msg = 0x0000000000000000;

	//send & read
	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA20EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}
	
	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);	
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x0FE)
	{
		if((retMSG>>56)==0xC0)
		{
			unsigned int value = (retMSG & 0x00FFF00000000000) >> 44;
			float result = value*VREF/4095;

			return result;
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after DAC1 check\n");
		return -5;		
	}
	return -7;
}

/*ID 21: Set the triggerboard DAC 0 value */
int Canbus::SetTriggerDac0(float threshold, float VREF)
{
	unsigned int id = 0x0AB;
	unsigned long long msg = 0x0000000000000000;
	int retval;

	if(threshold>VREF)
	{
		threshold = VREF;
	}

  	int k = 0;
  	k = (int)(threshold * 4096. / VREF);

	stringstream ss;
	ss << std::hex << (k);
	unsigned long long tmp = 0x0000000000000000;
	tmp = std::stoull(ss.str(),nullptr,16);

	msg = msg | (tmp<<52);

	//send & read
	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA21EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);

	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}

	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);	
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;	
	*/

	//Analize response
	if(retID == 0x0BA)
	{
		if((retMSG>>56)==0xC0 || (retMSG>>56)==0x40)
		{
			unsigned int value = (retMSG & 0x00FFF00000000000) >> 44;
			float result = value*VREF/4095;
			if(abs(result-threshold)<0.001)
			{
				return 0;
			}else
			{
				//std::cout << "Result was " << result << std::endl;
				return -6;
			}
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after DAC0 check\n");
		return -5;		
	}

	return -7;
}

/*ID 22: Set the triggerboard DAC 1 value */
int Canbus::SetTriggerDac1(float threshold, float VREF)
{
	unsigned int id = 0x0DE;
	unsigned long long msg = 0x0000000000000000;
	int retval;

	if(threshold>VREF)
	{
		threshold = VREF;
	}

  	int k = 0;
  	k = (int)(threshold * 4096. / VREF);

	stringstream ss;
	ss << std::hex << (k);
	unsigned long long tmp = 0x0000000000000000;
	tmp = std::stoull(ss.str(),nullptr,16);

	msg = msg | (tmp<<52);

	//send & read
	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA22EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);

	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}

	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);	
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	//Analize response
	if(retID == 0x0ED)
	{
		if((retMSG>>56)==0xC0 || (retMSG>>56)==0x40)
		{
			unsigned int value = (retMSG & 0x00FFF00000000000) >> 44;
			float result = value*VREF/4095;
			if(abs(result-threshold)<0.001)
			{
				return 0;
			}else
			{
				//std::cout << "Result was " << result << std::endl;
				return -6;
			}
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after DAC1 check\n");
		return -5;		
	}

	return -7;
}

/*ID 18: Get the temperature and humidity values*/
vector<float> Canbus::GetTemp()
{ 	
	vector<float> RHT = {-1,-1};
	unsigned int id = 0x123;
	unsigned long long msg = 0x0000000000000000;

	int retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA18EE01);
		return {(float)retval,(float)retval};	
	}

	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}

	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/ 

	unsigned long long t_temp;
	unsigned int temp_hex;
	unsigned long long t_hum;
	unsigned int hum_hex;

	if(retID == 0x321)
	{
		t_temp = retMSG & 0x000000FFFC000000;
		temp_hex = t_temp >> 26;
		t_hum = retMSG &  0x003FFF0000000000;
		hum_hex = t_hum >> 40;

		float T = (temp_hex/(pow(2,14)-2))*165-40;
		float H = (hum_hex/(pow(2,14)-2))*100;

		RHT[0] = (T);
		RHT[1] = (H);

		//cout << "H=" << H << " T=" << T << endl; 
		return RHT;
	}else
	{
		//fprintf(stderr, "No response from LVHV after RHT check\n");
		return {-6,-6};	
	}

	return {-7,-7};
}

/*ID 16: Set the HV state*/
int Canbus::SetHV_ONOFF(bool state){ 
	unsigned int id;
	unsigned long long msg;
	int retval;
	
	if(state==true){
		id = 0x040;
		msg = 0xBEEFDEAD00000000;
		retval = 1;
	}else{
		id = 0x030;
		msg = 0x00000000BEEFDEAD;
		retval = 0;
	}

	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA16EE01);
		return retval;	
	}
	
	usleep(TIMEOUT_RS);

	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}

	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x041)
	{	
		if(retMSG == 0x0001000100010001)
		{
			return 1;
		}else
		{
			//fprintf(stderr, "Response doesn't make sense!\n");
			return -5;	
		}
	}else if(retID == 0x031)
	{
		if(retMSG == 0x0000000000000000)
		{
			return 0;
		}else
		{
			//fprintf(stderr, "Response doesn't make sense!\n");
			return -5;	
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after HV set check\n");
		return -6;	
	}

	return retval;
}

/*ID 17: Set the HV voltage value*/
int Canbus::SetHV_voltage(float volts_user_input, float current_voltage, int verbosity){ 
	int retval=0;
	float sign;
	
	float v_ramp, v_diff; //for prep
	float vset, vpct, dac_vout; //for calc
	
	unsigned int id;
	unsigned long long msg;
	int k;
	unsigned long long tmp;
	
	if(volts_user_input > HV_MAX || volts_user_input<0)
	{
		volts_user_input = 0;
		errorcode.push_back(0xCA17EE01);
		//std::cout << "Max voltage set" << std::endl;
	}

	//take volts from input and 
	if (volts_user_input > current_voltage)
	{
		sign = 1.0;
	}else
	{
	    sign = -1.0;
	}
	//Set temporary voltage to the current voltage and get the difference
	v_ramp = current_voltage;
	v_diff = fabs(volts_user_input-current_voltage); 
    
    	//While ramp value is not set value
	while(v_ramp != volts_user_input)
	{
		if(v_diff < 50) // check if close to final voltage
		{
			v_ramp += sign*v_diff;
		}
		else // increment by DV volts
		{
			v_ramp += sign*DV;
		}
		v_diff = fabs(volts_user_input - v_ramp); 

        	//Prepare the message
		id = 0x050;
		msg = 0x0000000000000000;
	
		vset = v_ramp;
		vpct = vset / C40N_MAX;
		//printf("  fraction of max HV output (4kV) = %f\n", vpct);
		dac_vout = vpct * DAC_VMAX;
		//printf("  DAC output voltage = %f\n", dac_vout);

		// convert into DAC input code
		k = 0;
		k = (int)(pow(2,12) * dac_vout / DAC_VREF);
		//printf("DEBUG:  k = %d,  k = %X (hex),  k << 3 = %X (hex)\n", k, k, (k<<3));
		stringstream ss;
		ss << std::hex << (k<<3);
		tmp = 0x0000000000000000;
		tmp = std::stoull(ss.str(),nullptr,16);
		msg = msg | (tmp<<48);

		int retval = SendMessage(id,msg);
		if(retval!=0)
		{
			errorcode.push_back(0xCA17EE02);
			return retval;	
		}
		usleep(5000000);
		
		if(verbosity>1)
		{
		    std::cout << "Voltage set to " <<  v_ramp << "|" << volts_user_input << " V" << std::endl;
		}
	}
	return retval;
}

/*ID 25: Get the HV state*/
int Canbus::GetHV_ONOFF(){ 
	unsigned int id = 0x034;
	unsigned long long msg = 0x0000BEEFDEAD0000;
	int HV_state=-1;
	
	int retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA25EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}	


	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x035)
	{	
		unsigned int state = (retMSG & 0x0001000000000000) >> 48;
		if(state==0)
		{
			return 0;
		}else if(state==1)
		{
		    	unsigned long long HVval = (retMSG & 0xffff);
		    	unsigned long long HVval2 = (retMSG & 0xffff000000) >> 24;
			if(HVval==HVval2)
			{
				ReturnedHvValue = (double)HVval*CONVERSION*1000;
				return 1;
			}else
			{
				return -5;
			}
		}else
		{
			//fprintf(stderr, "Response doesn't make sense!\n");
			return -5;	
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after HV check\n");
		return -6;	
	}

	return HV_state;
}

/*ID 12: Set the LV state*/
int Canbus::SetLV(bool state){ 
	unsigned int id;
	unsigned long long msg;
	int retval=-1;
	
	if(state==true){
		id = 0x020;
		msg = 0xDEADBEEF00000000;
		retval = 1;
	}else{
		id = 0x010;
		msg = 0x00000000DEADBEEF;
		retval = 0;
	}

	retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA12EE01);
		return retval;	
	}
	
	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}
	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x021)
	{	
		if(retMSG == 0x0001000100010001)
		{
			return 1;
		}else
		{
			//fprintf(stderr, "Response doesn't make sense! (LV set 021#)\n");
			return -5;	
		}
	}else if(retID == 0x011)
	{
		if(retMSG == 0x0000000000000000)
		{
			return 0;
		}else
		{
			//fprintf(stderr, "Response doesn't make sense! (LV set 011#)\n");
			return -5;	
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after LV set check\n");
		return -6;	
	}
	
	return retval;
}

/*ID 13: Get the low voltage values*/
int Canbus::GetLV_ONOFF(){ 
	unsigned int id = 0x022;
	unsigned long long msg = 0x0000DEADBEEF0000;
	int LV_state=-1;
	
	int retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA13EE01);
		return retval;	
	}

	usleep(TIMEOUT_RS);

	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}	

	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x220)
	{	
		if(retMSG == 0x0000000000000000)
		{
			return 0;
		}else if(retMSG == 0x0001000100010001)
		{
			return 1;
		}else
		{
			//fprintf(stderr, "Response doesn't make sense! (LV get 220#)\n");
			return -5;	
		}
	}else
	{
		//fprintf(stderr, "No response from LVHV after LV check\n");
		return -6;
	}

	return LV_state;
}

/*ID 14: Get the low voltage values*/
vector<float> Canbus::GetLV_voltage(){ 
	unsigned int id = 0x3AD;
	unsigned long long msg = 0x00AD00AD00AD00AD;
	std::vector<float> volts ={-1,-1,-1};
	float v33, v25, v12;
	
	int retval = SendMessage(id,msg);
	if(retval!=0)
	{
		errorcode.push_back(0xCA14EE01);
		return {(float)retval,(float)retval,(float)retval};	
	}

	usleep(TIMEOUT_RS);
	
	char* rec_message;
	rec_message = ReceiveMessage(id,msg);
	if(strlen(rec_message)<=0)
	{
		free(rec_message);
		rec_message = ReceiveMessage(id,msg);
	}		

	//back parse message to state
	unsigned int retID = parseResponseID(rec_message);
	unsigned long long retMSG = parseResponseMSG(rec_message);
	free(rec_message);
	
	/*
	std::cout << "--------------- Control Window----------------" << std::endl;
	printf("%s\n", rec_message);
	printf("ID: 0x%03x\n", retID);
	printf("MSG: 0x%0llx\n", retMSG);
	std::cout << "----------------------------------------------" << std::endl;
	*/

	if(retID == 0x3DA)
	{	
		unsigned int v33h = ((retMSG & 0xFFFF000000000000) >> 48);
		unsigned int v25h = ((retMSG & 0x000000FFFF000000) >> 24);
		unsigned int v12h = (retMSG & 0x000000000000FFFF);

		v33 = v33h*CONVERSION;
		if(v33>=0)
		{
			volts[0] = (v33);
		}
		v25 = v25h*CONVERSION;
		if(v25>=0)
		{
			volts[1] = (v25);
		}
		v12 = v12h*CONVERSION;
		if(v12>=0)
		{
			volts[2] = (v12);
		}
		return volts;
	}else
	{
		//fprintf(stderr, "No response from LVHV after LV voltage check\n");
		return {-6,-6,-6};
	}

	return volts;
}

/*ID 23: Set the relay state*/
int Canbus::SetRelay(int idx, bool state){
	wiringPiSetup();
	pinMode(RLY1, OUTPUT);
	pinMode(RLY2, OUTPUT);
	pinMode(RLY3, OUTPUT);
	int ch;
	int status=-4;
	int stateInt;
	
	if(state==true)
	{
		stateInt = 1;
	}else{
		stateInt = 0;
	}

	switch(idx)
	{
		case 1:
			ch = RLY1;
			break;
		case 2:
			ch = RLY2;
			break;
		case 3:
			ch = RLY3;
			break;
		default :
			errorcode.push_back(0xCA23EE01);
			return -2;
	}
	
	digitalWrite(ch, stateInt);
	usleep(10000);
	if((digitalRead(ch)) == stateInt)
	{		
		return stateInt;
	}else 
	{
		errorcode.push_back(0xCA23EE02);
		return -3;
	}
	
	return status;
}

/*ID 24: Get the relay state*/
bool Canbus::GetRelayState(int idx){
	wiringPiSetup();
	pinMode(RLY1, OUTPUT);
	pinMode(RLY2, OUTPUT);
	pinMode(RLY3, OUTPUT);
	int ch;
	int retval = -3;

	switch(idx){
		case 1:
			ch = RLY1;
			break;
		case 2:
			ch = RLY2;
			break;
		case 3:
			ch = RLY3;
			break;
		default :
			errorcode.push_back(0xCA24EE01);
			return retval;
	}
	retval = digitalRead(ch);
	if(retval==0)
	{
		return false;
	}else if(retval==1)
	{
		return true;
	}
	return retval;
}

/*ID 25: Get the Saltbridge state*/
float Canbus::GetSaltbridge()
{
	//init
	string errmsg, target, serial;
	YTemperature *tsensor=0;
	YTemperature *t1=0;
	float Resistance=0.0;
	
	//std::cout << "Trying to connect to USB" << std::endl;
	
	// Setup the API to use local USB devices
    try
    {
        if (YAPI::RegisterHub("usb", errmsg) != YAPI::SUCCESS) 
        {
            //cerr << "RegisterHub error: " << errmsg << endl;
            errorcode.push_back(0xCA25EE01);
            Resistance = -111;
        }
 
        //std::cout << "Thermistor ID is"  << thermistor_id << std::endl;
        //Get target device and sensor
        target = thermistorID;
        
        tsensor = YTemperature::FindTemperature(target + ".temperature4");
        serial = tsensor->get_module()->get_serialNumber();
        
        //tsensor = YGenericSensor::FindGenericSensor(target + ".genericsensor1");
        //serial = tsensor->get_module()->get_serialNumber();	
        
        //cout << "serial " << serial << endl;
        
        t1 = YTemperature::FindTemperature(serial + ".temperature4");

        if(t1->isOnline()){
            Resistance = t1->get_signalValue();
            //cout << "R for saltbridge is " << Resistance << endl;
            string Unit = t1->get_signalUnit();
            //cout << "Unit for saltbridge is " << Unit << endl;
        }else
        {
            errorcode.push_back(0xCA25EE03);
            Resistance = -333;
        } 
    }
    catch(...)
    {
        errorcode.push_back(0xCA25EE02);
        Resistance = -222;
    }

	return Resistance;
}

/*ID 26: Get the thermistor value*/
float Canbus::GetThermistor()
{
	//init
	string errmsg, target, serial;
	YTemperature *tsensor=0;
	YTemperature *t1=0;
	float Temperature=0.0;
	
	//std::cout << "Trying to connect to USB" << std::endl;
	
	// Setup the API to use local USB devices
    try
    {
        if (YAPI::RegisterHub("usb", errmsg) != YAPI::SUCCESS) 
        {
            //cerr << "RegisterHub error: " << errmsg << endl;
            errorcode.push_back(0xCA26EE01);
            Temperature = -111;
        }
 
        //std::cout << "Thermistor ID is"  << thermistor_id << std::endl;
        //Get target device and sensor
        target = thermistorID;
        
        tsensor = YTemperature::FindTemperature(target + ".temperature1");
        serial = tsensor->get_module()->get_serialNumber();
        
        //tsensor = YGenericSensor::FindGenericSensor(target + ".genericsensor1");
        //serial = tsensor->get_module()->get_serialNumber();	
        
        //cout << "serial " << serial << endl;
        
    	t1 = YTemperature::FindTemperature(serial + ".temperature1");

        if(t1->isOnline()){
            Temperature = t1->get_signalValue();
            //cout << "R for thermistor is " << Temperature << endl;
            string Unit = t1->get_signalUnit();
            //cout << "Unit for thermistor is " << Unit << endl;
        }else
        {
            errorcode.push_back(0xCA26EE03);
            Temperature = -333;
        }
    }
    catch(...)
    {
        errorcode.push_back(0xCA26EE02);
        Temperature = -222;
    }

	return Temperature;
}
