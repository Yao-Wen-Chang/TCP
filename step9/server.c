#include "TCPSegment.h"
#define SERVER_INIT_SEQNUM 300
#define CLIENT_NUM 100

void UDPSetup();
void PktReceive();
void *PktTransmission(void*); // decide reply which kind of acket and reply it
void Pow(int*, int);
void Sqrt(int*, int);
void DNS(char*, int);
void ReplyVideo(int);
void SendPkt(int);
unsigned short GenChecksum(struct TCPPacket);



int sockfd;
struct TCPPacket sendPkt[CLIENT_NUM], rcvPkt[CLIENT_NUM], tmpPkt;
struct sockaddr_in servaddr, cliaddr;
int cwnd[CLIENT_NUM]; // MSS == 1024.0
int isCongestAvoid[CLIENT_NUM] = {0}; // boolean for congestion avoidance
int isSlowStart[CLIENT_NUM] = {1}; // boolean for congestion avoidance
int isFastRecovery[CLIENT_NUM] = {0};

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
    cwnd[pktID] = 1;
    isCongestAvoid[pktID] = 0;
    isSlowStart[pktID] = 1;
    isFastRecovery[pktID] = 0;
    sendPkt[pktID].printInfo = 0;
    int cwndCount = 0;
    int ssthresh = SSTHRESH;
    int nonAck = 0;
    int fastRetran = 0;
    /* generate random loss pkt */
    int lossIndex = rand()%1000;
    int lossCount = 0;

    if(file != NULL) {
        printf("****************** slow start ******************\n");
        while(fread(sendPkt[pktID].charData, sizeof(char), MAX_BUFFER_SIZE, file)) {
            /* check sum */
            if(lossCount == lossIndex) {
                fastRetran++;
                printf("[-]packet loss\n");
                sendPkt[pktID].checkSum = 87;
                isSlowStart[pktID] = 1;
                isCongestAvoid[pktID] = 0;
                if(fastRetran == 4) {
                    lossCount++;
                    printf("[+] ****************** Fast recovery ****************\n");
                    isSlowStart[pktID] = 0;
                    isCongestAvoid[pktID] = 0;
                    isFastRecovery[pktID] = 1;
                    
                }    
            }
            else {
                lossCount ++;
                sendPkt[pktID].checkSum = GenChecksum(sendPkt[pktID]);

            }
            /* setup the packet */
            //sendPkt[pktID].seqNum = rcvPkt[pktID].ackNum;  
            //sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + sizeof(rcvPkt[pktID]);

            // buffer , bytes of element , elements count , file
            SendPkt(pktID);
            cwndCount++;

            sendPkt[pktID].printInfo = 0;
            /* receive the ack if ack keep sending */
            
            if(recvfrom(sockfd, &rcvPkt[pktID], sizeof(rcvPkt[pktID]), 0, (struct sockaddr *)&cliaddr, &addrLen) > 0) {
                if(rcvPkt[pktID].printInfo){
                    printf("[+] [pid %d]receive the packet: seq_num = %d ack_num = %d\n", pktID, rcvPkt[pktID].seqNum, rcvPkt[pktID].ackNum);
                }  
                // delay ack check  
                if(rcvPkt[pktID].isAck == 0) {
                    nonAck ++; 
                }     
                else if(nonAck == 0) {
                    printf("[+] delay ack !\n");
                }    
            }
            else 
                printf("[-] [pid %d]receiver did not receive the packet\n", pktID);
            
            if(isSlowStart[pktID]) {
                if(cwndCount == cwnd[pktID]) {
                    sendPkt[pktID].printInfo = 1;
                    sendPkt[pktID].seqNum = cwnd[pktID];
                    printf("[+] [pid %d] sending the packet : %d kbytes\n", pktID, cwnd[pktID]);
                    cwnd[pktID] *= 2;
                    //dupAckNum[pktID] = 0;
                    if(cwnd[pktID] >= ssthresh) {
                        isCongestAvoid[pktID] = 1;
                        isSlowStart[pktID] = 0;
                            
                        isFastRecovery[pktID] = 0;
                        printf("****************** congestion avoid ******************\n");
                        printf("cwnd = %d  threshold = %d\n", cwnd[pktID], ssthresh);
                    }  
                }    
            }
            else if(isCongestAvoid[pktID]) {
                if(cwndCount == cwnd[pktID]) {
                    sendPkt[pktID].printInfo = 1;
                    sendPkt[pktID].seqNum = cwnd[pktID];
                    printf("[+] [pid %d] sending the packet : %d kbytes\n", pktID, cwnd[pktID]);
                    cwnd[pktID] += 1;
                    //dupAckNum[pktID] = 0;    

                    //printf("****************** congestion avoid ******************\n");
                }
            }
            else if(isFastRecovery[pktID]) {
                ssthresh = cwnd[pktID] / 2;
                cwnd[pktID] = ssthresh + 3;
                cwnd[pktID] *= 2;
                isCongestAvoid[pktID] = 1;
                isSlowStart[pktID] = 0;
                isFastRecovery[pktID] = 0;
                printf("****************** congestion avoid ******************\n");
            }

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
}
unsigned short GenChecksum(struct TCPPacket pkt) {
    unsigned short sum = 0 ;
	unsigned short temp = 0 ;
	sum += pkt.srcPort ;
	sum += pkt.destPort ;
	if ( 65535-pkt.srcPort < pkt.destPort ) ++sum ;

	temp = ( pkt.seqNum >> 16 ) ;
	sum += temp ;

	if ( 65535-sum < temp ) ++sum ;
	temp = pkt.seqNum ;
	sum += temp ;
	if ( 65535-sum < temp ) ++sum ;

	temp = ( pkt.ackNum >> 16 ) ;
	sum += temp ;
	if ( 65535-sum < temp ) ++sum ;
	temp = pkt.ackNum ;
	sum += temp ;
	if ( 65535-sum < temp ) ++sum ;

	uint16_t *data_pointer = (uint16_t *) pkt.charData ;
	int bytes = sizeof( pkt.charData ) ;
	while(bytes > 1){
        temp = (unsigned short)*data_pointer++;
        //If it overflows to the MSBs add it straight away
        sum += temp ;
        if ( 65535-sum < temp ) ++sum ;
        bytes -= 2; //Consumed 2 bytes
    }

    return ~sum;

}
