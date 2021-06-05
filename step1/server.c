#include "TCPSegment.h"
#define SERVER_INIT_SEQNUM 300
void UDPSetup();
void PktReceive();
void PktTransmission(); // decide reply which kind of acket and reply it
void Pow(int*);
void Sqrt(int*);

void Add(int*);
void Sub(int*);
void Mul(int*);
void Div(int*);

void DNS();
void ReplyVideo();
void SendPkt();

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

    /* set the send packet */
    sendPkt.srcPort = SERVER_PORT;
    sendPkt.destPort = CLIENT_PORT;
}
void PktReceive() {
    int rcvLen; // receive bytes
    int addrLen = sizeof(cliaddr);
    while(1) { 
        rcvLen = recvfrom(sockfd, &rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr *)&cliaddr, &addrLen);
        if(rcvLen > 0) {
            printf("[+]Receive the data: ack num = %d, seq num = %d\n", rcvPkt.ackNum, rcvPkt.seqNum);
            PktTransmission();    
        }
        else 
            printf("[-]Something wrong!!\n");
        
    }
}
void PktTransmission() {
    if(rcvPkt.isSyn) {
        sendPkt.ackNum = rcvPkt.seqNum + 1;
        sendPkt.seqNum = SERVER_INIT_SEQNUM;    
        sendPkt.isSyn = 1;
        sendPkt.isAck = 1;
        SendPkt();
    }
    else if(rcvPkt.isAck && rcvPkt.request == 0) {
        
        sendPkt.isSyn = 0;
        sendPkt.isAck = 0;
        printf("[+]-----Three way handshake finish-----\n");
        return;
    }
    else if(!rcvPkt.isAck) { 
        if(rcvPkt.request == 4) {
            ReplyVideo();
        }
        else {
            switch(rcvPkt.request) {
                case 1:
                    Pow(rcvPkt.intData);    
                    break;

                case 2:
                    Sqrt(rcvPkt.intData);
                    break;
                    

                case 3:
                    DNS(rcvPkt.charData); 
                    break;
                case 5:
                    Add(rcvPkt.intData);
                    break;
                case 6:
                    Sub(rcvPkt.intData);
                    break;
                case 7:
                    Mul(rcvPkt.intData);
                    break;
                case 8:
                    Div(rcvPkt.intData);
                    break;
                default:
                    break;
            }
           
            /* setup the packet */
            sendPkt.seqNum = rcvPkt.ackNum;
            sendPkt.ackNum = rcvPkt.seqNum + sizeof(rcvPkt); // the next wanting packet sequence number
            sendPkt.isAck = 1;
            SendPkt();
        }
    }

}

void Pow(int* intData) {
    sendPkt.doubleData = pow(intData[0], intData[1]);
}

void Sqrt(int* intData) {
    sendPkt.doubleData = sqrt(intData[0]);
}

void Add(int* intData) {
    sendPkt.doubleData = intData[0] + intData[1];

}

void Sub(int* intData) {
    
    sendPkt.doubleData = intData[0] - intData[1];
}

void Mul(int* intData) {

    sendPkt.doubleData = intData[0] * intData[1];
}    
void Div(int* intData) {

    sendPkt.doubleData = intData[0] / intData[1];
}
void DNS(char* hostname) {
    struct hostent *hostInfo;
    struct in_addr *address;
    hostInfo = gethostbyname(hostname);
    if(hostInfo == NULL) {
        printf("[-]Couldn't lookup %s\n", hostname);
    }
    else {
        address = (struct in_addr *) (hostInfo->h_addr);
        printf("[+]%s has address %s\n", hostname, inet_ntoa(*address));
        strcpy(sendPkt.charData, inet_ntoa(*address)); 
    }
}

void ReplyVideo() {
    
    FILE *file;
    char videoPath[20] = "../";
    int rcvLen;

    int addrLen = sizeof(cliaddr);
    strcat(videoPath, rcvPkt.videoName); // because video file put in top directotry
    file = fopen(videoPath, "r");
    
    if(file != NULL) {
        while(fread(sendPkt.charData, sizeof(char), MAX_BUFFER_SIZE, file)) {

            /* setup the packet */
            sendPkt.seqNum = rcvPkt.ackNum;  
            sendPkt.ackNum = rcvPkt.seqNum + sizeof(rcvPkt);

            // buffer , bytes of element , elements count , file
            SendPkt();

            /* receive the ack if ack keep sending */
            rcvLen = recvfrom(sockfd, &rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr *)&cliaddr, &addrLen);
            
            if(rcvLen > 0) {
                printf("[+]receive the packet: seq_num = %d ack_num = %d\n", rcvPkt.seqNum, rcvPkt.ackNum);
            }
            else 
                printf("[-]receiver did not receive the packet");
                
        }    
        strcpy(sendPkt.charData, "final");
        SendPkt();
        printf("[+]finish sending the video\n");
        fclose(file);
       
    }
    else
        printf("[-]incorrect open file\n");








}
void SendPkt() {

    int addrLen = sizeof(cliaddr);


    /* send the packet */
    if(sendto(sockfd, (char*)&sendPkt, sizeof(sendPkt), 0, (struct sockaddr*)&cliaddr, addrLen) < 0) {
        printf("[-]Please resend!!\n");

    }
    else {
        printf("[+]the sending packet seq-num: %d ack_num = %d\n", sendPkt.seqNum, sendPkt.ackNum);
    }
}
