# Reliable-Data-Transport-Protocol-Implementation

## Specifications
Suppose you’ve a file and you want to send this file from one side to the other (server to
client). You will need to split the file into chunks of data of fixed length and add the data of
one chunk to a UDP datagram packet in the data field of the packet. You need to implement
TCP with congestion control.
## Packet type and fields
There are two kinds of packets: Data packets and Ack-only packets.

![UDP datagram](https://github.com/radwaahmed20112000/Reliable-Data-Transport-Protocol-Implementation/blob/main/UDPdatagram.png)

## Congestion Control 
There are two main indicators for congestion: lost packets (buffer overflow at
routers) long delays (queuing in router buffers). Not handling network congestion may lead
to unneeded retransmissions (link carries multiple copies of a packet, since when a packet is
dropped, any upstream transmission capacity used for that packet is wasted)! To Implement
congestion control, the FSM of TCP congestion control given below is followed.

![GBN FSM](https://github.com/radwaahmed20112000/Reliable-Data-Transport-Protocol-Implementation/blob/main/GBN.png)

## Packet loss simulation
Implementation runs in a simulated environment, this means that we simulates
the packet loss probability (PLP) since packet loss is infrequent in a localhost or LAN
environment.

## Workflow between server and client
### Flow of data
The main steps are:
1. The client sends a datagram to the server to get a file giving its filename. This
send needs to be backed up by a timeout in case the datagram is lost.
2. The server forks off a child process to handle the client.
3. The server (child) creates a UDP socket to handle file transfer to the client.
4. Server sends its first datagram, the server uses some random number generator
random() function to decide with probability p if the datagram would be passed
to the method send() or just ignore sending it
5. Whenever a datagram arrives, an ACK is sent out by the client to the server.
6. If you choose to discard the package and not to send it from the server the
timer will expire at the server waiting for the ACK that will never come from
the client (since the packet wasn’t sent to it) and the packet will be resent again
from the server.
7. Update the window, and make sure to order the datagrams at the client side.
8. repeat those steps till the whole file is sent and no other datagrams remain.
9. close the connection.
