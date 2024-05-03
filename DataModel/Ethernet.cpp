#include "Ethernet.h"

Ethernet::Ethernet(std::string ipaddr, std::string port, int verbose) : m_socket(-1), servinfo(NULL)
{
    bool ret = OpenInterface(ipaddr, port);
    if(ret==false)
    {
        std::cerr << "Failed to open the interface ending program" << std::endl;
        CloseInterface();
        exit(EXIT_FAILURE);
    }else
    {   
        if(verbose>0)
        {
            std::cout << "Connected to the ACC with " << ipaddr << ":" << port << std::endl;
        }
    }
}


Ethernet::~Ethernet()
{
    CloseInterface();
}


bool Ethernet::OpenInterface(std::string ipaddr, std::string port)
{
    struct addrinfo  hints;
    int s;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if((s = getaddrinfo(ipaddr.c_str(), port.c_str(), &hints, &servinfo)) != 0)
    {
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return false;
    }

    // loop through all the results and make a socket
    for(list_of_addresses = servinfo; list_of_addresses != NULL; list_of_addresses = list_of_addresses->ai_next)
    {
        m_socket = socket(list_of_addresses->ai_family, list_of_addresses->ai_socktype, list_of_addresses->ai_protocol);
        if(m_socket == -1){continue;}

        break;
    }

    if(list_of_addresses == NULL || m_socket==-1)
    {
	    fprintf(stderr, "Failed to create socket\n");
        return false;
    }
    
    
    FD_ZERO(&rfds_);
    FD_SET(m_socket, &rfds_);

    return true;
}


void Ethernet::CloseInterface()
{
    if(m_socket>0) 
    {
        freeaddrinfo(servinfo);
        close(m_socket);
        m_socket = 0;
        std::cout << "Disconnected from ACC" << std::endl;
    }
}


void Ethernet::SwitchToBurst()
{
    int numbytes;
        
    buffer[0] = 2; // 2 = burst
    int packetSz = 1;

    if((numbytes = sendto(m_socket, buffer, packetSz, 0, list_of_addresses->ai_addr, list_of_addresses->ai_addrlen)) == -1)
    {
        perror("sw: sendto");
        exit(1);
    }
}


void Ethernet::SetBurstState(bool state)
{
    int numbytes;
        
    buffer[0] = 1; //Send without back     
    buffer[1] = 1; //Number of words        
    uint64_t addr = 0x0000000100000009;  // data enable
    memcpy((void*)&buffer[RX_ADDR_OFFSET_], (void*)&addr, 8);
    memset((void*)&buffer[RX_DATA_OFFSET_], state?1:0, 1);
    memset((void*)&buffer[RX_DATA_OFFSET_ + 1], 0, 7);
    int packetSz = RX_DATA_OFFSET_ + buffer[1] * 8;

    if((numbytes = sendto(m_socket, buffer, packetSz, 0, list_of_addresses->ai_addr, list_of_addresses->ai_addrlen)) == -1)
    {
        perror("sw: sendto");
        exit(1);
    }
}


bool Ethernet::SendData(uint64_t addr, uint64_t value, std::string read_or_write) 
{
    if(m_socket<=0) 
    {
        std::cerr << "Socket not open." << std::endl;
        return false;
    }

    int rw = -1;
    if(read_or_write=="r")
    {
        rw = 0;
    }else if(read_or_write=="w")
    {
        rw = 1;
    }else
    {
        return false;
    }

    buffer[0] = rw; //0 is write with readback, 1 is just write
    buffer[1] = 1;

    //Make command from in
    int packet_size = 0;
    if(read_or_write=="w")
    {
        memcpy(&buffer[RX_ADDR_OFFSET_], &addr, 8);
        memcpy(&buffer[RX_DATA_OFFSET_], &value, 8);
        packet_size = RX_DATA_OFFSET_ + 8;
    }else if(read_or_write=="r")
    {
        memcpy(&buffer[RX_ADDR_OFFSET_], &addr, 8);
        packet_size = RX_DATA_OFFSET_;
    }

    int returnval = sendto(m_socket,buffer,packet_size,0, list_of_addresses->ai_addr, list_of_addresses->ai_addrlen);
    if(returnval==-1)
    {
        std::cout << "Error data not send, tried to send " << buffer << std::endl; 
    }

    memset(buffer, 0, sizeof buffer);
    return true;
}


