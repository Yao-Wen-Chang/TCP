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
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details.
#include <pthread.h>
#include <system.h> // for sleep() or wait()

#define SERVER_PORT 8080
#define CLIENT_PORT 4000
#define MAX_BUFFER_SIZE 512
#define DELAY_ACK 500 // delay() ms
#define MSS 1 // kbytes
#define ROUND_TRIP_DELAY 15 // ms
#define VAR_THREASHOLD 64 // kbytes 
#define TRANSMISSION_RATE 10 // Mbps

struct TCPPacket {
    int ID;
    int srcPort;
    int destPort;
    int seqNum;
    int ackNum;
    unsigned short checkSum;
    int isSyn;
    int isAck;
    int request; // pow:1, sqrt:2, DNS:3, video retrieve:4
    char charData[MAX_BUFFER_SIZE];
    char videoName[10];
    int videoLen;
    int intData[2];
    double doubleData;
};
