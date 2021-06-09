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
#include <time.h> // for calculate timeout

#define SERVER_PORT 8080
#define CLIENT_PORT 4000
#define MAX_BUFFER_SIZE 512
#define DELAY_ACK 0.5 // delay() /s
#define MSS 1.0 // kbytes
#define ROUND_TRIP_DELAY 15 // ms
#define SSTHRESH 64.0 // kbytes 
#define TRANSMISSION_RATE 10 // Mbps
#define TIMEOUT 2.0

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
void delay(int number_of_seconds) // because the delay function is not working
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
  
    // Storing start time
    clock_t start_time = clock();
  
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}
