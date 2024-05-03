#ifndef ACC_H
#define ACC_H

#include <vector>

using namespace std;

class ACC 
{
    public:
        virtual ~ACC() {} // Virtual destructor to ensure proper cleanup in case of polymorphic deletion

        virtual vector<unsigned short> ReturnRawData() = 0;
        virtual vector<unsigned short> ReturnACCIF() = 0;
        virtual vector<int> ReturnBoardIndices() = 0;
        virtual vector<unsigned int> ReturnErrors() = 0;
        virtual void SetNumChCoin(unsigned int in) = 0;
        virtual void SetEnableCoin(int in) = 0;
        virtual void SetThreshold(unsigned int in) = 0;
        virtual void SetPsecChipMask(vector<int> in) = 0;
        virtual void SetPsecChannelMask(vector<unsigned int> in) = 0;
        virtual void SetValidationStart(unsigned int in) = 0;
        virtual void SetValidationWindow(unsigned int in) = 0;
        virtual void SetTriggermode(int in) = 0;
        virtual void SetSign(int in, int source) = 0;
        virtual void SetPPSRatio(unsigned int in) = 0;
        virtual void SetPPSBeamMultiplexer(int in) = 0;
        virtual void SetTimeoutInMs(int in) = 0;
        virtual int InitializeForDataReadout(unsigned int boardmask = 0xFF, int triggersource = 0) = 0;
        virtual int SetTriggerSource(unsigned int boardmask = 0xFF, int triggersource = 0) = 0;
        virtual int ListenForAcdcData(int triggersource, vector<int> LAPPD_on_ACC) = 0;
        virtual void VersionCheck() = 0;
        virtual void GenerateSoftwareTrigger() = 0;
        virtual void DumpData(unsigned int boardMask = 0xFF) = 0;
        virtual void ResetACDC() = 0;
        virtual void ResetACC() = 0;
        virtual void SetSMA_Debug(unsigned int PPS, unsigned int Beamgate) = 0;
        virtual bool SetPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc) = 0;
        virtual bool PPS_TestMode(int state) = 0;
        virtual void ClearData() = 0;
        virtual void ClearErrors() = 0;
        virtual bool EmptyUsbLine() = 0;
};

#endif 