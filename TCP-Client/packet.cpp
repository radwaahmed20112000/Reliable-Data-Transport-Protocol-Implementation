#include "Packet.h"
#include <iostream>    
#include <fstream>  
#include <string>
using namespace std;

void packet_init(struct packet *packet ,int start , int end ,std::string fileName ,int seqNumber ){
    char* dataPointer = readBytes(start , end , fileName);
    packet -> seqno = seqNumber;
    packet -> data  = dataPointer;
    int length = sizeof(packet);
    packet -> len = sizeof(packet)+sizeof(length);

}

void request_packet_init(struct packet *packet, char fileName[],int seqNumber ){
    packet -> seqno = seqNumber;
    strcpy(packet -> data,fileName);
    int length = sizeof(packet);
    packet -> len = sizeof(packet) + sizeof(length);
}


int getFileLength(std::string fileName){
       std::ifstream is (fileName, std::ifstream::binary);
       int length = 0 ;
        if (is) {
            // get length of file:
            is.seekg (0, is.end);
            length = is.tellg();
            is.seekg (0, is.beg);
            is.close();}
            return length;
        }
        

char* readBytes(int start , int end ,std::string fileName ){
    std::ifstream is (fileName, std::ifstream::binary);
  if (is) {
   
    is.seekg (start, is.beg);
    int length = end - start +1;
    char * buffer = new char [length];

    std::cout << "Reading " << length << " characters... ";
    // read data as a block:
    is.read (buffer,length);

    if (is)
      std::cout << "all characters read successfully.";
    else
      std::cout << "error: only " << is.gcount() << " could be read";
    is.close();
   
    for(int i=0; i<length; i++){//display elements using for loop
      std::cout<<"char_array["<<i<<"]:"<<buffer[i];
      std::cout<<("\n");
       
  }
  int arrSize = (sizeof(buffer)/sizeof(*buffer));
  cout << "The size of the array is: " << buffer[10];
    
  }
}
int main(){
   std::string fileName = "adv.txt";
    readBytes(3,5,fileName);
}
