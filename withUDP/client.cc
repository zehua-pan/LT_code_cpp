#include <iostream>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

using namespace std;

const int PACKET_SIZE = 32768;
const int DISTRIBUTION_RANGE = 25;
const int PORT = 8080;
const size_t CLI_BUF_SIZE = 128;
const string terminalMsg("end");

struct symbol
{
    symbol(int i, int d, int f): symbolID(i), degree(d), filesize(f){}
    int symbolID;
    int degree;
    long filesize;
    int neighbours[DISTRIBUTION_RANGE];
    char data[PACKET_SIZE];
};

struct timeval tv = {0, 100 * 1000};


int main()
{
    int sockfd;
    char buffer[CLI_BUF_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    // create socket 
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw runtime_error("client : socket creation failed");

    memset(&servaddr, 0, sizeof(servaddr));

    // fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    int n, sentSymN = 0;
    socklen_t len;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        throw runtime_error("setsockopt error");
    while(1)
    {
        symbol sym(0, sentSymN, 0);
        const char *symP = reinterpret_cast<const char *>(&sym);
        sendto(sockfd, symP, sizeof(sym), MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));
        cout << "sent " << sentSymN << " symbols" << endl;
        if(++sentSymN >= 210 && sentSymN % 10 == 0)
        {
            cout << "have sent " << sentSymN << " symbols" << endl;
            cout << "try to detect terminal message from server" << endl;
            if(recvfrom(sockfd, buffer, CLI_BUF_SIZE, MSG_WAITALL, (struct sockaddr*)&servaddr, &len) > 0 && *buffer == 'e')
            {
                cout << "receive end message from server, end this iteration" << endl;
                break;
            }
        }
        usleep(50 * 1000);
    }
    close(sockfd);
    return 0;
}
