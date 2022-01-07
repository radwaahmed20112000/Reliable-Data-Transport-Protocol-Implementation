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

using std::cout;
using std::endl;
using namespace std;

#define PORT 8000
#define MAXLINE 1024

struct clientState
{
    int w = 1;
    int ssthresh = 64;
    int timer = 10;
};

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
    printf("server is working");
    cout << "\n";

    while (1)
    {
        int choice = 0;
        // sleep(3);
        recvfrom(sockfd,
                 &choice, sizeof(choice), MSG_WAITALL, (struct sockaddr *)&servaddr,
                 (socklen_t *)&len);
        cout << "i am choice " << choice;
        cout << "\n";
        if (choice == 1)
        {
            // sleep(3);
            n = recvfrom(sockfd, (char *)fileName, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, (socklen_t *)&len);

            // cout << "i am n " << n;
            // cout << "\n";
            int nOfPackets = ceil(getFileLength(fileName) / 500.0);
            cout << "i am file name :";
            cout << fileName;
            cout << "\n";
            // cout << "The packets length is: " << nOfPackets << "\n";
            struct clientState state;
            int readyPackets = state.w;
            int counter = nOfPackets;
            queue<struct packet> nonAckPackets;
            //Fork New Process
            pid_t c_pid = fork();
            // cout << "I am pid :" << c_pid << "\n";
            if (c_pid < 0)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (c_pid > 0)
            {
                cout << "CHIIILLLD\n";
                int start = 0, end = 5;
                while (counter != 0)
                { //time_out
                    if (readyPackets > 0)
                    {
                        struct packet packet;
                        packet_init(&packet, start, end, fileName, counter);
                        start += 500;
                        end += 500;
                        counter--;
                        readyPackets--;
                        nonAckPackets.push(packet);
                        sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                    }
                }
                choice = -1;
                // exit(0);
            }
        }
        else if (choice == 2)
        {
            struct ack_packet ack;
            // sleep(3);
            n = recvfrom(sockfd, (struct ack_packet *)&ack, sizeof(ack),
                         MSG_WAITALL, (struct sockaddr *)&servaddr,
                         (socklen_t *)&len);
            cout << "i am ack number " << ack.ackno;
            cout << "\n";
            pid_t c_pid = fork();
            // cout << "I am pid :" << c_pid << "\n";
            if (c_pid < 0)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (c_pid > 0)
            {
                cout << "CHILD\n";
                //TODO
            }
        }
        // sleep(3);

        // return 0;
    }
}
