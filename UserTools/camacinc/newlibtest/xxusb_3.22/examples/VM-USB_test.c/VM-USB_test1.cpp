// VM-USB test program
// WIENER, Plein & Baus Corp. 01/2014
// Revision 2.0 - 01/22/14 tested with VM-DBA
// Andreas Ruben, aruben@wiener-us.com

/* Simple program to demonstrate VM_USB in EASY mode
 * 
 * Copyright (C) 2005-2009 WIENER, Plein & Baus, Ltd (thoagland@wiener-us.com)
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation, version 2.
 *
 *
 *
*/

#include <libxxusb.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <cstdlib>
#include <string.h>


int main (int argc,  char *argv[])
{
  int ret, i;
  xxusb_device_type devices[100];
  int VM_reg [20];
  struct usb_device *dev;
  usb_dev_handle *udev;       // Device Handle 
  long data_w, data_r, adr;
  int errors =0;
  int Address_Modifier= 0x09;

//Find XX_USB devices and open the first one found
  xxusb_devices_find(devices);
  dev = devices[0].usbdev;
  udev = xxusb_device_open(dev); 
  
// Make sure VM_USB opened OK
  if(!udev) {
    printf ("\n\nFailed to Open VM_USB \n\n");
    return 0;
  } else {
    printf("Device open\n"); 
  }
//  Prepare and READ VM-USB Registers for display and log file
  for (i=0; i<=15; i++)
  {
      ret = VME_register_read(udev, i, &data_r);
      VM_reg[i]= data_r;
  }
  printf("VM-USB Register:\n");
  printf("%x \t%x \t%x \t%x \t%x \n",VM_reg[0],VM_reg[1],VM_reg[2],VM_reg[3],VM_reg[4]);
  printf("%x \t%x \t%x \t%x \t%x \n",VM_reg[5],VM_reg[6],VM_reg[7],VM_reg[8],VM_reg[10]);
  printf("%x \t%x \t%x \t%x \t%x \n",VM_reg[11],VM_reg[12],VM_reg[13],VM_reg[14],VM_reg[15]);
// write / read back 32 data VME window (Address 0 to 0x1FFE as words in Std_NoPriv_Data)
  for (i =0x0; i < 0xFFFF; i+=0x4)
  {
        data_w = i/4+i/4*0x10000;                  // write and increment word address
        adr = i+i*0x10000;
        ret = VME_write_32(udev,Address_Modifier, adr, data_w);
        ret = VME_read_32(udev,Address_Modifier, adr, &data_r);
        if (data_w != data_w)
		  errors++;
  }
  printf("VME bus test errors: %i \n", errors);
  return errors;
// Close the Device
  xxusb_device_close(udev);
  printf("\n\n\n");
  
  return 0;
}

