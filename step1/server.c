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
struct TCPPacket rcvPkt, sendPkt;
struct sockaddr_in servaddr, cliaddr;

int main (int argc, char const *argv[]) {
    UDPSetup();
    PktReceive();

    return 0;
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
        rcvLen = recvfrom(sockfd, &rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr *)&cliaddr, &addrLen);
        if(rcvLen > 0) {
            printf("Receive the data: ack num = %d, seq num = %d\n", rcvPkt.ackNum, rcvPkt.seqNum);
            PktTransmission();    
        }
        else 
            printf("Something wrong!!\n");
        
    }
}
void PktTransmission() {
    double result;
    char *IP;
    int addrLen = sizeof(cliaddr);


    /* set the send packet */
    sendPkt.srcPort = SERVER_PORT;
    sendPkt.destPort = CLIENT_PORT;
     
    if(rcvPkt.isSyn) {
        sendPkt.ackNum = rcvPkt.seqNum + 1;
        sendPkt.seqNum = SERVER_INIT_SEQNUM;    
        sendPkt.isSyn = 1;
        sendPkt.isAck = 1;
    }
    else if(rcvPkt.isAck) {
        
        sendPkt.isSyn = 0;
        sendPkt.isAck = 0;
        printf("-----Three way handshake finish-----\n");
        return;
    }
    else { 
        switch(rcvPkt.request) {
            case 1:
                result = Pow(rcvPkt.intData);    
                sendPkt.doubleData = result;
                break;

            case 2:
                result = Sqrt(rcvPkt.intData);
                sendPkt.doubleData = result;
                break;

            case 3:
                strcpy(sendPkt.charData, DNS(rcvPkt.charData)); 
                break;

            case 4:
                ReplyVideo();
                break;

            default:
                break;
        }
        /* setup the packet */
        sendPkt.seqNum = rcvPkt.ackNum;  
        sendPkt.ackNum = rcvPkt.seqNum + sizeof(rcvPkt);
    }

   
    /* send the packet */
    if(sendto(sockfd, (char*)&sendPkt, sizeof(sendPkt), 0, (struct sockaddr*)&cliaddr, addrLen) < 0) {
        printf("Please resend!!\n");

    }
    else {
        printf("the sending packet seq-num: %d ack_num = %d\n", sendPkt.seqNum, sendPkt.ackNum);
    }





}
double Pow(int* intData) {
    return pow(intData[0], intData[1]);
}

double Sqrt(int* intData) {
    return sqrt(intData[0]);

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
        printf("%s has address %s\n", hostname, inet_ntoa(*address));
        return inet_ntoa(*address); 
    }
}

void ReplyVideo() {
    
    char video_name[10];
    FILE *file;

    strcpy(video_name, rcvPkt.charData);
    
    file = fopen(video_name, 'r');
    fread(sendPkt.charData, sizeof(char), 1024, file) // buffer , bytes of element , elements count , file


}

