#include "Packet.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>
#include <string.h>

using namespace std;

char *readBytes(int start, int end, std::string fileName)
{
  std::ifstream is(fileName, std::ifstream::binary);

  int length = end - start + 1;
  char *buffer = new char[length];

  if (is)
  {
    is.seekg(start, is.beg);

    // read data as a block:
    is.read(buffer, length);

    if (is)
      std::cout << "all characters read successfully.\n";
    else
      std::cout << "error: only " << is.gcount() << " could be read\n";
    is.close();
  }
  return buffer;
}

void packet_init(struct packet *packet, int start, int end, string fileName, int seqNumber)
{
  char *dataPointer = readBytes(start, end, fileName);
  strcpy(packet->data, dataPointer);
  packet->seqno = seqNumber;
  cout << "I am counter " << seqNumber << "\n";
  packet->len = sizeof(packet);
  // cout << "The length is: " << packet->len;
  // std::cout << ("\n");
  // cout << "The seqno is: " << packet->seqno;
  // std::cout << ("\n");
  // cout << "The size is: " << sizeof(packet->data) / sizeof(packet->data[0]);
  // std::cout << ("\n");
}
void packet_end(struct packet *packet)
{
  packet->seqno = -1;
}