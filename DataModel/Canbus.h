#ifndef CANBUS_H
#define CANBUS_H

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <sstream>
#include <vector>
#include <iomanip>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <wiringPi.h>

#include "yocto_api.h"
#include "yocto_temperature.h"
#include "yocto_genericsensor.h"


#define CANID_DELIM '#'
#define CAN_DLC 8
#define DELIM "#"
#define DATA_SEPERATOR '.'
#define C40N_MAX 4000.0 // V
#define HV_MAX 2360.0
#define DAC_VMAX 5.0
#define DAC_VREF 2.048
#define DV 50.0 // V
#define RLY1 25
#define RLY2 28
#define RLY3 29
#define TIMEOUT_RS 100000
#define CONVERSION 0.00518

using namespace std;

class Canbus{

public:

	/*ID 6: Constructor*/
	Canbus();
	/*ID 6.5: DeConstructor*/
	~Canbus();

	/*-------------------Comm functions-----------------*/
	/*ID 7: Connect to CANBUS socket*/
	bool Connect(); 
	/*ID 8: Disconnect from CANBUS socket*/
	bool Disconnect();
	/*ID 9: Send function to send CANBUS messages*/
	int SendMessage(unsigned int id, unsigned long long msg);
	/*ID 10: Send function to receive CANBUS messages*/
	char* ReceiveMessage(unsigned int id, unsigned long long msg);
	/*ID 11: Function to return all errors*/
	void clearErrors(){errorcode.clear();}
	vector<unsigned int> returnErrors()
	{
		if(errorcode.size()==0)
		{
			errorcode.push_back(returncode);
		}
		return errorcode;
	}

	/*---------------------LV functions-----------------*/
	/*ID 12: Set the LV state*/
	int SetLV(bool state);
	/*ID 13: Get the LV state*/
	int GetLV_ONOFF(); 
	/*ID 14: Get the low voltage values*/
	vector<float> GetLV_voltage(); 

	/*----------------Photodiode functions--------------*/
	/*ID 15: Get the light levels from the photodiode*/
	float GetPhotodiode(); 
	
	/*---------------------HV functions-----------------*/
	/*ID 16: Set the HV state*/
	int SetHV_ONOFF(bool state); 
	/*ID 17: Set the HV voltage value*/
	int SetHV_voltage(float volts_user_input, float current_voltage, int verbosity); 
	/*ID 25: Get the HV state*/
	int GetHV_ONOFF(); 

	/*--------Temperature and humidity functions--------*/
	/*ID 18: Get the temperature and humidity values*/
	vector<float> GetTemp(); 
	/*ID 26: Get the temperature from the thermistor*/
	float GetThermistor(); 

	/*--------------Triggerboard functions--------------*/
	/*ID 19: Get the triggerboard DAC 0 value */
	float GetTriggerDac0(float TrigVref);
	/*ID 20: Get the triggerboard DAC 1 value */
	float GetTriggerDac1(float TrigVref);
	/*ID 21: Set the triggerboard DAC 0 value */
	int SetTriggerDac0(float threshold, float TrigVref);
	/*ID 22: Set the triggerboard DAC 1 value */
	int SetTriggerDac1(float threshold, float TrigVref);
	
	/*------------------Relay functions-----------------*/
	/*ID 23: Set the relay state*/
	int SetRelay(int idx, bool state);
	/*ID 24: Get the relay state*/
	bool GetRelayState(int idx);
	
	/*----------------Saltbridge functions---------------*/
	/*ID 25: Get the saltbridge state*/
	float GetSaltbridge();

	/*-----------------Global variables-----------------*/
	void SetThermistorID(string in){thermistorID = in;}
	string thermistorID;
	int s;
	int nbytes;
	int required_mtu;
	struct ifreq ifr;
	struct sockaddr_can addr;
	struct canfd_frame frame;
	float get_HV_volts = 0;
	float ReturnedHvValue;
	unsigned int returncode = 0x00000000;
	vector<unsigned int> errorcode;

	/*--------------------------------------------------*/
	/*ID 1: ascii bin help function*/
	unsigned char asc2nib(char c){
		if((c>='0') && (c<='9'))
		{
		  return c - '0';
		}
		if((c>='A') && (c <='F'))
		{
		  return c - 'A' + 10;
		}
		if((c>='a') && (c<='f'))
		{
		  return c - 'a' + 10;
		}
		return 16;
	}

