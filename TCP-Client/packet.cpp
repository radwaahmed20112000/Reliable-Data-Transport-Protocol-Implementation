#include <stdint.h>
#include "packet.h"


void packet(struct packet *packet, int seqNumber ){
    packet -> seqno = seqNumber;

    int length = sizeof(packet);
    packet -> len = sizeof(packet) + sizeof(length);
}



