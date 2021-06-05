#include "TCPSegment.h"
#define CLIENT_INIT_SEQNUM 100


// variable declaration
int sockfd;
struct sockaddr_in servaddr, cliaddr;
struct TCPPacket sendPkt, rcvPkt;

void UDPSetup(); 
void ThreeWayHandshake();
void Request();
void StoreVideo();
void SendPkt();
void ReceivePkt();

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
    printf("[+]-----Start the three-way handshake-----\n");

    /* first time syn */
    sendPkt.seqNum = CLIENT_INIT_SEQNUM;
    sendPkt.isSyn = 1;
    sendPkt.isAck = 0;
    sendPkt.request = 0;
    sendPkt.srcPort = CLIENT_PORT;
    sendPkt.destPort = SERVER_PORT; 
    SendPkt();


    // receive the cameback data
    ReceivePkt();
    
    /* first time ack */
    sendPkt.seqNum = rcvPkt.ackNum;
    sendPkt.ackNum = rcvPkt.seqNum + 1; 
    sendPkt.isSyn = 0;
    sendPkt.isAck = 1;
    SendPkt();


    /* finish handshake pkt setup */
    sendPkt.isSyn = 0;
    sendPkt.isAck = 0;
    printf("[+]-----Three way handshake finish-----\n");
}


/* start test 4 functions */
void Request() { 
    int reqType;
    int num1, num2;
    char* videoName;
    char* domainName;
    int addrLen = sizeof(cliaddr);

    while(1) {
        printf("[+]Type the request you want: pow:1, sqrt:2, DNS:3, video retrieve:4\n");
        scanf("%d", &reqType);
        sendPkt.request = reqType;
        sendPkt.isAck = 0;

        switch(reqType) {
            case 1: // pow
                printf("[+]Please type in two numbers.\n");
                scanf("%d%d", &(sendPkt.intData[0]), &(sendPkt.intData[1]));
                break;

            case 2: // sqrt
                printf("[+]Please type in one number.\n");
                scanf("%d", &(sendPkt.intData[0]));
                break;

            case 3: // DNS
                printf("[+]Please type in a domain name\n");
                scanf("%s", sendPkt.charData);
                break;
            case 4: // video
                printf("[+]Please type in which viedo you want: 1.mp4 2.mp4 ....\n");
                scanf("%s", sendPkt.videoName);
                
                break;


        }

        SendPkt();

        if(reqType == 4) {
            StoreVideo();
        }
        else {
            ReceivePkt();

            /* switch to reveal which kind of info */ 
            switch(reqType) {
                case 1:
                case 2:
                    printf("[+]the result is %f\n", rcvPkt.doubleData);
                    break;

                case 3:
                    printf("[+]the result is %s\n", rcvPkt.charData);
                    break;
            }

            /* setup the packet */
            sendPkt.seqNum = rcvPkt.ackNum; 
            sendPkt.ackNum = rcvPkt.seqNum + sizeof(rcvPkt); // the next wanting packet sequence number
            sendPkt.isAck = 1;
            SendPkt();
        }
        
        sendPkt.isAck = 0; // restore for next time request

        
    }
    
}


void StoreVideo() {
    FILE *file;
    int addrLen = sizeof(cliaddr);
    file = fopen("received-video", "wb");

    /* start receive the video */
    if(file != NULL) { 

        while(recvfrom(sockfd, &rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr*)&servaddr, &addrLen) > 0) {
            if(strcmp(rcvPkt.charData, "final") == 0)
                break;
            printf("[+]receive the packet: seq_num = %d ack_num = %d\n", rcvPkt.seqNum, rcvPkt.ackNum);    
            fwrite(rcvPkt.charData, sizeof(char), MAX_BUFFER_SIZE, file); // write the video into the file

            /* setup the packet */
            sendPkt.seqNum = rcvPkt.ackNum; 
            sendPkt.ackNum = rcvPkt.seqNum + sizeof(rcvPkt); // the next wanting packet sequence number
            sendPkt.isAck = 1;
            SendPkt();     

        }
        printf("Finish receive the video\n"); 
        fclose(file);
    }    
    else
        printf("fail to create a new file\n");

}
void SendPkt() {
    int addrLen = sizeof(cliaddr);
    /* send the packet */
    if(sendto(sockfd, &sendPkt, sizeof(sendPkt), 0, (struct sockaddr*)&servaddr, addrLen) < 0) {
        printf("[-]please resend!!\n");
    }
    else {
        printf("[+]sending packet to %s seq_num = %d ack_num = %d\n", "127.0.0.1", sendPkt.seqNum, sendPkt.ackNum);
    }

}
void ReceivePkt() {
    int addrLen = sizeof(cliaddr);
    /* receive the packet */
    if(recvfrom(sockfd, &rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr*)&servaddr, &addrLen) < 0){
        printf("[-]didn't receive the packet\n");
    }
    else {
        printf("[+]receive the packet from %s seq_num = %d ack_num = %d\n", "127.0.0.1", rcvPkt.seqNum, rcvPkt.ackNum);    
    }

}

