#ifndef _ETHERNET_H_INCLUDED
#define _ETHERNET_H_INCLUDED

#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <string>
#include <vector>

using namespace std;

#define RX_DATA_OFFSET_ 10
#define RX_ADDR_OFFSET_ 2
#define TX_DATA_OFFSET_ 2
#define MAXBUFLEN_ 1500

class Ethernet
{
    private:
        int m_socket;
        struct addrinfo *servinfo, *list_of_addresses;
        unsigned char buffer[MAXBUFLEN_];
        fd_set rfds_;
    	struct timeval tv_;
    	int packetID_;

    public:
        Ethernet(std::string ipaddr, std::string port, int verbose=0);
        ~Ethernet();

        enum
        {
            DO_NOT_RETURN = 0x02,
            REQUEST_ACK   = 0x04,
            NO_ADDR_INC   = 0x08
        };

        bool OpenInterface(std::string ipaddr, std::string port);
        void CloseInterface();

        void SwitchToBurst();
        void SetBurstState(bool state);

        bool SendData(uint64_t addr, uint64_t value, std::string read_or_write="w");

        uint64_t RecieveDataSingle(uint64_t addr, uint64_t value=0x0);
        std::vector<uint64_t> RecieveBurst(int numwords, int timeout_sec = 65, int timeout_us = 0);

        std::vector<uint64_t> RecieveDataVector(uint32_t addr, uint64_t value, int size=-1, uint8_t flags = 0);

};

#endif
