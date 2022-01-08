#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include "chrono"
#include "../Packet/Packet.h"
#include "../Packet/ack-packet.h"
#define MAXLINE 1024

using namespace std;

void write_file(char *data)
{
    int rcv_bytes = strlen(data);
    string fileName = "rcv.txt";

    FILE *file_pointer = fopen(fileName.c_str(), "a");
    if (file_pointer == NULL)
    {
        printf("File Cannot be opened.\n");
        exit(EXIT_FAILURE);
    }

    // printf("%s\n", data);
    if ((fwrite(data, sizeof(char), rcv_bytes, file_pointer)) < rcv_bytes)
    {
        perror("File write failed! ");
        exit(EXIT_FAILURE);
    }

    fclose(file_pointer);
}

// Driver code
int main()
{
    struct ack_packet buffer[MAXLINE];
    int n;
    socklen_t len;
    uint32_t waiting_seqno = 0;
    uint32_t last_recieved_ack;
    struct packet response;
    int client_socket;
    struct sockaddr_in server_addr;
    bool file_ack = false;
    bool accepted = true;

    //reading from the file

    ifstream requestFile("client.in");
    string temp;
    getline(requestFile, temp);
    const char *IP = temp.c_str();

    getline(requestFile, temp);
    in_port_t port_number = atoi(temp.c_str());

    getline(requestFile, temp);
    const char *fileName = temp.c_str();

    requestFile.close();

    // Creating socket file descriptor
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("Error in socket");
        exit(1);
    }
    printf("Client socket created successfully.\n");

    // Filling server information
    server_addr.sin_family = AF_INET;
    // int rtnVal = inet_pton(AF_INET, IP, &server_addr.sin_addr);
    // if (rtnVal == 0)
    // {
    //     perror("inet_pton() failed, invalid address string\n");
    //     exit(1);
    // }
    // else if (rtnVal < 0)
    // {
    //     perror("inet_pton() failed\n");
    //     exit(1);
    // }
    server_addr.sin_port = htons(port_number);

    // char file[fileName.size() + 1];
    // strcpy(file, fileName.c_str());
    // struct packet file_datagram;
    // packet(&file_datagram, 0);
    // file_datagram.data = fileName.c_str();

    // sends the file name and waits for an ack
    // while (!file_ack)
    // {

    sendto(client_socket, (const char *)fileName, sizeof(fileName), MSG_CONFIRM, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    cout << "file name is sent.\n";
    cout << "HIIIII FILELLELELELE:";
    cout << fileName << "\n";

    //     time_t start = time(0);
    //     double diff;
    //     while (difftime(time(0), start) < 6 && !n)
    //     {
    //         n = recvfrom(client_socket, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&server_addr, &len);
    //     }

    //     if (n)
    //     {
    //         file_ack = true;
    //     }

    //     cout << "Server : %s\n"
    //          << buffer << endl;
    // }

    while (1)
    {

        n = recvfrom(client_socket, (struct packet *)&response, sizeof(response), MSG_WAITALL, (struct sockaddr *)&server_addr, &len);
        cout << "\n";
        cout << "client :" << response.seqno << "\n";

        //check the end of the file
        if (response.seqno == -1)
        {
            break;
        }
        cout << response.data;
        // recieving the wanted packet
        if (response.seqno == waiting_seqno)
        {
            struct ack_packet new_ack;
            ack_packet_init(&new_ack, response.seqno);
            sendto(client_socket, (struct ack_packet *)&new_ack, sizeof(new_ack), MSG_CONFIRM, (const struct sockaddr *)&server_addr, sizeof(server_addr));
            // cout << "client : %d\n"
            //      << new_ack.ackno << endl;

            write_file(response.data);

            waiting_seqno++;
            last_recieved_ack = new_ack.ackno;
        }

        // recieving not in order packet
        else
        {
            struct ack_packet new_ack;
            ack_packet_init(&new_ack, last_recieved_ack);

            sendto(client_socket, (struct ack_packet *)&new_ack, sizeof(new_ack), MSG_CONFIRM, (const struct sockaddr *)&server_addr, sizeof(server_addr));
            // cout << "client : "
            //      << new_ack.ackno << "\n";
        }
    }

    close(client_socket);
    return 0;
}
