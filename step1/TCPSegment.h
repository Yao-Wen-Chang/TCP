#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <math.h>
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
    char *request; // video retrieve, math calculation, DNS
    char *charData;
    int *intData;

}
