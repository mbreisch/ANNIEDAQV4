#ifndef _STDUSB_H_INCLUDED
#define _STDUSB_H_INCLUDED

#include <libusb-1.0/libusb.h> //represents the new version of the usb lib
#include <vector>

using namespace std;

#define INVALID_HANDLE_VALUE NULL
#define USB_TOUT_MS_WRITE 50 // in ms
#define USB_TOUT_MS_READ 50 // in ms
#define EP_WRITE 0x02 //USBFX2 end point address for bulk write
#define EP_READ 0x86 //USBFX2 end point address for bulk read
#define INTFNO 0 //USBFX2 interface number
#define CNFNO 1 //USBFX2 configuration number

class stdUSB {
    public:
        stdUSB(); //empty usb constructor
        stdUSB(uint16_t vid, uint16_t pid); //usb constructor with vid/pid settings
        ~stdUSB(); //usb deconstructor

        bool createHandles(int device_count = 1); //creates usb handles and claims the interface
        bool freeHandle(); //frees already allocated handles

        bool sendData(unsigned int data); //sends a data package, in this case commands to the connected device 
        int readData(unsigned char *pData, int *lread); //recieves a data package of 512Byte
		vector<unsigned short> safeReadData(int maxSamples); //allocates memory properly for reading

        bool isOpen(); //control function for open usb line
        bool reset(); //reset usb connection

    private:
        struct libusb_device* init(int device_count); //initilises the usb device for claiming handles
        struct libusb_device_handle *stdHandle; //usb handle struct
	    struct libusb_context *usb_context;
	
        /* USBFX2 device descriptions */
        uint16_t USBFX2_VENDOR_ID; //0x090c;
        uint16_t USBFX2_PRODUCT_ID; //0x1000;  
};

#endif
