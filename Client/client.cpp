// Client side implementation of UDP client-server model

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
#include <iostream>

using std::cout;

#define PORT 8000
#define MAXLINE 1024

// Driver code
int main()
{
    int sockfd;
    char buffer[MAXLINE];
    char *hello = "adv.txt";
    struct sockaddr_in servaddr;
    struct packet packet;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n, len;

    sendto(sockfd, (const char *)hello, sizeof(hello),
           MSG_CONFIRM, (const struct sockaddr *)&servaddr,
           sizeof(servaddr));
    printf("Hello message sent.\n");

    struct ack_packet ack2;
    ack_packet_init(&ack2, 888);
    sendto(sockfd, (struct ack_packet *)&ack2, sizeof(ack2),
           MSG_CONFIRM, (const struct sockaddr *)&servaddr,
           sizeof(servaddr));
    struct ack_packet ack3;
    ack_packet_init(&ack3, 555);
    sendto(sockfd, (struct ack_packet *)&ack3, sizeof(ack3),
           MSG_CONFIRM, (const struct sockaddr *)&servaddr,
           sizeof(servaddr));
    // n = recvfrom(sockfd, (struct packet *)&packet, MAXLINE,
    //              MSG_WAITALL, (struct sockaddr *)&servaddr,
    //              (socklen_t *)&len);
    // buffer[n] = '\0';
    // printf("Server : %s\n", packet.data);

    close(sockfd);
    return 0;
}