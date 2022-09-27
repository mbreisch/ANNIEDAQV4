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
//#include <cstdlib>
#include <string.h>


int main (int argc,  char *argv[])
{
  
  int ret=0, i;
  xxusb_device_type devices[100];
  struct usb_device *dev;
  usb_dev_handle *udev;       // Device Handle 
  int IntArray [10000];  //for FIFOREAD
  //Find XX_USB devices and open the first one found
  xxusb_devices_find(devices);
  dev = devices[0].usbdev;
  udev = xxusb_device_open(dev); 
  
  // Make sure VM_USB opened OK
  if(!udev) {
    printf ("\n\nFailed to Open VM_USB \n\n");
    return 0;
  } else {
    printf("VM-USB Device open\n");
  }
  while (ret >= 0) {
    i = i + 1;
    ret = xxusb_usbfifo_read(udev, IntArray,512,30);
  }
   // Close the Device
  xxusb_device_close(udev);
  printf("\n VM-USB Device closed \n\n");
  
  return 0;
}

