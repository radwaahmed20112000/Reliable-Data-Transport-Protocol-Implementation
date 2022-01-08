#include <stdint.h>
#include <string>

struct packet
{
    uint16_t len;   //Packet length
    uint32_t seqno; //Sequence number
    char data[500];
};

void packet_init(struct packet *packet, int start, int end, std::string fileName, int seqNumber);
void packet_end(struct packet *packet);
// void station_load_train(struct station *station, int count);
