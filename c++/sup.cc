// sup.cc : simple udp-based protocol implementation.
// by: allison morris

#include "sup.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>


// initializes the client and opens a socket to addr:port.
sup::client::client(unsigned int addr, unsigned int port) 
  : _addr(addr), _port(port), _seqnum(0)  {

}

// packs the data into the buffer and prepapres to send.
void sup::client::create_msg(int len, char* data) {

}

// initializes the server and opens socket on addr:port.
sup::server::server(unsigned int addr, unsigned int port, unsigned int rate) 
  : _addr(addr), _port(port), _rate(rate) {

}

// listens for messages on socket. sends ack after message is received.
void sup::server::listen() {

}

// sends the message in the buffer and waits until ack is received.
// resends message if ack is not returned.
void sup::client::send_msg() {

}
