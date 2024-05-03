#include "SeeConnectedBoards.h"

SeeConnectedBoards::SeeConnectedBoards():Tool(){}


bool SeeConnectedBoards::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

    if(!m_variables.Get("Interface_Name",m_data->TCS.Interface_Name)) m_data->TCS.Interface_Name="USB";
    if(!m_variables.Get("Interface_IP",m_data->TCS.Interface_IP)) m_data->TCS.Interface_IP="127.0.0.1";
    if(!m_variables.Get("Interface_Port",m_data->TCS.Interface_Port)) m_data->TCS.Interface_Port="8888";
	if(m_data->acc==nullptr)
    {
        if(strcmp(m_data->TCS.Interface_Name.c_str(),"USB") == 0)
        {
            m_data->acc = new ACC_USB();
        }else if(strcmp(m_data->TCS.Interface_Name.c_str(),"ETH") == 0)
        {
            m_data->acc = new ACC_ETH(m_data->TCS.Interface_IP, m_data->TCS.Interface_Port);
        }else
        {
            std::cout << "Invalid Interface_Name" << std::endl;
        }
    }

	return true;
}


bool SeeConnectedBoards::Execute()
{
    m_data->acc->VersionCheck();
    
	return true;
}


bool SeeConnectedBoards::Finalise()
{
	delete m_data->acc;
    m_data->acc=0;
	return true;
}

