/* Simple VM_USB program
 * 
 * Copyright (C) 2005-2009 WIENER, Plein & Baus, Ltd (thoagland@wiener-us.com)
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation, version 2.
 *
 *
 *
 * This file will show the basic principles of using the xx_usb
 * Library.  It is offerered as a simple version idea of how to:
 *       Open the Device
 *       Set up the LEDs
 *       Set up a DGG
 *       Close the Device
 * In order for it to work be sure to change the serial number
 * To that of your VM_USB.
 *
include <libxxusb.h>
#include <stdio.h>



*/

#include <libxxusb.h>
#include <stdio.h>
#include <time.h>
#include <math.h>


#define BA 0x8800


int main (int argc,  char *argv[])
{
    int ret, j;
    //int *Data_Array=malloc(1000);
    long Data=0;
    xxusb_device_type devices[100]; 
    struct usb_device *dev;
    usb_dev_handle *udev;       // Device Handle 
    long Stack[8];
    Stack[0] = 0x0007;
    Stack[1] = 0;
    Stack[2] = 0x0029;
    Stack[3] = 0x00;
    Stack[4] = 0x8820;
    Stack[5] = 0x0000;
    Stack[6] = 0x0002;
    Stack[7] = 0x0000;
    long Stack_read [10];

    
    //setup output file
    FILE * DataFile;
    DataFile = fopen("VHQ_Info.txt","w");


    //Find XX_USB devices and open the first one found
    xxusb_devices_find(devices);
    dev = devices[0].usbdev;
    udev = xxusb_device_open(dev); 

    
    // Make sure VM_USB opened OK
    if(!udev) 
    {
	     printf ("\n\nFailedto Open VM_USB \n\n");
	     return 0;
    }
    
    VME_read_16(udev, 0x29, BA, &Data);
    printf("\n\nStatus Register = 0x%lx", Data);
    
    VME_write_16(udev, 0x29, BA+0x10, 0x2BB);
    
    
    


    
    // Write Stack to VM_USb
    xxusb_stack_write(udev, 0x2, Stack);
    
    
    
    // Read Stack back from VM_USB and Display
    printf("\n\n\nStack Read from Module\n");
    xxusb_stack_read(udev,0x02, Stack_read);
    for(j=0;j<8;j++)
    {
      printf("%lx\n", Stack_read[j]);
    }
    printf("\n\n");

    
    VME_register_read(udev, 0x4, &Data);
    //Data = Data & 0xFFFFFF00;
    Data = Data | 0x01;  
    VME_register_write(udev, 0x4, Data);
    

    //int* Data_Array=malloc(1000);

    
    VME_register_write(udev, 0x28, 0x02BB);
    xxusb_register_write(udev,1,0x1);   // Start DAQ mode
    
    char key;
/*    
	//poll Fifo 10 times
	while(key !="q")
    {
    
      ret=xxusb_usbfifo_read(udev, Data_Array,8192,30);
	    if(ret>0)
	    {
	        for(l=0;l<8192;l++)
		        fprintf(DataFile, "%x\n",Data_Array[l]);
	    } else {
	        printf("no data: %i\n", ret);
	    }
    
    printf(" %c ", key);
    key=getchar();
    }
    
    printf("\n\nExited Loop\n");
*/

    key= getchar();
    printf("\nstopping DAQ");
    
    xxusb_register_write(udev,1,0x0);   // stop acquistion

/*
	// read from fifo until Time out error and discard all data
    ret = xxusb_usbfifo_read(udev, Data_Array, 8192, 30);
    while(ret >0)
    {
      ret = xxusb_usbfifo_read(udev, Data_Array, 8192, 30);    
      //printf("drain Buffer\n");
    }
    printf("Buffer Empty\n");
 
 */   
    

    // Close the Device
    xxusb_device_close(udev);
    printf("\n\n\n");
    
    return 0;
}

