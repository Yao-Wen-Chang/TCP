#include "TCPSegment.h"
#define CLIENT_INIT_SEQNUM 100


// variable declaration
int sockfd;
struct sockaddr_in servaddr, cliaddr;

struct TCPPacket sendPkt, rcvPkt;
void UDPSetup(); 
void ThreeWayHandshake();
void Request();

int main(int argc, char *argv[]) {

    UDPSetup();
    ThreeWayHandshake();
    Request();
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
    if(bind(sockfd, (const struct sockaddr *)&cliaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void ThreeWayHandshake() {
    int addrLen = sizeof(cliaddr);
    printf( "-----Start the three-way handshake-----\n" ) ;

    /* first time syn */
    // packet setup
    sendPkt.seqNum = CLIENT_INIT_SEQNUM;
    sendPkt.isSyn = 1;
    sendPkt.isAck = 0;
    sendPkt.srcPort = CLIENT_PORT;
    sendPkt.destPort = SERVER_PORT; 

    // send packet to server
    if(sendto(sockfd, (char*)&sendPkt, sizeof(sendPkt), 0,
               (struct sockaddr*)&servaddr, addrLen) < 0) {

        printf("Please reconnect!!");
    }
    else {
        printf("the sending packet seq-num: %d\n", sendPkt.seqNum);
    
    }
     
    // receive the cameback data
    if(recvfrom(sockfd, (char*)&rcvPkt, sizeof(rcvPkt), 0,
        (struct sockaddr*)&servaddr, &addrLen) < 0){
        printf("Didn't receive the packet\n");
    }
    else {

        printf("receive the packet: seq_num = %d ack_num = %d\n", rcvPkt.seqNum, rcvPkt.ackNum);    

        sendPkt.seqNum = rcvPkt.ackNum;
        sendPkt.ackNum = rcvPkt.seqNum + 1; 
        sendPkt.isSyn = 0;
        sendPkt.isAck = 1;
    }
    
    if(sendto(sockfd, (char*)&sendPkt, sizeof(sendPkt), 0,
               (struct sockaddr*)&servaddr, addrLen) < 0) {

        printf("Please reconnect!!");
    }
    else {
        printf("the sending packet seq-num: %d ack_num = %d\n", sendPkt.seqNum, sendPkt.ackNum);

        sendPkt.isSyn = 0;
        sendPkt.isAck = 0;
    }

    printf("-----Three way handshake finish-----\n");
}


/* start test 4 functions */
void Request() { 
    int reqType;
    int num1, num2;
    char* videoName;
    char* domainName;
    int addrLen = sizeof(cliaddr);

    while(1) {
        printf("Type the request you want: pow:1, sqrt:2, DNS:3, video retrieve:4\n");
        scanf("%d", &reqType);
        sendPkt.request = reqType;

        switch(reqType) {
            case 1: // pow
                printf("Please type in two numbers.\n");
                scanf("%d%d", &(sendPkt.intData[0]), &(sendPkt.intData[1]));
                break;

            case 2: // sqrt
                printf("Please type in one number.\n");
                scanf("%d", &(sendPkt.intData[0]));
                break;

            case 3: // DNS
                printf("Please type in a domain name\n");
                scanf("%s", sendPkt.charData);
                break;
            case 4: // video
                printf("Please type in which viedo you want: 1.mp4 2.mp4 ....\n");
                scanf("%s", sendPkt.charData);
                
                break;


        }

        /* setup the packet */
        sendPkt.seqNum = rcvPkt.ackNum; 
        sendPkt.ackNum = rcvPkt.seqNum + sizeof(rcvPkt); // the next wanting packet sequence number

        /* send the packet */
        if(sendto(sockfd, &sendPkt, sizeof(sendPkt), 0,
                   (struct sockaddr*)&servaddr, addrLen) < 0) {

            printf("Please resend!!\n");
        }
        else {
            printf("sending packet seq_num = %d ack_num = %d\n", sendPkt.seqNum, sendPkt.ackNum);

        }

        /* receive the packet */ 
        if(recvfrom(sockfd, &rcvPkt, sizeof(rcvPkt), 0,
            (struct sockaddr*)&servaddr, &addrLen) < 0){
            printf("Didn't receive the packet\n");
        }
        else {

            printf("receive the packet: seq_num = %d ack_num = %d\n", rcvPkt.seqNum, rcvPkt.ackNum);    
            switch(reqType) {
                case 1:
                case 2:
                    printf("checkpoint\n");
                    printf("the result is %f\n", rcvPkt.doubleData);
                    break;

                case 3:
                    printf("the result is %s\n", rcvPkt.charData);
                    break;
            }
        }

        

    }
}
