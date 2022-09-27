#include <stdio.h>
#include <usb.h>
main(){
  printf("A");
  struct usb_bus *bus;
  printf("B");
struct usb_bus *usb_busses;
 printf("C");
  struct usb_device *dev;
  printf("D");
  usb_init();
  printf("E");
  usb_find_busses();
  printf("F");
  usb_find_devices();
  printf("G");
  for (bus = usb_busses; bus; bus = bus->next)
    printf("H");
    for (dev = bus->devices; dev; dev = dev->next){
      printf("Trying device %s/%s\n", bus->dirname, dev->filename);
      printf("\tID_VENDOR = 0x%04x\n", dev->descriptor.idVendor);
      printf("\tID_PRODUCT = 0x%04x\n", dev->descriptor.idProduct);
    }
}