uint64_t Ethernet::RecieveDataSingle(uint64_t addr, uint64_t value) 
{
    if(m_socket<=0) 
    {
        std::cerr << "Socket not open." << std::endl;
        return 0xeeeeaa01;
    }

    int rec_bytes = -1;
	
    if(!SendData(addr,value,"r"))
    {
        std::cout << "Could not send read command" << std::endl;
        return 0xeeeeaa02;
    }

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
    
    tv_ = {1, 250000};  // 0 seconds and 250000 useconds
    int retval = select(m_socket+1, &rfds_, NULL, NULL, &tv_);

    if(retval > 0)
    {
        if((rec_bytes = recvfrom(m_socket,buffer,MAXBUFLEN_ - 1,0,(struct sockaddr*)&their_addr,&addr_len)) == -1)
        {
            perror("recvfrom");
            return 0xeeeeaa03;
        }
    }else if(retval == 0)
    {
        printf("Read Timeout\n");
        return 0xeeeeaa04;
    }else
    {
        perror("select()");
        return 0xeeeeaa05;
    }

    uint64_t data;
    memcpy((void*)&data, (void*)&buffer[TX_DATA_OFFSET_], 8);

    memset(buffer, 0, sizeof buffer);
    return data;
}


std::vector<uint64_t> Ethernet::RecieveBurst(int wanted_number_of_words, int timeout_sec, int timeout_us)
{
    // ---- INITS
    int numbytes = 0; 
    int wordsRead = 0;
    int how_much_to_read = 0;
    bool last_read = false;

    int bytesize = 2;
    int maxbytes = 1456;
    int max_words_to_be_read = maxbytes/bytesize;

    uint64_t functionreturn = 0xeeeebb01; // Default error return

    // ---- Create the adresses to read from
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);

    // ---- Reset buffer
    buffer[0] = 0;

    // ---- Define read size
    wanted_number_of_words += 8/bytesize;
    if(bytesize*wanted_number_of_words>maxbytes)
    {
        how_much_to_read = maxbytes + 2; 
    }else
    {
        how_much_to_read = bytesize*wanted_number_of_words + 2;
    }
    // std::cout << "For the first time I will read " << how_much_to_read << " words" << std::endl;

    std::vector<uint64_t> data(wanted_number_of_words);
    int reduced_number_of_words;// = (wanted_number_of_words/(bytesize/2))+1;
    // std::vector<uint64_t> tmp_data(reduced_number_of_words);
    reduced_number_of_words = wanted_number_of_words;
    // std::cout << "The goal is now " << reduced_number_of_words << std::endl;
    while(wordsRead < reduced_number_of_words)
    {
        tv_ = {timeout_sec, timeout_us};  // 0 seconds and 250000 useconds
        int retval = select(m_socket+1, &rfds_, NULL, NULL, &tv_);
        if(retval > 0)
        {
            if((numbytes = recvfrom(m_socket,buffer,how_much_to_read,0,(struct sockaddr*)&their_addr,&addr_len)) == -1)
            {
                perror("recvfrom");
                functionreturn = 0xeeeebb02;
                break;
            }else
            {
                // std::cout << "Got " << numbytes << " bytes back" << std::endl;
            }

            // ---- Makes sure the buffer is a burst read
            if(!((buffer[0] & 0x7) == 1 || (buffer[0] & 0x7) == 2 || (buffer[0] & 0x7) == 3)) printf("Not burst packet! %x\n", buffer[0]); 

            // ---- Copy all currently stored data in buffer to data
            for(int i = 0; i < (numbytes-2)/bytesize; ++i)
            {
                if(i+wordsRead < wanted_number_of_words)
                {
                    memcpy((void*)(data.data()+i+wordsRead), (void*)&buffer[TX_DATA_OFFSET_ + bytesize*i], bytesize);    
                }else
                {
                    break;
                }
            }

            // ---- Update the amount of words that were already read
            // std::cout << "Read words updated from  " << wordsRead;
            wordsRead += (numbytes-2)/bytesize;
            // std::cout << " to " << wordsRead << std::endl;

            // ---- Checks if the number of bytes that are supposed to be read needs to be updated
            // ---- Avoids timeout if number left is smaller than max
            if(bytesize*(reduced_number_of_words-wordsRead)<maxbytes && reduced_number_of_words-wordsRead!=0) // ---- Case for smaller than max but goal is already reached
            {
                how_much_to_read = (reduced_number_of_words - wordsRead)*bytesize + 2;
                // std::cout << "Setting read to " << (reduced_number_of_words - wordsRead)*2 + 2 << std::endl;
            }else if(reduced_number_of_words-wordsRead==0) // ---- Goal is reached
            {
                // printf("Data successfull %d\n",wordsRead);
                break;
            }else if(reduced_number_of_words-wordsRead<0) // --- WTF
            {
                std::cout << "WTF" << std::endl; 
            }
        }else if(retval == 0) // ---- Case if select times out
        {
            printf("Burst Read Timeout\n");
            functionreturn = 0xeeeebb03;
            break;
        }else // ---- Case if select breaks
        {
            perror("select()");
            functionreturn = 0xeeeebb04;
            break;
        }
    }

    if(data.size()==0)
    {
        data.push_back(functionreturn);
    }else{
        // for(int k=0; k<data.size(); k++)
        // {
        //     // printf("%d - 0x%016llx\n",k,data.at(k));
        //     // if ((k + 1) % 4 == 0) {
        //     // printf("----\n");
        //     // }
        // }
    }

    memset(buffer, 0, sizeof buffer);
    return data;
}


