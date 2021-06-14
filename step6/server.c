#include "TCPSegment.h"
#define SERVER_INIT_SEQNUM 300
#define CLIENT_NUM 2
/* assume the ack from the client is equal to zero */

/* function declaration */
void UDPSetup();
void PktReceive();
void *PktTransmission(void*); // decide reply which kind of acket and reply it
void Pow(int*, int);
void Sqrt(int*, int);
void DNS(char*, int);
void ReplyVideo(int);
void SendPkt(int);
int PktLossGen(int);
unsigned short GenChecksum(struct TCPPacket);
void detectOutOrder();

/* global variable declaration */
int sockfd;
struct TCPPacket sendPkt[CLIENT_NUM], rcvPkt[CLIENT_NUM], tmpPkt;
struct sockaddr_in servaddr, cliaddr;
int ackHistory[CLIENT_NUM] = {0}; // record how many packet haven't been acked yet for each clients
int dupAckNum[CLIENT_NUM] = {0};
int cwnd[CLIENT_NUM]; // MSS == 1024.0
int isCongestAvoid[CLIENT_NUM] = {0}; // boolean for congestion avoidance
int isSlowStart[CLIENT_NUM] = {1}; // boolean for congestion avoidance
int startProcess[CLIENT_NUM] = {0};

pthread_mutex_t lock;

int main (int argc, char const *argv[]) { 
    UDPSetup();
    int i;
    int client[CLIENT_NUM];

    for(i = 0; i < CLIENT_NUM; i++) {
        client[i] = i;
        pthread_create(&ptid[pktID], NULL, &PktReceive, (void*)&client[i]);
    }    
    for(i = 0; i < CLIENT_NUM; i++) {
        pthread_join(ptid[i], NULL);
    }
    pthread_exit(NULL);
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
    memset(ackHistory, 0, sizeof(CLIENT_NUM)); // set all packet ack to all packets have acked
}
void *PktReceive() {
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
            if(startProcess[pktID] == 1)
                continue;
            memcpy(&rcvPkt[pktID], &tmpPkt, sizeof(tmpPkt));
            printf("[+] [pid %d]Receive the data: ack num = %d, seq num = %d\n", pktID, rcvPkt[pktID].ackNum, rcvPkt[pktID].seqNum);
            /* pthread start process */

        }
        else { 
            ackHistory[tmpPkt.ID] ++; // wrong so no ack
            printf("[-] [pid %d]Something wrong!!\n", pktID);

        }

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
            startProcess[pktID] = 1;
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
            sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum; // the next wanting packet sequence number
            sendPkt[pktID].isAck = 1;

            if(ackHistory[pktID] == 0) {
                printf("******************** delay ack occur*******************\n");
                delay(DELAY_ACK);
            }    
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

    /* local variable declare */ 
    FILE *file;
    char videoPath[20] = "../";
    int rcvLen;
    int i;
    int br = 0; // break when read at the file's end
    int rcvAck = 0; // for purpose of checking duplicate ack
    int addrLen = sizeof(cliaddr);
    int readLen = 0;
    int ssthresh = SSTHRESH;
    clock_t  startTime, endTime;


    /* data setup */
    strcat(videoPath, rcvPkt[pktID].videoName); // because video file put in top directotry
    file = fopen(videoPath, "r");
    sendPkt[pktID].seqNum = 1;
    cwnd[pktID] = 1;
    isCongestAvoid[pktID] = 0;
    isSlowStart[pktID] = 1; 

    if(file != NULL) {
        
        while(fread(sendPkt[pktID].charData, sizeof(char), MSS, file)) {
        printf("****************** slow start ******************\n");
            printf("[+] [pid %d]cwnd = %d kbytes, threshold = %d kbytes\n", pktID, cwnd[pktID], ssthresh);

            sendPkt[pktID].seqNum = cwnd[pktID];  
            sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum;
            SendPkt(pktID);
            printf("[+] [pid %d] send a packet at: %d kbytes\n", pktID, cwnd[pktID]);
            //printf("%ld\n", sizeof(sendPkt[pktID])); 
            /* setup the packet */
            /*sendPkt[pktID].seqNum = cwnd[pktID];  
            sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum;*/
   
            //SendPkt(pktID);
/*
            if(recvfrom(sockfd, &rcvPkt[pktID], sizeof(rcvPkt[pktID]), 0, (struct sockaddr *)&cliaddr, &addrLen) > 0) {
                printf("[+] [pid %d]receive the packet: seq_num = %d ack_num = %d\n", pktID, rcvPkt[pktID].seqNum, rcvPkt[pktID].ackNum);
                if(isSlowStart[pktID]) {
                    if(rcvPkt[pktID].ackNum == rcvAck) {
                        
                        dupAckNum[pktID]++;

                    }        
                    //else { 
                        // new ack 
                        printf("test---------\n");
                        rcvAck = rcvPkt[pktID].ackNum;    
                        cwnd[pktID] *= 2;
                        dupAckNum[pktID] = 0;
                        if(cwnd[pktID] >= ssthresh) {
                            isCongestAvoid[pktID] = 1;
                            isSlowStart[pktID] = 0;
                            
                            printf("****************** congestion avoid ******************\n");
                            printf("cwnd = %d  threshold = %d\n", cwnd[pktID], ssthresh);
                        }    

                    //}
                }
                else if(isCongestAvoid[pktID]) {
                    if(rcvPkt[pktID].ackNum == rcvAck) {
                        dupAckNum[pktID]++;

                    }        
                    //else { 
                        // new ack 
                        rcvAck = rcvPkt[pktID].ackNum;    
                        
                        cwnd[pktID] += 1;
                        dupAckNum[pktID] = 0;
                    //}


                }    
            }
            else { 

                ackHistory[tmpPkt.ID] ++; // wrong so no ack
                printf("[-] [pid %d]receiver did not receive the packet\n", pktID);
            }

            if(br)
                break;                
        
*/
        }    
        strcpy(sendPkt[pktID].charData, "final");
        SendPkt(pktID);
        printf("[+] [pid %d]finish sending the video\n", pktID);
        fclose(file);
        startProcess[pktID] = 0;
       
    }
    else
        printf("[-] [pid %d]incorrect open file\n", pktID);
}
void SendPkt(int pktID) {

    int addrLen = sizeof(cliaddr);
    delay(ROUND_TRIP_DELAY);
    if(PktLossGen(sendPkt[pktID].seqNum)) {
        sendPkt[pktID].checkSum = 87;
    }    
    else 
        sendPkt[pktID].checkSum = GenChecksum(sendPkt[pktID]);

    /* send the packet */
    if(sendto(sockfd, (char*)&sendPkt[pktID], sizeof(sendPkt[pktID]), 0, (struct sockaddr*)&cliaddr, addrLen) < 0) {
        printf("[-] [pid %d]Please resend!!\n", pktID);

    }
}


int PktLossGen(int seq) {
    seq %= 1000000;
    int rndNum = rand() % 1000000;

    return rndNum == seq;
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

