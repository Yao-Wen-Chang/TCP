#include "TCPSegment.h"
#define SERVER_INIT_SEQNUM 300
void UDPSetup();
void PktReceive();
void PktTransmission(); // decide reply which kind of acket and reply it
double Pow(int*);
double Sqrt(int*);
char* DNS();
void ReplyVideo();
void ThreeWayHandshake();

int sockfd;
struct TCPPacket rcvPkt;
struct sockaddr_in servaddr, cliaddr;

int main (int argc, char const *argv[]) {
    UDPSetup();
    PktReceive();
}


void UDPSetup() {
    
    // Creating socket file descriptor
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Filling client address
    memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; // IPv4
    cliaddr.sin_addr.s_addr = INADDR_ANY;
    cliaddr.sin_port = htons(CLIENT_PORT);

    // filling server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);

    // Bind the socket with the server address
    if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}
void PktReceive() {
    int rcvLen; // receive bytes
    int addrLen = sizeof(cliaddr);
    while(1) { 
        rcvLen = recvfrom(sockfd, (char*)&rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr *)&cliaddr, &addrLen);
        if(rcvLen > 0) {
            printf("Receive the data: ack num = %d, seq num = %d\n", rcvPkt.ackNum, rcvPkt.seqNum);
        }
        else 
            printf("Something wrong!!\n");
        
        PktTransmission();    
    }
}

void PktTransmission() {
    struct TCPPacket sendPkt;
    double result;
    char *IP;
    int addrLen = sizeof(cliaddr);

    sendPkt.srcPort = SERVER_PORT;
    sendPkt.destPort = CLIENT_PORT;
     
    if(rcvPkt.isSyn) {
        sendPkt.ackNum = rcvPkt.seqNum + 1;
        sendPkt.seqNum = SERVER_INIT_SEQNUM;    
        printf("check---- %d\n", sendPkt.seqNum);
        sendPkt.isSyn = 1;
        sendPkt.isAck = 1;
    }
    else if(rcvPkt.isAck) {
        
        sendPkt.isSyn = 0;
        sendPkt.isAck = 0;
        printf("-----Three way handshake finish-----\n");
    }
    else { 
        switch(rcvPkt.request) {
            case 1:
                sendPkt.intData = malloc(sizeof(double));
                result = Pow(rcvPkt.intData);    
                sendPkt.doubleData = &(result);
                break;

            case 2:
                sendPkt.intData = malloc(sizeof(double));
                result = Sqrt(rcvPkt.intData);
                sendPkt.doubleData = &(result);
                break;

            case 3:
                sendPkt.charData = malloc(sizeof(char) * 50);
                IP = DNS(rcvPkt.charData); 
                sendPkt.charData = IP;
                break;

            case 4:
                ReplyVideo();
                break;

            default:
                break;
        }

    }

    /* setup the packet */
    sendPkt.seqNum = rcvPkt.ackNum;  
    sendPkt.ackNum = rcvPkt.seqNum + sizeof(sendPkt);
   
    /* send the packet */
    if(sendto(sockfd, (char*)&sendPkt, sizeof(sendPkt), 0, (struct sockaddr*)&cliaddr, addrLen) < 0) {
        printf("Please resend!!\n");

    }
    else {
        printf("the sending packet seq-num: %d ack_num = %d\n", sendPkt.seqNum, sendPkt.ackNum);
    }
}
double Pow(int* intData) {
    return pow(*intData, *(intData + 1));
}

double Sqrt(int* intData) {
    return sqrt(*intData);

}

char* DNS(char* hostname) {
    struct hostent *hostInfo;
    struct in_addr *address;

    hostInfo = gethostbyname(hostname);
    
    if(hostname == NULL) {
        printf("Couldn't lookup %s\n", hostname);
    }
    else {
        address = (struct in_addr *) (hostInfo->h_addr);
        return inet_ntoa(*address); 
    }
}

void ReplyVideo() {
    printf("which");

}

void ThreeWayHandshake() {
    

}
