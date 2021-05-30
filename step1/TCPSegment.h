#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <math.h>
#include <netdb.h> // for DNS method
#define SERVER_PORT 8080
#define CLIENT_PORT 4000
#define MAXLINE 1024


struct TCPPacket {

    int srcPort;
    int destPort;
    int seqNum;
    int ackNum;
    int checkSum;
    int isSyn;
    int isAck;
    int request; // pow:1, sqrt:2, DNS:3, video retrieve:4
    char charData[1024];
    int intData[2];
    double doubleData;
};
