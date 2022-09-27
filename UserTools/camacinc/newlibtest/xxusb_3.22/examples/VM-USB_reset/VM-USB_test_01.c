// VM-USB test program
// WIENER, Plein & Baus Ltd. 04/2006
// Revision 1.0 - 04/11/06
// Andreas Ruben, aruben@wiener-us.com
//
#include <stdio.h>
#include <string.h>
#include "time.h"
#include <libxxusb.h>


// General variables
long wbuf, Data;			// common data
int ret;					// return of function calls
struct usb_device *dev;
xxusb_device_type xxusbDev[32];
// VM-USB Slave module settings 
long VM_Slave_SBA = 0x88000000; // default base address as per jumper setting
long VM_Slave_VBA;			// variable base address




//-------------------------------------------------------------------------
// Test of 32 bit read / write in EASY MODE
// write / read back 32 data VME window (Address 0 to 0xFFFE)
// requires VDIS-2 

short VMUSB_test_RW32 (usb_dev_handle *hdev, short Address_Modifier)
{
	long data_w, data_r, adr;
	int i;
	int errors =0;
	
	
	// write / read back 32 data VME window (Address 0 to 0x1FFE as words in Std_NoPriv_Data)
	for (i =0xfff0; i < 0xFFFF; i+=4)         
	{
		data_w = i/4+i/4*0x10000;                  // write and increment word address
		adr = i+i*0x10000;                  
		ret = VME_write_32(hdev,Address_Modifier, adr, data_w);
		ret = VME_read_32(hdev,Address_Modifier, adr, &data_r);

		if (data_w != data_w) 
		  errors++;
	}

	return errors;
}





//-------------------------------------------------------------------------
// Test of 32 bit BLT in EASY mode
// write 32bit data read with BLT 
short VMUSB_test_BLT32 (usb_dev_handle *hdev, short Address_Modifier, long BADR)
{
	int data_w, i, j, k;
	long BLT_buf[1000];
	long check, adr;
	int count = 31;
	int errors =0;
	
	
	// write / read back 32 data VME window (Address 0 to 0x1FFE as words in Std_NoPriv_Data)
	for (j = 0; j < 0x20; j++)         // 8k memory segment = 8 x 4 * 256 (BLT)
	{
		for (i = 0; i < 0x100; i++)         
		{
			data_w = i+j*0x100;
			k= i*4;
			adr = BADR+j*0x100+k;                  
			ret = VME_write_32(hdev,0x09, adr, data_w);
		}
		adr = BADR+j*0x100;
		ret = VME_BLT_read_32(hdev,Address_Modifier, count, adr, BLT_buf);
		for (i = 0; i < 31; i++)         
		{
			k=4*i;
			check = i+j*0x100;
		    if (check != BLT_buf[i]) 
		    {
        //printf("  BLT32 error at i = %i j = %i %lx : %lx\n",i,j,check, BLT_buf[i]);
        errors++;
        }
		}
		//printf("  BLT32 test loop %d  \n",j);
	}

	return errors;
}




//-------------------------------------------------------------------------
// Test of 32 bit BLT in STACK mode
// write 32bit data read with BLT 
short VMUSB_stack_BLT32 (usb_dev_handle *hdev, short Address_Modifier, long BADR)
{
  long stack [100];
	int data_w, i,j,k;
	long adr;
	unsigned int IntArray [10000];  //-> produces 32 bit data structure, use only with event alignment!



   // fill 8k of VM-USB SLAVE with data
	for (i = 0; i < 0x1FFF; i+=2)         
	{
		data_w = i+(i+1)*0x100+j*0x1000;
		adr = BADR+j*0x100+i;                  
		ret = VME_write_16(hdev,Address_Modifier, adr, data_w);
	}
// prepare stack
	stack[0] = 0x5; 
	stack[1] = 0x0;
	stack[2] = 0x010B;
	stack[3] = 0x3200;
	stack[4] = 0x0000;
	stack[5] = 0xEE00;
	ret = xxusb_stack_write(hdev, 2, stack);
//	Set internal pulser to 1ms repetition rate
	ret = VME_DGG(hdev,0,6,0,0xffff,0xff,0,0);   // Setup Pulser for O1
	ret = VME_DGG(hdev,1,6,1,0xffff,0xff,0,0);   // Setup Pulser for O1
	ret = VME_LED_settings(hdev,1,1,0,0);		 // Setup Red LED on Pulser for O1
//	set Trigger delay to 10us, no scaler AM=0x100D for internal register
    VME_write_32(hdev,0x100D,8,0xa );   

//	set VM-USB output buffer in Global Mode Register to:
//	1 = 8k (recommended by Jan)
//	2 = 4k
//  5 =	512
    Data = 1;  
    VME_write_32(hdev,0x100D,4,Data);
//  Start DAQ mode
	xxusb_register_write(hdev, 1, 0x1); // start acquisition 
    k=0;
    while(k<0x1000) // number of blocks to read
    {
//	ret = xxusb_usbfifo_read(hdev, IntArray, 8192, 20);		// slow byte to word combination
	ret = xxusb_bulk_read(hdev, IntArray, 8192, 100);	
	if(ret>0)
	{
		printf("Read return= %i\n ",ret);
//		for (i=0; i < ret; i=i+1)
//		{	
//			fprintf(DataFile, "%x\n",IntArray[i]);
//		} 
	}
	else 	printf("no read\n");
	k++;
// leave DAQ mode
    xxusb_register_write(hdev, 1, 0x0);
// drain all data from fifo    
	ret=1;
	k=0;
	while(ret>0)
    {
		ret = xxusb_bulk_read(hdev, IntArray, 8192, 100);	
		if(ret>0)
		{
			printf("drain loops: %i (result %i)\n ",k,ret);
			k++;
			if (k>10) ret=0;
		}
	}
//	in case still DAQ mode -> leave!!!
//  xxusb_register_write(hdev, 1, 0x0);
	}


	return 1;
}


int main()
{	

	int ret;
	usb_dev_handle *udev;   
	xxusb_device_type devices[100]; 
	char* Serial;
	int errors=0;


  xxusb_devices_find(devices);
  dev = devices[0].usbdev;
  udev = xxusb_device_open(dev); 
  
  if(!udev)
  {
    printf("\n\nNo VM_USB present\n\nTEST FAILED");
    return 0; 
  } 
  
  Serial= devices[0].SerialString;
  printf("\n\n%s\n\n", Serial);
    


	ret = VMUSB_test_RW32 (udev, 0x09);
	if (!ret)
		printf("32 bit Read Write test passed \n");
	else
	 {
	   printf("32 bit Read Write test failed with %i errors \n", ret);
	   errors=errors+ret;
	 }
	   
	   
  ret = VMUSB_test_BLT32 (udev, 0x0b, VM_Slave_SBA);
	if (!ret)
		printf("32 bit BLT test passed \n");
	else
	 {
	   printf("32 bit BLT test failed with %i errors \n", ret);
	   errors=errors+ret;
	 }
	   
//	ret = VMUSB_stack_BLT32 (udev, 0x0b, VM_Slave_SBA);

	


	xxusb_device_close(udev);


  if(errors)
    printf("\n\n************** TEST FAILED ********************\n\n");
  else 
    printf("\n\n************** TEST PASSED ********************\n\n");
	
  
  return errors;
}

 
