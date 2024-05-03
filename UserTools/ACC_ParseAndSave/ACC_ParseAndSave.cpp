#include "ACC_ParseAndSave.h"

ACC_ParseAndSave::ACC_ParseAndSave():Tool(){}


bool ACC_ParseAndSave::Initialise(std::string configfile, DataModel &data)
{

    if(configfile!="")  m_variables.Initialise(configfile);

    m_data= &data;
    m_log= m_data->Log;

    if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

    m_variables.Get("EventsPerFile",EvtsPerFile);
    m_variables.Get("Savemode",m_data->local.Savemode);
    
    m_data->local.time = getTime();

    starttime = getTime();

    system("mkdir -p Results")

    return true;
}


bool ACC_ParseAndSave::Execute()
{
    bool retfinish=false;

    if(m_data->psec.readRetval!=0)
    {
        return true;
    }

    //Savemode check
    if(m_data->local.Savemode==0) //If no save is chosen skip tool
    {
        retfinish = true;
    }else if(m_data->local.Savemode==1) //ASCII save in parsed files
    {
        int retval, type;
        if(m_data->psec.ReceiveData.size()%16==0)
        {
            type = 16; 
        }else if(m_data->psec.ReceiveData.size()%7795==0)
        {
            type = 7795;
        }
        int n_bo = m_data->psec.BoardIndex.size();

        for(int i=0; i<n_bo; i++)
        {
            for(int cj=i*type; cj<(i+1)*type; cj++)
            {
                m_data->local.TransferMap[m_data->psec.BoardIndex.at(i)].push_back(m_data->psec.ReceiveData.at(cj));
            }
        }

        for(std::map<int, vector<unsigned short>>::iterator it=m_data->local.TransferMap.begin(); it!=m_data->local.TransferMap.end(); ++it)
        {
            //fill ParsedStream with vectors from data
            retval = getParsedData(it->second);
            if(retval == -3) //If a PPS was seen skip in this mode
            {
                return true;
            }else if(retval == 0)
            {
                m_data->local.ParseData[it->first]=data;
                retval = getParsedMeta(it->second,it->first);
                if(retval!=0)
                {
                    std::cout << "Meta parsing went wrong! " << retval << std::endl;
                }
                m_data->local.ParseMeta[it->first]=meta;	
            }else if(retval!=0 && retval!=-3)
            {
                std::cout << "Parsing went wrong! " << retval << std::endl;
            }
            channel_count = 0;
        }

		if(m_data->local.counter>EvtsPerFile)
		{
			m_data->local.counter=0;
			m_data->local.time = getTime();
		}
		SaveASCII(m_data->local.time);
		m_data->local.counter +=1;	

        retfinish = true;
    }else if(m_data->local.Savemode==2)
    {
        int retval, type;
        if(m_data->psec.ReceiveData.size()%16==0)
        {
            type = 16; 
        }else if(m_data->psec.ReceiveData.size()%7795==0)
        {
            type = 7795;
        }
        int n_bo = m_data->psec.BoardIndex.size();

        for(int i=0; i<n_bo; i++)
        {
            for(int cj=i*type; cj<(i+1)*type; cj++)
            {
                m_data->local.TransferMap[m_data->local.BoardIndex.at(i)].push_back(m_data->local.ReceiveData.at(cj));
            }
        }

		if(m_data->local.counter>EvtsPerFile)
		{
			m_data->local.counter=0;
			m_data->local.time = getTime();
		}
		SaveRaw(m_data->local.time);
		m_data->local.counter +=1;

        //Basic return raw
        retfinish = true;
    }else
    {
        std::cout << "Unknown save mode set!" << std::endl;
    }

	//Cleanup	
	m_data->local.ParseData.clear();
	m_data->local.ParseMeta.clear();
	m_data->psec.AccInfoFrame.clear();
	m_data->psec.BoardIndex.clear();
	m_data->psec.ReceiveData.clear();
    m_data->psec.RawWaveform.clear();
    m_data->local.TransferMap.clear();

    m_data->local.DataSaved++;
    int MaxEvents;
    m_variables.Get("MaxEvents",MaxEvents);
    if(m_data->local.DataSaved==MaxEvents)
    {
        m_data->vars.Set("StopLoop",1);
    }

    return retfinish,
}


bool ACC_ParseAndSave::Finalise()
{

    std::tm tm = {};
    std::stringstream ss(starttime);
    ss >> std::get_time(&tm, "%Y%d%m_%H%M%S");
    auto start = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    auto end = chrono::system_clock::now();
    auto dt = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Runtime was " << to_string(dt) << " ms"  << endl;

    return true;
}

