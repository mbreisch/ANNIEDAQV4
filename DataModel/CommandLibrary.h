#ifndef _COMMANDLIBRARY_H_INCLUDED
#define _COMMANDLIBRARY_H_INCLUDED

#include <iostream>

using namespace std;


class CommandLibrary_ACC
{
    public:
        //Write
        uint64_t Global_Reset = 0x00000000;
        uint64_t RX_Buffer_Reset_Request = 0x00000002;
        uint64_t Generate_Software_Trigger = 0x00000010;
        uint64_t Read_ACDC_Data_Buffer = 0x00000020;
        uint64_t Trigger_Mode_Select = 0x00000030;
        uint64_t SMA_Polarity_Select = 0x00000038;  
        uint64_t Beamgate_Window_Start = 0x0000003A;
        uint64_t Beamgate_Window_Length = 0x0000003B;
        uint64_t PPS_Divide_Ratio = 0x0000003C;
        uint64_t PPS_Beamgate_Multiplex = 0x0000003D;
        
        //Utility
        uint64_t PPS_Input_Use_SMA = 0x00000090;
        uint64_t Beamgate_Trigger_Use_SMA = 0x00000091;
        uint64_t ACDC_Command = 0x00000100;

        //Read
        uint64_t Trigger_Mode_Readback = 0x00000030;
        uint64_t SMA_Polarity_Readback = 0x00000038;
        uint64_t Beamgate_Window_Start_Readback = 0x0000003A;
        uint64_t Beamgate_Window_Length_Readback = 0x0000003B;
        uint64_t Firmware_Version_Readback = 0x00001000;
        uint64_t Firmware_Date_Readback = 0x00001001;
        uint64_t PLL_Lock_Readback = 0x00001002;
        uint64_t ACDC_Board_Detect = 0x00001003;
        uint64_t External_CLock_Lock_Readback = 0x00001004;
        uint64_t Data_Frame_Receive = 0x00002000;
        uint64_t RX_Buffer_Empty_Readback = 0x00002001;
        uint64_t RX_Buffer_Size_Readback = 0x00002010;
        uint64_t RX_Buffer_Size_Ch0123_Readback = 0x00002019;
        uint64_t RX_Buffer_Size_Ch4567_Readback = 0x00002018;

    private:

};

class CommandLibrary_ACDC
{
    public:
        uint64_t Set_DLLVDD = 0x00A00000;
        uint64_t Set_Pedestal_Offset = 0x00A20000;
        uint64_t Set_RingOsz_Voltage = 0x00A40000;
        uint64_t Set_Selftrigger_Threshold = 0x00A60000;
        uint64_t Set_Triggermode = 0x00B00000;
        uint64_t Set_Selftrigger_Mask_0 = 0x00B10000; 
        uint64_t Set_Selftrigger_Mask_1 = 0x00B11000; 
        uint64_t Set_Selftrigger_Mask_2 = 0x00B12000; 
        uint64_t Set_Selftrigger_Mask_3 = 0x00B13000; 
        uint64_t Set_Selftrigger_Mask_4 = 0x00B14000; 
        uint64_t Set_Selftrigger_Coincidence_Min = 0x00B15000;
        uint64_t Set_Selftrigger_Sign = 0x00B16000;
        uint64_t Set_Selftrigger_Use_Coincidence = 0x00B18000;
        uint64_t SMA_Polarity_Select = 0x00B20000;
        uint64_t Enable_Transfer = 0x00B50000;
        uint64_t Trigger_Reset_Request = 0x00B51000;
        uint64_t Event_And_Time_Reset_Request = 0x00B52000;
        uint64_t Disable_Transfer = 0x00B54000;
        uint64_t Trigger_Test_Mode = 0x00B60000;
        uint64_t Calibration_Mode = 0x00C00000;
        uint64_t ID_Frame_Request = 0x00D00000;
        uint64_t LED_Mode_Select = 0x00E00000;
        uint64_t LED_Test_Mode = 0x00800000;
        uint64_t Test_Mode_0 = 0x00F00000;
        uint64_t DLL_Reset_Request = 0x00F20000;
        uint64_t Global_Reset = 0x00FF0000; 


    private:

};
#endif
