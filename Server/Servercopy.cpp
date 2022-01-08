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
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/msg.h>

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
// Shared Memory
static int *readyPackets;
static int *currentProcesses = 0;
static int *sendPid;
static struct clientState *state;
static struct packet *pack;
static queue<struct packet> nonAckPackets;
key_t key;
int msgid;
uint32_t x = 0;

// Parent Process Variables
int duplicateAcks = 0;
pid_t c_duplicate;

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
    // key_t key;
    // int msgid;

    // // ftok to generate unique key
    // key = ftok("progfile", 65);

    // // msgget creates a message queue
    // // and returns identifier
    // msgid = msgget(key, 0666 | IPC_CREAT);
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

    int fileLength = getFileLength(fileName);
    int nOfPackets = ceil(fileLength / 500.0);
    cout << "i am file name :";
    cout << fileName;
    cout << "\n";
    cout << "The packets length is: " << nOfPackets << "\n";
    //GLOBALS
    readyPackets = (int *)mmap(NULL, sizeof *readyPackets, PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    currentProcesses = (int *)mmap(NULL, sizeof *currentProcesses, PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    state = (struct clientState *)mmap(NULL, sizeof *state, PROT_READ | PROT_WRITE,
                                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    pack = (struct packet *)mmap(NULL, sizeof *pack, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    *readyPackets = (*state).w;
    (*state).w = 1;
    (*state).ssthresh = 64;
    (*state).timer = 10;
    (*state).congState = 1;
    //Fork New Process
    pid_t c_pid = fork();
    cout << c_pid << " PID\n";
    // cout << "I am pid :" << c_pid << "\n";
    if (c_pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (c_pid > 0)
    {
        *sendPid = c_pid;
        cout << "CHIIILLLD\n";
        *currentProcesses = *currentProcesses + 1;
        int start = 0, end = 500, counter = 0;

        if (fileLength < end)
            end = fileLength;
        while (counter < nOfPackets)
        { //time_out
            //Semaphore wait()
            cout << "I AM WINDOWWWW " << (*state).w << "\n";
            if (*readyPackets > 0)
            {
                struct packet packet;
                packet_init(&packet, start, end, fileName, counter);
                start += 500;
                end += 500;
                counter += 1;
                *readyPackets -= 1;
                if (fileLength < end)
                    end = fileLength;
                cout << "aloooo " << counter << "\n";

                // msgsnd(msgid, &packet, sizeof(packet), 0);
                // nonAckPackets.push(packet);
                *pack = packet;
                cout << "I am seq : " << packet.seqno << "\n";
                cout << "start " << start << "\n";
                cout << "end " << end << "\n";
                sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
            }
            else
            {
                //Time out
                sleep((*state).timer);
                if (*readyPackets == 0)
                {
                    int seq = nonAckPackets.front().seqno;
                    counter = seq;
                    start = seq * 500;
                    end = start + 500;
                    queue<struct packet> empty;
                    swap(nonAckPackets, empty);
                    (*state).congState = 1;
                    (*state).ssthresh = (*state).w / 2;
                    (*state).w = 1;
                    (*state).timer = 10;
                    *readyPackets = (*state).w;
                }
            }
        }
        cout << "END OF FILLELLEEELELLELELEL\n";
        struct packet packet;
        packet_end(&packet);
        sendto(sockfd, (struct packet *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);

        // exit(0);
    }
    while (1)
    {
        // struct packet p;
        // msgrcv(msgid, &p, sizeof(p), 1, 0);
        // cout << p.seqno << "ANA PRINTEEEEEEEEDDDDDDDDDDDDDDD\n";
        struct ack_packet ack;
        n = recvfrom(sockfd, (struct ack_packet *)&ack, sizeof(ack),
                     MSG_WAITALL, (struct sockaddr *)&servaddr,
                     (socklen_t *)&len);

        // On Ack Recieve
        if (n != 0)
        {
            cout << ack.ackno << " IAM AACKCKCKCK \n";
            cout << "I AM N " << n
                 << "\n";

            // Check Duplicate Acks
            struct packet firstPacket = nonAckPackets.front();
            cout << firstPacket.seqno << " SEQ NO \n";
            if (x == ack.ackno)
            {
                x += 1;
                cout << "IAM HERRREEEE COMPARE zzz\n";
                nonAckPackets.pop();
                // Check if it was slow start
                if ((*state).w >= (*state).ssthresh)
                {
                    (*state).congState = 2;             //Update State Congestion Avoidance
                    (*state).ssthresh = (*state).w / 2; //Update threshold
                }
                //if it is slow start
                if ((*state).congState == 1)
                {
                    // kill(*sendPid, SIGSTOP);

                    *readyPackets = (*state).w + *readyPackets + 1;
                    (*state).w = (*state).w * 2;

                    // kill(*sendPid, SIGCONT);
                }
                else
                {
                    // kill(*sendPid, SIGSTOP);
                    // sleep(1);
                    cout << *readyPackets << " ready\n";
                    *readyPackets = *readyPackets + 2;
                    // kill(*sendPid, SIGCONT);
                    (*state).w = (*state).w + 1; //Update window size
                }
                (*state).timer = 10 * (*state).w;
            }
            //Delay in ack from Client
            else if ((x - ack.ackno) > 1)
            {
                cout << "STATE 2\n";
                // cout << (firstPacket.seqno - ack.ackno);
                cout << ack.ackno << " \n";

                while (nonAckPackets.front().seqno != ack.ackno)
                    nonAckPackets.pop();
            }
            else
            {
                //if 3 duplicate acks : resend from queue
                cout << "HEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n";
                if (duplicateAcks == 3)
                {
                    cout << "IAM HERRREEEE 333333333333\n";
                    duplicateAcks = 0;
                    //Check if 3 duplicate acks for the first time?
                    if (*currentProcesses == 2)
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
                        *currentProcesses = *currentProcesses + 1;
                        while (!copiedPackets.empty())
                        {
                            struct packet p = copiedPackets.front();
                            copiedPackets.pop();
                            sendto(sockfd, (struct packet *)&p, sizeof(p), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                        }
                    }
                    kill(c_pid, SIGCONT);
                    ((*state)).ssthresh = (*state).w / 2;
                    (*state).congState = 1;
                }
                else
                {
                    cout << "IAM HERRREEEE ALOOOOOOOO\n";
                    duplicateAcks = duplicateAcks + 1;
                }
            }
        }
        if (ack.ackno == nOfPackets - 1)
        {
            cout << "BREAK";
            break;
        }
        // return 0;
    }
    close(sockfd);
    return 0;
}
