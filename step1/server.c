#include "TCPSegment.h"
#define SERVER_INIT_SEQNUM 300
void UDPSetup();
void PktReceive();
void PktTransmission(int, int, int, int, int, int, int, char*);
double Pow();
double Sqrt();
int* DNS();

int main (int argc, cahr const *argv[]) {

}


void UDPSetup() {
    
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    // Creating socket file descriptor
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

     // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
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
    char *ack = "ack";
    socklen_t len = sizeof(cliaddr);  //len is value/resuslt
    while(1) { 
        TCPPacket rcvPkt; 
        rcvLen = recvfrom(fd, (char*)&rcvPkt, sizeof(rcvPkt), 0, (struct sockaddr *)&cliaddr, &addrlen);
        if(rcvLen > 0) {
            printf("Receive the data: ack num = %d, seq num = %d\n", rcvPkt.ackNum, rcvPkt.seqNum);
            if
        }
        else 
            printf("Something wrong!!\n");
        
        PktTransmission(rcvPkt.srcPort, SERVER_PORT, rcvPkt.seqNum, rcvPkt.ackNum, rcvPkt.checkSum, rcvPkt.isSyn, rcvPkt.isAck, rcvPkt.request);    
        sendto(sockfd, (const char *)ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
    }
}

void PktTransmission(int srcPort, int destPort, int seqNum, int ackNum, int checkSum, int isSyn, int isAck, char* request) {
    TCPPacket sendPkt;
    sendPkt.srcPort = destPort;
    sendPkt.destPort = srcPort;
     
    if(isSyn) {
        sendPkt.ackNum = seqNum + 1;
        sendPkt.seqNum = SERVER_INIT_SEQNUM;    
        sendPkt.isSyn = 1;
        sendPkt.isAck = 1;
    }
    else {
    
        switch(request) {
            case "pow":
                double result = Pow();    
            case "sqrt":
                double result = Sqrt();
            case "DNS":
                int* ip = DNS(); 
            case "video":

        }

    }


}

int Pow() {
    int num1, num2;
    printf("Enter the number: ");
    scanf("%d, %d", num1, num2);
    return pow(num1, num2);
}

int Sqrt() {
    int num;
    printf("Enter the number: ");
    scanf("%d", num);
    return sqrt(num);

}

int* DNS() {



}
