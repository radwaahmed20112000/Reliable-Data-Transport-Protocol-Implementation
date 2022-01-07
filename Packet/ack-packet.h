#include <stdint.h>
struct ack_packet
{
    uint16_t len;
    uint32_t ackno;
};

void ack_packet_init(struct ack_packet *ack_packet, int ackno);