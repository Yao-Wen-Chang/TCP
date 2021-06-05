#include "TCPSegment.h"
#define SERVER_INIT_SEQNUM 300
#define CLIENT_NUM 2

void UDPSetup();
void PktReceive();
void *PktTransmission(void*); // decide reply which kind of acket and reply it
void Pow(int*, int);
void Sqrt(int*, int);
void DNS(char*, int);
void ReplyVideo(int);
void SendPkt(int);

int sockfd;
struct TCPPacket sendPkt[CLIENT_NUM], rcvPkt[CLIENT_NUM], tmpPkt;
struct sockaddr_in servaddr, cliaddr;
pthread_mutex_t lock;

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
    int addrLen = sizeof(cliaddr);
    int pktID;
    int i;
    pthread_t ptid[CLIENT_NUM];
    if(pthread_mutex_init(&lock, NULL) !=0) {
        printf("mutex init fail\n");
        return;
    }
    /* start receiving packet */
    while(1) { 

        pthread_mutex_lock(&lock); // lock point to prevent race condition
        if(recvfrom(sockfd, &tmpPkt, sizeof(tmpPkt), 0, (struct sockaddr *)&cliaddr, &addrLen) > 0) {
            pktID = tmpPkt.ID;
            memcpy(&rcvPkt[pktID], &tmpPkt, sizeof(tmpPkt));
            printf("[+] [pid %d]Receive the data: ack num = %d, seq num = %d\n", pktID, rcvPkt[pktID].ackNum, rcvPkt[pktID].seqNum);
            /* pthread start process */

            pthread_create(&ptid[pktID], NULL, &PktTransmission, (void*)&pktID);
        }
        else 
            printf("[-] [pid %d]Something wrong!!\n", pktID);
        
    }
    for(i = 0; i < CLIENT_NUM; i++) {
        pthread_join(ptid[i], NULL);
    } 
    pthread_exit(NULL);
}
void *PktTransmission(void *ID) {
    int pktID = *(int*)ID;
    if(rcvPkt[pktID].isSyn) {
        sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + 1;
        sendPkt[pktID].seqNum = SERVER_INIT_SEQNUM;    
        sendPkt[pktID].isSyn = 1;
        sendPkt[pktID].isAck = 1;
        SendPkt(pktID);
    }
    else if(rcvPkt[pktID].isAck && rcvPkt[pktID].request == 0) {
        
        sendPkt[pktID].isSyn = 0;
        sendPkt[pktID].isAck = 0;
        printf("[+] [pid %d]-----Three way handshake finish-----\n", pktID);
    }
    else if(!rcvPkt[pktID].isAck){ 
        
        if(rcvPkt[pktID].request == 4) {
            ReplyVideo(pktID);
        }
        else {
            switch(rcvPkt[pktID].request) {
                case 1:
                    Pow(rcvPkt[pktID].intData, pktID);    
                    break;

                case 2:
                    Sqrt(rcvPkt[pktID].intData, pktID);
                    break;

                case 3:
                    DNS(rcvPkt[pktID].charData, pktID); 
                    break;

                default:
                    break;
            }
           
            /* setup the packet */
            sendPkt[pktID].seqNum = rcvPkt[pktID].ackNum;
            sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + sizeof(rcvPkt[pktID]); // the next wanting packet sequence number
            sendPkt[pktID].isAck = 1;
            SendPkt(pktID);
        }
    }
    pthread_mutex_unlock(&lock); // unlock

}

void Pow(int* intData, int pktID) {
    sendPkt[pktID].doubleData = pow(intData[0], intData[1]);
}

void Sqrt(int* intData, int pktID) {
    sendPkt[pktID].doubleData = sqrt(intData[0]);
}

void DNS(char* hostname, int pktID) {
    struct hostent *hostInfo;
    struct in_addr *address;

    hostInfo = gethostbyname(hostname);
    if(hostname == NULL) {
        printf("[-] [pid %d]Couldn't lookup %s\n", pktID, hostname);
    }
    else {
        address = (struct in_addr *) (hostInfo->h_addr);
        printf("[+] [pid %d]%s has address %s\n", pktID, hostname, inet_ntoa(*address));
        strcpy(sendPkt[pktID].charData, inet_ntoa(*address)); 
    }
}

void ReplyVideo(int pktID) {
    
    FILE *file;
    char videoPath[20] = "../";
    int rcvLen;

    int addrLen = sizeof(cliaddr);
    strcat(videoPath, rcvPkt[pktID].videoName); // because video file put in top directotry
    file = fopen(videoPath, "r");
    
    if(file != NULL) {
        while(fread(sendPkt[pktID].charData, sizeof(char), MAX_BUFFER_SIZE, file)) {

            /* setup the packet */
            sendPkt[pktID].seqNum = rcvPkt[pktID].ackNum;  
            sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + sizeof(rcvPkt[pktID]);

            // buffer , bytes of element , elements count , file
            SendPkt(pktID);

            /* receive the ack if ack keep sending */
            
            if(recvfrom(sockfd, &rcvPkt[pktID], sizeof(rcvPkt[pktID]), 0, (struct sockaddr *)&cliaddr, &addrLen) > 0) {
                printf("[+] [pid %d]receive the packet: seq_num = %d ack_num = %d\n", pktID, rcvPkt[pktID].seqNum, rcvPkt[pktID].ackNum);
            }
            else 
                printf("[-] [pid %d]receiver did not receive the packet\n", pktID);
        }    
        strcpy(sendPkt[pktID].charData, "final");
        SendPkt(pktID);
        printf("[+] [pid %d]finish sending the video\n", pktID);
        fclose(file);
       
    }
    else
        printf("[-] [pid %d]incorrect open file\n", pktID);








}
void SendPkt(int pktID) {

    int addrLen = sizeof(cliaddr);


    /* send the packet */
    if(sendto(sockfd, (char*)&sendPkt[pktID], sizeof(sendPkt[pktID]), 0, (struct sockaddr*)&cliaddr, addrLen) < 0) {
        printf("[-] [pid %d]Please resend!!\n", pktID);

    }
    else {
        printf("[+] [pid %d]the sending packet seq-num: %d ack_num = %d\n", pktID, sendPkt[pktID].seqNum, sendPkt[pktID].ackNum);
    }
}
