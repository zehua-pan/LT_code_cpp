#ifndef RECEIVER_H
#define RECEIVER_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "globalParameters.h"

class Receiver 
{
    public:
        // constructor
        Receiver();
        ~Receiver() {close(sockfd);}
        // member functions
        symbol* getSymbol();
        void terminate();
    private:
        // data member 
        int sockfd;
        char buffer[SER_BUF_SIZE];
        struct sockaddr_in servaddr, cliaddr;
};

Receiver::Receiver()
{
    // creating socket file descriptor
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw runtime_error("server : socket creation failed");

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
    // bind the socket with the server address
    if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        throw runtime_error("bind failed");
}

inline
symbol* Receiver::getSymbol()
{
    recvfrom(sockfd, buffer, SER_BUF_SIZE, MSG_WAITALL, (struct sockaddr*)&cliaddr, &len);
    symbol* sym = reinterpret_cast<symbol*>(buffer);
    return sym;
}

void Receiver::terminate()
{
    // set timeout for recvfrom 
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tvY, sizeof(tvY)) < 0)
        throw runtime_error("setsockopt: set timeout error");

    sendto(sockfd, terminalMsg.c_str(), terminalMsg.size(), MSG_CONFIRM, (const struct sockaddr*)&cliaddr, len);
    while(recvfrom(sockfd, buffer, SER_BUF_SIZE, MSG_WAITALL, (struct sockaddr*)&cliaddr, &len) > 0)
    {
        sendto(sockfd, terminalMsg.c_str(), terminalMsg.size(), MSG_CONFIRM, (const struct sockaddr*)&cliaddr, len);
    }
    cout << "no more data receive from client, one iteration ends" << endl;

    // reset for next recvfrom
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tvN, sizeof(tvN)) < 0)
        throw runtime_error("setsockopt: set no timeout error");
}

#endif
