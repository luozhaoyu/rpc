#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int process_request(struct MyRPC *request, struct MyRPC *response, int dropped, int total, int drop_rate) {
    srand(time(NULL));
    float r;
    r = (float)(rand() % 100) / 100;
    if (r < (float)(drop_rate - dropped) / (100 - total)) { // drop it
        return 1;
    } else if (request->flag == 0) {
        bzero(response, sizeof(struct MyRPC));
        response->flag = 1; // ack
        return 0;
    }
}

int start_rpc(int drop_rate) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;

    struct MyRPC request, response;
    int total=0;
    int dropped=0;
    int status;

    // main loop of rpc udp server
    while (1) {
        if ((numbytes = recvfrom(sockfd, &request, sizeof(request), 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        status = process_request(&request, &response, dropped, total, drop_rate);
        if (status < 0) {
            perror("fail in processing request");
        } else if (status == 1) { // drop it
            dropped++;
        } else if (status == 0) { // ack it
            if ((numbytes = sendto(sockfd, &response, sizeof(response), 0,
                (struct sockaddr *)&their_addr, addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }
        } else {
            perror("strange output");
        }
        total++;
    }

    printf("listener: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    close(sockfd);

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Input a packet drop_rate!\n");
        exit(1);
    }
    start_rpc(atoi(argv[1]));
}
