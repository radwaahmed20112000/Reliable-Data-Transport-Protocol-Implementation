// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../Packet/Packet.h"
#include "../Packet/ack-packet.h"
#include <stdint.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include <queue>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <stdbool.h>
using std::cout;
using std::endl;
using namespace std;

#define PORT 2300
#define MAXLINE 1024

//1 for Slow start (2*)
//2 for Congestion Avoidance (2+)
struct clientState
{
    int w = 1;
    int timer = 2;
};
struct clientState state;
int readyPackets = state.w;
queue<struct packet> nonAckPackets;

int getFileLength(std::string fileName)
{
    std::ifstream is(fileName, std::ifstream::binary);
    int length = 0;
    if (is)
    {
        // get length of file:
        is.seekg(0, is.end);
        length = is.tellg();
        is.seekg(0, is.beg);
        is.close();
    }
    return length;
}

void sendPackets(int sockfd, int nOfPackets, int fileLength, char fileName[], struct sockaddr_in cliaddr)
{
    cout << "CHIIILLLD\n";
    int start = 0, end = 500, counter = 0;

    if (fileLength < end)
        end = fileLength;

    do
    {
        //Semaphore wait()
        cout << "READY ";
        if (readyPackets > 0)
        {
            struct packet packet;
            packet_init(&packet, start, end, fileName, counter);
            start += 500;
            end += 500;
            counter += 1;
            readyPackets -= 1;
            if (fileLength < end)
                end = fileLength;
            cout << "aloooo " << counter << "\n";

            nonAckPackets.push(packet);
            cout << "I am seq : " << packet.seqno << "\n";
            cout << "start " << start << "\n";
            cout << "end " << end << "\n";
            // bool TrueFalse = (rand() % 100) < 75;

            sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
            cout << "SENT\n";
        }
        else
        {
            //Time out
            cout << "TIME OUT\n";
            sleep(state.timer);
            cout << readyPackets << " READY\n ";
            cout << counter << " counter\n ";
            if (readyPackets == 0)
            {
                int seq = nonAckPackets.front().seqno;
                counter = seq;
                start = seq * 500;
                end = start + 500;
                queue<struct packet> empty;
                swap(nonAckPackets, empty);
                state.w = 1;
                state.timer = 10;
                readyPackets = state.w;
                cout << "RESET\n";
            }
            cout << "HHH\n";
        }
        cout << "else\n";
    } while (counter < nOfPackets);
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char fileName[MAXLINE];

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int len, n;

    len = sizeof(cliaddr); //len is value/resuslt
    cout << "server is working";
    cout << "\n";
    n = recvfrom(sockfd, (char *)fileName, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, (socklen_t *)&len);

    int fileLength = getFileLength(fileName);
    int nOfPackets = ceil(fileLength / 500.0);
    cout << "i am file name :";
    cout << fileName;
    cout << "\n";
    cout << "The packets length is: " << nOfPackets << "\n";

    std::thread send_thread(sendPackets, sockfd, nOfPackets, fileLength, fileName, cliaddr);

    while (1)
    {
        struct ack_packet ack;
        // sleep(3);
        n = recvfrom(sockfd, (struct ack_packet *)&ack, sizeof(ack),
                     MSG_WAITALL, (struct sockaddr *)&servaddr,
                     (socklen_t *)&len);
        // On Ack Recieve
        if (n != 0)
        {
            struct packet firstPacket = nonAckPackets.front();
            if (firstPacket.seqno == ack.ackno)
            {
                cout << "STATE 1\n";
                readyPackets += 1;
                nonAckPackets.pop();
            }
        }
        cout << "i am ack number " << ack.ackno;
        cout << "\n";
        if (ack.ackno == nOfPackets - 1)
        {
            cout << "BREAK";
            cout << "END OF FILLELLEEELELLELELEL\n";
            struct packet packet;
            packet_end(&packet);

            sendto(sockfd, (struct packet *)&packet, sizeof(packet),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
            break;
        }
        // return 0;
    }
    close(sockfd);
    return 0;
}
