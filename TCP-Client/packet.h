#include <stdint.h>
#include <string>

struct packet {
uint16_t len;
uint32_t seqno;
char data[500];
};


void packet(struct packet *packet, int seqNumber );
