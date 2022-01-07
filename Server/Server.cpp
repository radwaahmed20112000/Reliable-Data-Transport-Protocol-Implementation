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

//1 for Slow start (2*)
//2 for Congestion Avoidance (2+)
struct clientState
{
    int w = 1;
    int ssthresh = 64;
    int timer = 10;
    int congState = 1;
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
    n = recvfrom(sockfd, (char *)fileName, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, (socklen_t *)&len);

    // cout << "i am n " << n;
    // cout << "\n";
    int nOfPackets = ceil(getFileLength(fileName) / 500.0);
    cout << "i am file name :";
    cout << fileName;
    cout << "\n";
    // cout << "The packets length is: " << nOfPackets << "\n";
    //GLOBALS
    struct clientState state;
    int readyPackets = state.w;
    int counter = 0;
    int duplicateAcks = 0;
    int currentProcesses = 0;
    pid_t c_duplicate;
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
        currentProcesses++;
        //TODO UPDATE : END
        int start = 0, end = 5;
        while (counter < nOfPackets)
        { //time_out
            //Semaphore wait()
            if (readyPackets > 0)
            {
                struct packet packet;
                packet_init(&packet, start, end, fileName, counter);
                start += 500;
                end += 500;
                counter++;
                readyPackets--;
                nonAckPackets.push(packet);
                sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
            }
            else
            {
                //Time out
                sleep(state.timer);
                if (readyPackets == 0)
                {
                    int seq = nonAckPackets.front().seqno;
                    counter = seq;
                    start = seq * 500;
                    end = start + 500;
                    queue<struct packet> empty;
                    swap(nonAckPackets, empty);
                    state.congState = 1;
                    state.ssthresh = state.w / 2;
                    state.w = 1;
                    state.timer = 10;
                    readyPackets = state.w;
                }
            }
        }
        // exit(0);
    }
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
            // Check Duplicate Acks
            struct packet firstPacket = nonAckPackets.front();
            if (firstPacket.seqno == ack.ackno)
            {
                nonAckPackets.pop();
                // Check if it was slow start
                if (state.w >= state.ssthresh)
                {
                    state.congState = 2;          //Update State Congestion Avoidance
                    state.ssthresh = state.w / 2; //Update threshold
                }
                //if it is slow start
                if (state.congState == 1)
                {
                    readyPackets = state.w + readyPackets + 1;
                    state.w = state.w * 2;
                }
                else
                {
                    readyPackets = readyPackets + 2;
                    state.w = state.w + 1; //Update window size
                }
                state.timer = 10 * state.w;
            }
            else if (firstPacket.seqno < ack.ackno)
            {
                while (nonAckPackets.front().seqno != ack.ackno)
                    nonAckPackets.pop();
            }
            else
            {
                //if 3 duplicate acks : resend from queue
                if (duplicateAcks == 3)
                {
                    duplicateAcks = 0;
                    //Check if 3 duplicate acks for the first time?
                    if (currentProcesses == 2)
                        kill(c_duplicate, SIGKILL);

                    kill(c_pid, SIGSTOP);
                    queue<struct packet> copiedPackets(nonAckPackets);
                    c_duplicate = fork();

                    if (c_duplicate < 0)
                    {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                    else if (c_duplicate > 0)
                    {
                        currentProcesses++;
                        while (!copiedPackets.empty())
                        {
                            struct packet p = copiedPackets.front();
                            copiedPackets.pop();
                            sendto(sockfd, (struct packet *)&p, sizeof(p), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                        }
                    }
                    kill(c_pid, SIGCONT);
                    state.ssthresh = state.w / 2;
                    state.congState = 1;
                }
                else
                    duplicateAcks++;
            }
        }
        cout << "i am ack number " << ack.ackno;
        cout << "\n";

        // return 0;
    }
}
