#include <stdint.h>
#include "ack-packet.h"

void ack_packet_init(struct ack_packet *ack_packet, int ackno)
{
    ack_packet->ackno = ackno;
}