	/*ID 2: Takes a message id and body and creates a CANBUS specific message*/
	char* parseFrame(unsigned int id, unsigned long long msg)
	{
		char* r_frame = (char*)malloc(128);

		stringstream ss;
		ss << std::setfill('0') << std::setw(3) << std::hex << id;

		stringstream ss2;
		ss2 << std::setfill('0') << std::setw(16) << std::hex << msg;

		strcpy(r_frame,ss.str().c_str());
		strcat(r_frame,DELIM);
		strcat(r_frame,ss2.str().c_str());

		return r_frame;
	}

	/*ID 3: Main function that creates CANBUS messages*/
	int createCanFrame(unsigned int id, unsigned long long msg, struct canfd_frame *cf)
	{
		char *t_frame;// = (char *)malloc(128);
		t_frame = parseFrame(id,msg);

		int i, idx, dlc, len;
		int maxdlen = CAN_MAX_DLEN;
  		int ret = CAN_MTU;
		unsigned char temp;

		len = strlen(t_frame);
		memset(cf, 0, sizeof(*cf));

		if (len<4)
		{
			errorcode.push_back(0xCA03EE01);
			free(t_frame);
			return 0;
		}
		if (t_frame[3] == CANID_DELIM)
		{
			idx = 4;
			for (i=0; i<3; i++)
			{
		  		if((temp = asc2nib(t_frame[i])) > 0x0F)
		  		{
		  			errorcode.push_back(0xCA03EE02);
					free(t_frame);
		    			return 0;
		  		}
		  		cf->can_id |= (temp << (2-i)*4);
			}
		}else if (t_frame[8] == CANID_DELIM)
		{
			idx = 9;
			for (i=0; i<8; i++)
			{
		  		if((temp = asc2nib(t_frame[i])) > 0x0F)
		  		{
		  			errorcode.push_back(0xCA03EE03);
					free(t_frame);
			    		return 0;
				}
		  		cf->can_id |= (temp << (7-i)*4);
			}
			if(!(cf->can_id & CAN_ERR_FLAG))
			{
		  		cf->can_id |= CAN_EFF_FLAG;
			}
		} else 
		{
			errorcode.push_back(0xCA03EE04);
			free(t_frame);
			return 0;
		}

		if ((t_frame[idx] == 'R') || (t_frame[idx] == 'r')) // RTR frame
		{
			cf->can_id |= CAN_RTR_FLAG;

			/* check for optional DLC value for CAN 2.0B frames */
			if (t_frame[++idx] && (temp = asc2nib(t_frame[idx])) <= CAN_MAX_DLC)
			cf->len = temp;
			free(t_frame);
			return ret;
		}
		
		if (t_frame[idx] == CANID_DELIM) // CAN FD frame escape char '##'
		{
			maxdlen = CANFD_MAX_DLEN;
			ret = CANFD_MTU;

			/* CAN FD frame <canid>##<flags><data> */
			if ((temp = asc2nib(t_frame[idx+1])) > 0x0F)
			{	
				free(t_frame);
				return 0;
			}
			cf->flags = temp;
			idx += 2;
		}
		
		for (i=0, dlc=0; i<maxdlen; i++)
		{
			if(t_frame[idx] == DATA_SEPERATOR)
			{
		  		idx++;
			}
			if(idx >= len)
			{
		  		break;
			}
			if((temp = asc2nib(t_frame[idx++])) > 0x0F)
			{
				errorcode.push_back(0xCA03EE05);
				free(t_frame);
		  		return 0;
			}
			cf->data[i] = (temp << 4);
			if((temp = asc2nib(t_frame[idx++])) > 0x0F)
			{
				errorcode.push_back(0xCA03EE06);
				free(t_frame);
		  		return 0;
			}
			cf->data[i] |= temp;
			dlc++;
		}
		cf->len = dlc;

		free(t_frame);
		return ret;
	}

	/*ID 4: Returns an id from a CANBUS message*/
	unsigned int parseResponseID(char* response)
	{
	  unsigned int retID;

	  char ch_id[4];
	  for(int i=0; i<3; i++)
	  {
	    ch_id[i] = response[i];
	  }
	  ch_id[3] = '\0';
	  retID = strtoul(ch_id,nullptr,16);

	  return retID;
	}

	/*ID 5: Returns an body from a CANBUS message*/
	unsigned long long parseResponseMSG(char* response)
	{
	  unsigned long long retMSG;

	  char ch_msg[17];
	  for(int i=0; i<16; i++)
	  {
	    ch_msg[i] = response[i+4];
	  }
	  ch_msg[16] = '\0';
	  retMSG = strtoull(ch_msg,nullptr,16);

	  return retMSG;
	}
};

#endif
