#include <iostream>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

using namespace std;

const int PACKET_SIZE = 32768;
const int DISTRIBUTION_RANGE = 25;

struct symbol
{
    symbol(int i, int d, int f): symbolID(i), degree(d), filesize(f){}
    int symbolID;
    int degree;
    long filesize;
    int neighbours[DISTRIBUTION_RANGE];
    char data[PACKET_SIZE];
};

struct timeval tvY = {0, 100 * 1000};
struct timeval tvN = {0, 0};

const int PORT = 8080;
const size_t OFFSET = 128;
const size_t SER_BUF_SIZE = sizeof(symbol);
const string terminalMsg("end");

int main()
{
    int sockfd;
    char buffer[SER_BUF_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    // creating socket file descriptor
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw runtime_error("server : socket creation failed");

    memset(&servaddr, 0, sizeof(servaddr));

    // fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
    // bind the socket with the server address
    if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        throw runtime_error("bind failed");

    int n, receivedSymN;
    socklen_t len = sizeof(cliaddr);
    while(1)
    {
        recvfrom(sockfd, buffer, SER_BUF_SIZE, MSG_WAITALL, (struct sockaddr*)&cliaddr, &len);
        symbol* sym = reinterpret_cast<symbol*>(buffer);
        cout << "symbol degree : " << sym->degree << endl;
        if(++receivedSymN >= 100)
        {
            cout << "receive " << receivedSymN << " symbols" << endl;
            cout << "sending terminal message to client" << endl;
            sendto(sockfd, terminalMsg.c_str(), terminalMsg.size(), MSG_CONFIRM, (const struct sockaddr*)&cliaddr, len);
            // set timeout for recvfrom 
            if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tvY, sizeof(tvY)) < 0)
                throw runtime_error("setsockopt: set timeout error");
            while(recvfrom(sockfd, buffer, SER_BUF_SIZE, MSG_WAITALL, (struct sockaddr*)&cliaddr, &len) > 0)
            {
                sendto(sockfd, terminalMsg.c_str(), terminalMsg.size(), MSG_CONFIRM, (const struct sockaddr*)&cliaddr, len);
            }
            cout << "no more data receive from client, one iteration ends" << endl;
            receivedSymN = 0;
            // reset, recvfrom should not time out until next iteration
            if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tvN, sizeof(tvN)) < 0)
                throw runtime_error("setsockopt: set no timeout error");
            memset(buffer, 0, sizeof(buffer));
        }
    }
    close(sockfd);
    return 0;
}



