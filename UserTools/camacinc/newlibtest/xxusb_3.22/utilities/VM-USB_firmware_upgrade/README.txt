vmusb_flash.cpp

This Program was written to allow the user of a VM_USB to update the firmware
on the controller from a LINUX PC.  

Before compiling:
- The xxusb_lib library must be compiled and installed on you computer.


Before Running:
- Switch the firmware switch on the front of the module to on of the Programming
  positions (P1-P4)
- Ensure that the red Fail light is on. (if it isn't try power cycling the crate)


Running:
- The program takes the firmware file to load as an arguement.  The most recent
  firmware (as of 8/4/06) is located in the firmware directory.  More recent 
  versions may be avialable at www.wiener-d.com
- While running the program the red fail light should flash slowly indicating 
  that the firmware is being loaded.
- The new firmware ID word is displayed after the program completes


This file was written by:

Tim Hoagland 
       of 
WIENER, Plein & Baus, Ltd.

If you have any questions or problems with this utility, please contact Tim Hoagland at:
thoagland@wiener-us.com
