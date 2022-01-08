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
    int timer = 5;
    int congState = 1;
};
//Global Variables
struct clientState state;
int readyPackets = state.w;
int duplicateAcks = 0;
int currentProcesses = 0;
bool stop = false;
int prop;
int fileLength;
char fileName[MAXLINE];
queue<int> nonAckPackets;
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
    queue<int> copiedPackets(nonAckPackets);

    currentProcesses = currentProcesses + 1;

    while (!copiedPackets.empty())
    {
        if (terminateThread)
            std::terminate();
        int seq = copiedPackets.front();
        struct packet packet;
        int start = seq * 500;
        int end = start + 500;
        if (end > fileLength)
            end = fileLength;
        packet_init(&packet, start, end, fileName, seq);
        copiedPackets.pop();
        sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
    }
    stop = false;
}
void sendPackets(int sockfd, int nOfPackets, int fileLength, char fileName[], struct sockaddr_in cliaddr)
{
    cout << "CHIIILLLD\n";
    currentProcesses = currentProcesses + 1;
    int start = 0, end = 500, counter = 0;

    if (fileLength < end)
        end = fileLength - 1;

    while (counter < nOfPackets || !nonAckPackets.empty())
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
                end = fileLength - 1;
            nonAckPackets.push(packet.seqno);
            cout << "I am seq : " << packet.seqno << "\n";
            cout << "I am ready packets : " << readyPackets << "\n";
            cout << "start " << start << "\n";
            cout << "end " << end << "\n";
            bool TrueFalse = (rand() % 100) > prop;
            if (TrueFalse)
            {
                sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
                cout << "SENT\n";
            }
        }
        else if (stop == false)
        {
            //Time out
            cout << "Ready Packets is zero, WAITING STATE\n";
            sleep(state.timer);
            if (readyPackets == 0)
            {
                int seq = nonAckPackets.front();
                counter = seq;
                start = seq * 500;
                end = start + 500;
                queue<int> empty;
                swap(nonAckPackets, empty);
                state.congState = 1;
                state.ssthresh = state.w / 2;
                state.w = 1;
                state.timer = 10;
                readyPackets = state.w;
                cout << "RESET\n";
            }
        }
    }
}

int main()
{
    //reading from the file

    ifstream requestFile("server.in");
    string temp;

    getline(requestFile, temp);
    in_port_t port_number = atoi(temp.c_str());

    getline(requestFile, temp);
    unsigned int seed = atoi(temp.c_str());
    srand(seed);

    getline(requestFile, temp);
    prop = atoi(temp.c_str());

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

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
    servaddr.sin_port = htons(port_number);

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
    fileLength = getFileLength(fileName);
    int nOfPackets = ceil(fileLength / 500.0);
    cout << "i am file name :";
    cout << fileName;
    cout << "\n";
    cout << "The packets length is: " << nOfPackets << "\n";

    std::thread send_thread(sendPackets, sockfd, nOfPackets, fileLength, fileName, cliaddr);

    while (1)
    {
        struct ack_packet ack;
        n = recvfrom(sockfd, (struct ack_packet *)&ack, sizeof(ack),
                     MSG_WAITALL, (struct sockaddr *)&servaddr,
                     (socklen_t *)&len);
        // On Ack Recieve
        if (n != 0)
        {
            // Check Duplicate Acks
            int firstPacket = nonAckPackets.front();

            if (firstPacket == ack.ackno)
            {
                cout << "STATE 1 : Ack is sent Successfully!\n";
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
                    cout << "CONGESTION AVOIDANCE\n";
                    readyPackets = state.w + readyPackets + 1;
                    state.w = state.w * 2;
                }
                else
                {
                    cout << "SLOW START\n";
                    readyPackets = readyPackets + 2;
                    state.w = state.w + 1; //Update window size
                }
                state.timer = 10 * state.w;
            }
            //Delay in ack from Client
            else if (firstPacket - ack.ackno > 1)
            {
                cout << "STATE 2: Delay in Ack from Client\n";
                while (nonAckPackets.front() != ack.ackno)
                    nonAckPackets.pop();
            }
            else
            {
                cout << "STATE 3: Duplicate Acks\n";
                //if 3 duplicate acks : resend from queue
                if (duplicateAcks == 3)
                {
                    cout << "THREE DUPLICATE ACK\n";
                    duplicateAcks = 0;
                    //Check if 3 duplicate acks for the first time?
                    if (currentProcesses == 2)
                        terminateThread = true;
                    std::thread resend_thread(resendPackets, sockfd, cliaddr);
                    state.ssthresh = state.w / 2;
                    state.congState = 1;
                    terminateThread = false;
                }
                else
                {
                    duplicateAcks += 1;
                }
            }
        }
        cout << "i am ack number " << ack.ackno << "\n";
        //Check file end
        if (ack.ackno == nOfPackets - 1)
        {
            cout << "END OF FILE\n";
            struct packet packet;
            packet_end(&packet);
            sendto(sockfd, (struct packet *)&packet, sizeof(packet),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
            break;
        }
    }
    close(sockfd);
    return 0;
}
