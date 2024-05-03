#ifndef LocalData_H
#define LocalData_H

#include <iostream>
#include <vector>
#include <map>
#include <queue>

using namespace std;

class LocalData{


    public:

        LocalData();

        map<int, vector<unsigned short>> TransferMap;
        map<int,map<int, vector<unsigned short>>> ParseData;
        map<int,vector<unsigned short>> ParseMeta;
        
        int counter;
        int DataSaved;
        string time;
        string Timestamp;

    private:

};

#endif
