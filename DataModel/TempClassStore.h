#ifndef TempClassStore_H
#define TempClassStore_H

#include <iostream>
#include <vector>
#include <map>
#include <queue>

#include<PsecData.h>

using namespace std;

class TempClassStore{


    public:

        TempClassStore();

        //Interface
        std::string Interface_Name;
        std::string Interface_IP;
        std::string Interface_Port;

        //New maps to allow for MuliLAPPDs <LAPPD_ID, value>
        map<int,queue<PsecData>> Buffer;
        map<int,int> Timeoutcounter;

    private:

};

#endif
