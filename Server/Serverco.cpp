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
    int ssthresh = 64;
    int timer = 2;
    int congState = 1;
};
struct clientState state;
int readyPackets = state.w;
int duplicateAcks = 0;
int currentProcesses = 0;
bool stop = false;
queue<struct packet> nonAckPackets;
bool terminateThread = false;

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
void resendPackets(int sockfd, struct sockaddr_in cliaddr)
{
    stop = true;
    queue<struct packet> copiedPackets(nonAckPackets);
    // c_duplicate = fork();

    // if (c_duplicate < 0)
    // {
    //     perror("fork");
    //     exit(EXIT_FAILURE);
    // }
    // else if (c_duplicate > 0)
    // {
    currentProcesses = currentProcesses + 1;

    while (!copiedPackets.empty())
    {
        if (terminateThread)
            std::terminate();
        struct packet p = copiedPackets.front();
        copiedPackets.pop();
        sendto(sockfd, (struct packet *)&p, sizeof(p), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
    }
    stop = false;
}
void sendPackets(int sockfd, int nOfPackets, int fileLength, char fileName[], struct sockaddr_in cliaddr)
{
    cout << "CHIIILLLD\n";
    currentProcesses = currentProcesses + 1;
    int start = 0, end = 500, counter = 0;

    if (fileLength < end)
        end = fileLength;

    do
    {
        //Semaphore wait()
        if (stop == false && readyPackets > 0)
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
        else if (stop == false)
        {
            //Time out
            cout << "TIME OUT\n";
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
                cout << "RESET\n";
            }
        }
    } while (counter < nOfPackets && !nonAckPackets.empty());
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

    // cout << "i am n " << n;
    // cout << "\n";
    int fileLength = getFileLength(fileName);
    int nOfPackets = ceil(fileLength / 500.0);
    cout << "i am file name :";
    cout << fileName;
    cout << "\n";
    cout << "The packets length is: " << nOfPackets << "\n";
    //GLOBALS

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
            // Check Duplicate Acks
            struct packet firstPacket = nonAckPackets.front();
            if (firstPacket.seqno == ack.ackno)
            {
                cout << "STATE 1\n";
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
            //Delay in ack from Client
            else if (firstPacket.seqno - ack.ackno > 1)
            {
                cout << "STATE 2\n";
                while (nonAckPackets.front().seqno != ack.ackno)
                    nonAckPackets.pop();
            }
            else
            {
                cout << "STATE 3\n";
                //if 3 duplicate acks : resend from queue
                if (duplicateAcks == 3)
                {
                    duplicateAcks = 0;
                    //Check if 3 duplicate acks for the first time?
                    if (currentProcesses == 2)
                        terminateThread = true;
                    //     kill(c_duplicate, SIGKILL);
                    // kill(c_pid, SIGSTOP);
                    std::thread resend_thread(resendPackets, sockfd, cliaddr);
                    // }
                    // kill(c_pid, SIGCONT);
                    state.ssthresh = state.w / 2;
                    state.congState = 1;
                    terminateThread = false;
                }
                else
                    duplicateAcks = duplicateAcks + 1;
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
