#include "TCPSegment.h"
#define CLIENT_INIT_SEQNUM 100

void UDPSetup(); 
void ThreeWayHandshake();

int main(int argc, char *argv[]) {

    UDPSetop();
    ThreeWayHandshake
 
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
    cliaddr.sin_family = AF_INET; // IPv4
    cliaddr.sin_addr.s_addr = INADDR_ANY;
    cliaddr.sin_port = htons(CLIENT_PORT);

    // Bind the socket with the server address
    if(bind(sockfd, (const struct sockaddr *)&cliaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void ThreeWayHandshake() {
    TCPPacket packet;
    
    /* first time syn */
    // packet setup
    packet.seqNum = CLIENT_INIT_SEQNUM;
    packet.isSyn = 1;
    packet.srcPort = CLIENT_PORT;
    packet.destPort = SERVER_PORT; 

    // send packet
    sendto(sockfd, )


}
