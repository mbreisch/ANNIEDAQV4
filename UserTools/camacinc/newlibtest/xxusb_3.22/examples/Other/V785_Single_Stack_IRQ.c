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


#define V785_BA 0x00400000
#define V785_FIRMWARE_ID 0x1000
#define V785_SERIAL_MSB 0x8F02
#define V785_SERIAL_LSB 0x8F06
#define V785_IRQ_LEVEL 0x100A
#define V785_IRQ_VECTOR 0x100C
#define V785_EVENT_TRIGGER 0x1020

int main (int argc,  char *argv[])
{
    int ret, j;
    //int *Data_Array=malloc(1000);
    long Data=0;
    xxusb_device_type devices[100]; 
    struct usb_device *dev;
    usb_dev_handle *udev;       // Device Handle 
    long Stack[6];
    Stack[0] = 0x0005;
    Stack[1] = 0;
    Stack[2] = 0x010B;
    Stack[3] = 0x2200;
    Stack[4] = 0;
    Stack[5] = 0x0040;
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


    // Set Red LED to light with NIM1
    VME_LED_settings(udev, 1,1,0,0);   


    // Set DGG channel B to trigger on NIM1, output on O2,
    //     with delay =500 x 12.5ns,
    //     and width =  500 x 12.5ns,
    //     not latching or inverting
    VME_DGG(udev,0,2,0,0,0x200,0,0);
    

    
    //Read the Firmware ID of the V785
    VME_read_16(udev, 0x9, V785_BA+V785_FIRMWARE_ID, &Data );
    printf("\n\nV785 Firmware Revision: %lx", Data);
    
    
    //Get V785 serial Number
    ret=VME_read_16(udev, 0x9, V785_BA+V785_SERIAL_LSB, &Data );
    printf("\nV785 Serial Number: %li", (Data & 0xFFF));
    
    //Set IRQ Level 1
    VME_write_16(udev, 0x9, V785_BA+V785_IRQ_LEVEL, 0x2);

    //Set IRQ Vector 0XAB
    VME_write_16(udev, 0x9, V785_BA+V785_IRQ_VECTOR, 0xAB);
    
    //Set Event trigger to 8 events
    VME_write_16(udev, 0x9, V785_BA+V785_EVENT_TRIGGER, 0x8);
    
    VME_register_write(udev, 0x28, 0x22AB);
    
    
    // Write Stack to VM_USb
    xxusb_stack_write(udev, 0x10, Stack);
    
    
    
    // Read Stack back from VM_USB and Display
    printf("\n\n\nStack Read from Module\n");
    xxusb_stack_read(udev,0x10, Stack_read);
    for(j=0;j<6;j++)
    {
      printf("%lx\n", Stack_read[j]);
    }
    printf("\n\n");
    
    VME_register_read(udev, 0x4, &Data);
    //Data = Data & 0xFFFFFF00;
    Data = Data | 0x01;  
    VME_register_write(udev, 0x4, Data);
    
    int k=0;
    int l=0;

    int* Data_Array=malloc(1000);

    
    
    xxusb_register_write(udev,1,0x1);   // Start DAQ mode
    
	//poll Fifo 10 times
	while(k <10)
    {
      ret=xxusb_usbfifo_read(udev, Data_Array,8192,30);
	    if(ret>0)
	    {
	        for(l=0;l<8192;l++)
		        fprintf(DataFile, "%x\n",Data_Array[l]);
	    } else {
	        printf("no data: %i\n", ret);
	    }
      k++;
    }


    xxusb_register_write(udev,1,0x0);   // stop acquistion


	// read from fifo until Time out error and discard all data
    ret = xxusb_usbfifo_read(udev, Data_Array, 8192, 30);
    while(ret >0)
    {
      ret = xxusb_usbfifo_read(udev, Data_Array, 8192, 30);    
      //printf("drain Buffer\n");
    }
    printf("Buffer Empty\n");
    
    

    // Close the Device
    xxusb_device_close(udev);
    printf("\n\n\n");
    
    return 0;
}