std::vector<uint64_t> Ethernet::RecieveDataVector(uint32_t addr, uint64_t value, int size, uint8_t flags) 
{
    if(m_socket<=0) 
    {
        std::cerr << "Socket not open." << std::endl;
        return {};
    }

    buffer[0] = (flags & NO_ADDR_INC)?0x08:0x00;
    buffer[1] = size;

    //Make command from in
    memcpy(&buffer[RX_ADDR_OFFSET_], &addr, 8);
    memcpy(&buffer[RX_DATA_OFFSET_], &value, 8);

    int packet_size = RX_DATA_OFFSET_ + 8;

    int returnval = sendto(m_socket,buffer,packet_size,0, list_of_addresses->ai_addr, list_of_addresses->ai_addrlen);
    if(returnval==-1)
    {
        std::cout << "Error data not send, tried to send " << buffer << std::endl; 
    }

    if(size==-1)
    {
        size = 16000;
    }

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
    
    tv_ = {0, 250000};  // 0 seconds and 250000 useconds
    int retval = select(m_socket+1, &rfds_, NULL, NULL, &tv_);

    while(true){
        int rec_bytes=-1;
        if(retval > 0)
        {
            if((rec_bytes = recvfrom(m_socket,buffer,MAXBUFLEN_ - 1,0,(struct sockaddr*)&their_addr,&addr_len)) == -1)
            {
                perror("recvfrom");
                return {406};
            }
	    }else if(retval == 0)
        {
            printf("Read Timeout\n");
            return {405};
        }else
        {
            perror("select()");
            return {404};
        }

        std::vector<uint64_t> data((rec_bytes-2)/8);
        memcpy((void*)data.data(), (void*)&buffer[TX_DATA_OFFSET_], rec_bytes - 2);

        cout<<"Data size: "<<data.size()<<" words"<<endl;
        memset(buffer, 0, sizeof buffer);
        if(data.size()==size)
        {
            return data;
        }
    }

	vector<uint64_t> data;

    return data;
}