#ifndef SENDER_H
#define SENDER_H

#include "globalParameters.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

class Sender 
{
    public:
        // constructor
        Sender(int );
        ~Sender() {close(sockfd);}
        // member functions
        int send(const symbol&); // send symbols, 
    private:
        // data member
        long numberThreshold;
        long checkInterval;
        int sockfd;
        char buffer[CLI_BUF_SIZE];
        struct sockaddr_in servaddr, cliaddr;
};

Sender::Sender(long nTh) : numberThreshold(nTh), sentSymN(0)
{
    checkInterval = (long)((double)numberThreshold * 0.1) + 1;
    // create socket 
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::runtime_error("client : socket creation failed");

    memset(&servaddr, 0, sizeof(servaddr));

    // fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // set timeout for recvfrom
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        throw std::runtime_error("setsockopt error");
}

inline
void Sender::send(const symbol& sym)
{
    socklen_t len;
    const char *symP = reinterpret_cast<const char *>(&sym);
    sendto(sockfd, symP, sizeof(sym), MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));
}

inline
bool Sender::checkEndMsg()
{
    return recvfrom(sockfd, buffer, CLI_BUF_SIZE, MSG_WAITALL, (struct sockaddr*)&servaddr, &len) > 0 && *buffer == 'e';
}

#endif
