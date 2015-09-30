#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <strings.h>
#include <sys/epoll.h>
#include <errno.h>


#define PORT 32100
#define BUF_SIZE (512*1024 + 1)

#define MAX_PACKET_SIZE (512) // it should be less than max UDP packet size: 65536
#define MAX_DATA_SIZE (MAX_PACKET_SIZE - sizeof(uint8_t) - sizeof(uint8_t) - sizeof(uint32_t))

// assert(MAX_DATA_SIZE > 0);

struct MyRPC {
    uint8_t function_id; // echo = 0
    uint8_t flag; // MSG = 0, ACK = 1
    uint32_t actual_size; // actual data size
    int8_t data[MAX_DATA_SIZE]; // actual_size <= MAX_PACKET_SIZE
};


#endif