int ACC_ParseAndSave::getParsedMeta(std::vector<unsigned short> buffer, int classindex)
{
	//Catch empty buffers
	if(buffer.size() == 0)
	{
		std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
		return -1;
	}

 	//Prepare the Metadata vector 
	meta.clear();

	//Helpers
	int chip_count = 0;

	//Indicator words for the start/end of the metadata
	const unsigned short startword = 0xBA11; 
	unsigned short endword = 0xFACE; 
    unsigned short endoffile = 0x4321;

	//Empty metadata map for each Psec chip <PSEC #, vector with information>
	map<int, vector<unsigned short>> PsecInfo;

	//Empty trigger metadata map for each Psec chip <PSEC #, vector with trigger>
	map<int, vector<unsigned short>> PsecTriggerInfo;
	unsigned short CombinedTriggerRateCount;

	//Empty vector with positions of aboves startword
	vector<int> start_indices=
    {
        1539, 3091, 4643, 6195, 7747
    }; 

	//Find the startwords and write them to the vector
	vector<unsigned short>::iterator bit;
	//Fill the psec info map
	for(int i: start_indices)
	{
		//Write the first word after the startword
		bit = buffer.begin() + (i+1);

		//As long as the endword isn't reached copy metadata words into a vector and add to map
		vector<unsigned short> InfoWord;
		while(*bit != endword && *bit != endoffile && InfoWord.size() < 14)
		{
			InfoWord.push_back(*bit);
			++bit;
		}
		PsecInfo.insert(pair<int, vector<unsigned short>>(chip_count, InfoWord));
		chip_count++;
	}

	//Fill the psec trigger info map
	for(int chip=0; chip<NUM_PSEC; chip++)
	{
		for(int ch=0; ch<NUM_CH/NUM_PSEC; ch++)
		{
			//Find the trigger data at begin + last_metadata_start + 13_info_words + 1_end_word + 1 
			bit = buffer.begin() + start_indices[4] + 13 + 1 + 1 + ch + (chip*(NUM_CH/NUM_PSEC));
			PsecTriggerInfo[chip].push_back(*bit);
		}
	}

	//Fill the combined trigger
	CombinedTriggerRateCount = buffer[7792];

	//----------------------------------------------------------
	//Start the metadata parsing 
	meta.push_back(classindex);
	for(int CHIP=0; CHIP<NUM_PSEC; CHIP++)
	{
		meta.push_back((0xDCB0 | CHIP));
		for(int INFOWORD=0; INFOWORD<13; INFOWORD++)
		{
			meta.push_back(PsecInfo[CHIP][INFOWORD]);		
		}
		for(int TRIGGERWORD=0; TRIGGERWORD<6; TRIGGERWORD++)
		{
			meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);
		}
	}

	meta.push_back(CombinedTriggerRateCount);
	// dummy timestamp
    // ----------------------------------------------------------------------------------------------------
    uint64_t timestamp = static_cast<uint64_t>(std::time(nullptr));
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << timestamp; //Makes a 64bit hex string from the timestamp

    //Splits the 64bit hex sting into 4 16bit hex stings
    std::string hex_parts[4];
    for(int i = 0; i < 4; ++i)
    {
            hex_parts[i] = ss.str().substr(i*4, 4);
    }

    // Converting each part to unsigned short
    for(int i = 0; i < 4; ++i)
    {
            std::istringstream iss(hex_parts[i]);
            unsigned short short_value;
            iss >> std::hex >> short_value;
            meta.push_back(short_value);
    }
    // ----------------------------------------------------------------------------------------------------
	meta.push_back(0xeeee);
	return 0;
}


int ACC_ParseAndSave::getParsedData(std::vector<unsigned short> buffer)
{
	//Catch empty buffers
	if(buffer.size() == 0)
	{
		std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
		return -1;
	}
	if(buffer.size() == 16)
	{
		data.clear();
		return -3;	
	}

	//Prepare the Metadata vector 
	data.clear();
    channel_count=0;

	//Indicator words for the start/end of the metadata
	const unsigned short startword = 0xF005; 
	unsigned short endword = 0xBA11;
	unsigned short endoffile = 0x4321;

	//Empty vector with positions of aboves startword
	vector<int> start_indices=
    {
        2, 1554, 3106, 4658, 6210
    }; 

	//Find the startwords and write them to the vector
	vector<unsigned short>::iterator bit;
	//Fill data map
	for(int i: start_indices)
	{
		//Write the first word after the startword
		bit = buffer.begin() + (i+1);

		//As long as the endword isn't reached copy metadata words into a vector and add to map
		vector<unsigned short> InfoWord;
		while(*bit != endword && *bit != endoffile)
		{
			InfoWord.push_back((unsigned short)*bit);
			if(InfoWord.size()==NUM_SAMP)
			{
				data.insert(pair<int, vector<unsigned short>>(channel_count, InfoWord));
				InfoWord.clear();
				channel_count++;
			}
			++bit;
		}	
	}

	return 0;
}


void ACC_ParseAndSave::SaveRaw(string time)
{
	//Direct raw save of data
	for(std::map<int, vector<unsigned short>>::iterator it=m_data->local.TransferMap.begin(); it!=m_data->local.TransferMap.end(); ++it)
	{
		string rawfn = "./Results/Raw_b" + to_string(it->first) + "_" + time + ".txt";
		ofstream d(rawfn.c_str(), ios::app); 
		for(unsigned short k: it->second)
		{
			d << hex <<  k << " ";
		}
		d << endl;
		d.close();
	}
}

void ACC_ParseAndSave::SaveASCII(string time)
{
	string rawfn = "./Results/Ascii" + time + ".txt";
	ofstream d(rawfn.c_str(), ios::app); 

	vector<int> boardsReadyForRead = m_data->psec.BoardIndex;

	map<int,map<int, vector<unsigned short>>> map_data = m_data->local.ParseData;
	map<int, vector<unsigned short>> map_meta = m_data->local.ParseMeta;

	string delim = " ";
	for(int enm=0; enm<NUM_SAMP; enm++)
	{
		d << dec << enm << delim;
		for(int bi: boardsReadyForRead)
		{
			if(map_data[bi].size()==0)
			{
				cout << "Mapdata is empty" << endl;
			}
			for(int ch=0; ch<NUM_CH; ch++)
			{
				if(enm==0)
				{
					//cout << "Writing board " << bi << " and ch " << ch << ": " << map_data[bi][ch+1][enm] << endl;
				}
				d << dec << (unsigned short)map_data[bi][ch][enm] << delim;
			}
			if(enm<(int)map_meta[bi].size())
			{
				d << hex << map_meta[bi][enm] << delim;

			}else
			{
				d << 0 << delim;
			}
		}
		d << endl;
	}
	d.close();

    boardsReadyForRead.clear();
    map_data.clear();
    map_meta.clear();
}