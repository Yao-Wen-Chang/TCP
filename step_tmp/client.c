#include "TCPSegment.h"
#define CLIENT_INIT_SEQNUM 100
#define CLIENT_NUM 2

// variable declaration
int sockfd;
struct sockaddr_in servaddr, cliaddr;
struct TCPPacket sendPkt[CLIENT_NUM], rcvPkt[CLIENT_NUM];
pthread_mutex_t lock;


// function declaration
void UDPSetup(); 
void *ThreeWayHandshake(void*);
void Request(int);
void StoreVideo(int);
void SendPkt(int);
void ReceivePkt(int);
unsigned short GenChecksum(struct TCPPacket);

int main(int argc, char *argv[]) { // argv is the client port number
    int i;
    int arg[CLIENT_NUM];
    UDPSetup();
    pthread_t ptid[CLIENT_NUM];

    if(pthread_mutex_init(&lock, NULL) !=0) {
        printf("mutex init fail\n");
        return 0;
    }
    for(i = 0; i < CLIENT_NUM; i++) {
        arg[i] = i;
        pthread_create(&ptid[i], NULL, &ThreeWayHandshake, (void*)&arg[i]);

    }   

    for(i = 0; i < CLIENT_NUM; i++) {
        pthread_join(ptid[i], NULL);
    }    
    pthread_exit(NULL);
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

void *ThreeWayHandshake(void *ID) {
    int addrLen = sizeof(cliaddr);
    int pktID = *(int*)ID;
    printf( "[+] [pid %d]-----Start the three-way handshake-----\n", pktID) ;

    sendPkt[pktID].ID = pktID;
    /* first time syn */
    sendPkt[pktID].seqNum = CLIENT_INIT_SEQNUM;
    sendPkt[pktID].isSyn = 1;
    sendPkt[pktID].isAck = 0;
    sendPkt[pktID].request = 0;
    sendPkt[pktID].srcPort = CLIENT_PORT;
    sendPkt[pktID].destPort = SERVER_PORT; 
    SendPkt(pktID);


    // receive the cameback data
    ReceivePkt(pktID);
    
    /* first time ack */
    sendPkt[pktID].seqNum = rcvPkt[pktID].ackNum;
    sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + 1; 
    sendPkt[pktID].isSyn = 0;
    sendPkt[pktID].isAck = 1;
    SendPkt(pktID);


    /* finish handshake pkt setup */
    sendPkt[pktID].isSyn = 0;
    sendPkt[pktID].isAck = 0;
    printf("[+] [pid %d]-----Three way handshake finish-----\n", pktID);
    Request(pktID);
}


/* start test 4 functions */
void Request(int pktID) { 
    int reqType;
    int num1, num2;
    char* videoName;
    char* domainName;
    int addrLen = sizeof(cliaddr);
    while(1) {
        pthread_mutex_lock(&lock);
        printf("[+] [pid %d]Type the request you want: pow:1, sqrt:2, DNS:3, video retrieve:4\n", pktID);
        scanf("%d", &reqType);
        sendPkt[pktID].request = reqType;
        sendPkt[pktID].isAck = 0;

        switch(reqType) {
            case 1: // pow
                printf("[+] [pid %d]Please type in two numbers.\n", pktID);
                scanf("%d%d", &(sendPkt[pktID].intData[0]), &(sendPkt[pktID].intData[1]));
                break;

            case 2: // sqrt
                printf("[+] [pid %d]Please type in one number.\n", pktID);
                scanf("%d", &(sendPkt[pktID].intData[0]));
                break;

            case 3: // DNS
                printf("[+] [pid %d]Please type in a domain name\n", pktID);
                scanf("%s", sendPkt[pktID].charData);
                break;
            case 4: // video
                printf("[+] [pid %d]Please type in which viedo you want: 1.mp4 2.mp4 ....\n", pktID);
                scanf("%s", sendPkt[pktID].videoName);
                
                break;


        }

        SendPkt(pktID);
        if(reqType == 4) {
            StoreVideo(pktID);
        }
        else {
            ReceivePkt(pktID);

            /* switch to reveal which kind of info */ 
            switch(reqType) {
                case 1:
                case 2:
                    printf("[+] [pid %d]the result is %f\n", pktID, rcvPkt[pktID].doubleData);
                    break;

                case 3:
                    printf("[+] [pid %d]the result is %s\n", pktID, rcvPkt[pktID].charData);
                    break;
            }

            /* setup the packet */
            sendPkt[pktID].seqNum = rcvPkt[pktID].ackNum; 
            sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + sizeof(rcvPkt); // the next wanting packet sequence number
            sendPkt[pktID].isAck = 1;
            SendPkt(pktID);
        }
        
        sendPkt[pktID].isAck = 0; // restore for next time request
        
        pthread_mutex_unlock(&lock);
        
    }
    
}


void StoreVideo(int pktID) {
    FILE *file;
    int addrLen = sizeof(cliaddr);
    file = fopen("received-video", "wb");
    sendPkt[pktID].printInfo = 0;
    /* start receive the video */
    if(file != NULL) { 

        while(recvfrom(sockfd, &rcvPkt[pktID], sizeof(rcvPkt[pktID]), 0, (struct sockaddr*)&servaddr, &addrLen) > 0) {
            if(strcmp(rcvPkt[pktID].charData, "final") == 0)
                break;
            if(rcvPkt[pktID].printInfo) {
                printf("[+] [pid %d]receive the packet: seq_num = %d ack_num = %d\n", pktID, rcvPkt[pktID].seqNum, rcvPkt[pktID].ackNum);    
                sendPkt[pktID].printInfo = 1;
                sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum; 
            }
            if(rcvPkt[pktID].checkSum == 87) {
                printf("[-] [pid %d] packet loss detect checksum = %d\n", pktID, rcvPkt[pktID].checkSum);
                sendPkt[pktId].ackNum = rcvPkt[pktID].seqNum;
            }    
            fwrite(rcvPkt[pktID].charData, sizeof(char), MAX_BUFFER_SIZE, file); // write the video into the file

            /* setup the packet */
            //sendPkt[pktID].seqNum = rcvPkt[pktID].ackNum; 
            //sendPkt[pktID].ackNum = rcvPkt[pktID].seqNum + sizeof(rcvPkt[pktID]); // the next wanting packet sequence number
            sendPkt[pktID].isAck = 1;
            SendPkt(pktID);     

        }
        printf("[+] [pid %d]Finish receive the video\n", pktID); 
        fclose(file);
    }    
    else
        printf("[-] [pid %d]fail to create a new file\n", pktID);

}
void SendPkt(int pktID) {
    int addrLen = sizeof(cliaddr);
    /* send the packet */
    if(sendto(sockfd, &sendPkt[pktID], sizeof(sendPkt[pktID]), 0, (struct sockaddr*)&servaddr, addrLen) < 0) {
        printf("[-] [pid %d]Please resend!!\n", pktID);
    }

}
void ReceivePkt(int pktID) {
    int addrLen = sizeof(cliaddr);
    /* receive the packet */
    if(recvfrom(sockfd, &rcvPkt[pktID], sizeof(rcvPkt[pktID]), 0, (struct sockaddr*)&servaddr, &addrLen) < 0){
        printf("[-] [pid %d]Didn't receive the packet\n", pktID);
    }
    else {
        printf("[+] [pid %d]receive the packet: seq_num = %d ack_num = %d\n", pktID, rcvPkt[pktID].seqNum, rcvPkt[pktID].ackNum);    
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
