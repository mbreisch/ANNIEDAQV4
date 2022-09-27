/* Simple program to demonstrate VM_USB in multiple stack mode
 * 
 * Copyright (C) 2005-2009 WIENER, Plein & Baus, Ltd (thoagland@wiener-us.com)
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation, version 2.
 *
 *
 *
 *  This program file is an example of how to write two seperate stacks
 *  to the VM_USB and execute them on different triggers.  To do this the 
 *  WIENER VDIS module is used.    
 *

*/

#include <libxxusb.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <cstdlib>

// Base address of VDIS
#define BA 0x8800


int main (int argc,  char *argv[])
{
    int ret, j;
    long Data=0;
    xxusb_device_type devices[100]; 
    struct usb_device *dev;
    usb_dev_handle *udev;       // Device Handle 
    
    //This is the stack that executes on IRQ.  It simply clear the interupt.
    long IRQ_Stack[8];
    IRQ_Stack[0] = 0x0007;
    IRQ_Stack[1] = 0;
    IRQ_Stack[2] = 0x0029;
    IRQ_Stack[3] = 0x00;
    IRQ_Stack[4] = 0x8820;
    IRQ_Stack[5] = 0x0000;
    IRQ_Stack[6] = 0x0002;
    IRQ_Stack[7] = 0x0000;
    
    
    //This is the stack that executes on I1.  It simply sets the interupt.
    long Trigger_Stack[8];
    Trigger_Stack[0] = 0x000B;
    Trigger_Stack[1] = 32;
    Trigger_Stack[2] = 0x0029;
    Trigger_Stack[3] = 0x00;
    Trigger_Stack[4] = 0x8820;
    Trigger_Stack[5] = 0x0000;
    Trigger_Stack[6] = 0x0001;
    Trigger_Stack[7] = 0x0000;


    //This array stores the Stack values read back from the VM_USB
    long Stack_read [12];


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
    
    //Read VM_USB Firmware Register
    VME_register_read(udev, 0x0, &Data);
    printf("\n\nFirmware_revision = %lx", (Data & 0xFFFF));
    
    //Read the status/control register of the VDIS
    VME_read_16(udev, 0x29, BA, &Data);
    printf("\nStatus Register = 0x%lx", Data);
    
    //Set the interupt vectot and interupt level information for the VDIS
    VME_write_16(udev, 0x29, BA+0x10, 0x1BB);
    
    // Set Red LED to light with NIM1
    VME_LED_settings(udev, 1,1,0,0);
    
    // Write IRQ Stack to VM_USB
    xxusb_stack_write(udev, 18, IRQ_Stack);
    VME_register_write(udev, 0x28, 0x21BB);
        
    // Write trigger Stack to VM_USB
    xxusb_stack_write(udev, 0x2, Trigger_Stack); 
    VME_register_write(udev, 0x2C, 0x000);     
    
    
    
    // Read IRQ Stack back from VM_USB and Display
    printf("\n\n\nStack Read from Module\n");
    xxusb_stack_read(udev,18, Stack_read);
    for(j=0;j<8;j++)
    {
      printf("%lx\n", Stack_read[j]);
    }
    printf("\n\n");
    
    
    // Read trigger Stack back from VM_USB and Display
    printf("\n\n\nStack Read from Module\n");
    xxusb_stack_read(udev,0x2, Stack_read);
    for(j=0;j<8;j++)
    {
      printf("%lx\n", Stack_read[j]);
    }
    printf("\n\n");


    //Set Buffer information for the VM_USB readout buffer
    VME_register_read(udev, 0x4, &Data);
    Data = Data | VME_register_read(udev, 0x4, &Data);  
    VME_register_write(udev, 0x4, Data);
    
    //Data Array for the VM_USB data to be stored in
    int* Data_Array=new int[1000];

    // Start DAQ mode
    xxusb_register_write(udev,1,0x1);   
    

    // poll the fifo 100000 times,  Data will be sent to buffer even though 
    //  we don't read anything out.  Every time a stack executes a header
    //  and EOB word is written to the buffer.  We simply empty the buffer.
    int key =0;
	  while(key <1000000)
    {
      ret=xxusb_usbfifo_read(udev, Data_Array,8192,30);
      key++;
    }
    printf("\n\nExited Loop\n");

    // stop acquistion
    xxusb_register_write(udev,1,0x0);   


	  // read from fifo until Time out error and discard all data
    ret = xxusb_usbfifo_read(udev, Data_Array, 8192, 30);
    while(ret >0)
      ret = xxusb_usbfifo_read(udev, Data_Array, 8192, 30);    


    // Close the Device
    xxusb_device_close(udev);
    printf("\n\n\n");
    
    return 0;
}

