#include <stdint.h>
#include <string>

struct packet {
uint16_t len;
uint32_t seqno;
char data[5];
};

void packet_init(struct packet *packet ,int start , int end ,std::string fileName ,int seqNumber);

void packet_init(struct packet *packet, char fileName[], int seqNumber );
