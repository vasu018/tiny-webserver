#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_BUFF_SIZE 65535

int print_usage () {
    printf ("\n# Usage: ./udp host port filename [packetLength] [burstSize] [burstInterval (in USec)]\n");
    printf ("# Examples: 1. ./udp host port filename\n");
    printf ("#           2. ./udp host port filename 512 1 1\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int sock, length;
    struct sockaddr_in serverInfo;
    char buff[MAX_BUFF_SIZE];
    int ret_b;
    int lport = 1025;
    int uport = 65535;
    int packetLength = 1400; /* Default = 512 */
    int burstSize = 1; /* Default = 1 */
    long burstInterval = 0; /* Default in microseconds */

    if (argc < 4) {
        print_usage();
    }
    int serverPort = atoi(argv[2]);
    if (serverPort < lport || serverPort > uport) {
        printf ("Invalid Port: %s\n", argv[2]);
        return(0);
    }
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("# Error in creating a socket");
        exit(-1);
    }

    memset (&serverInfo, '0', sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[2]));
    serverInfo.sin_addr.s_addr = inet_addr(argv [1]);
  
    if (argc >= 5 && argv[4]) {
        packetLength = atoi(argv[4]);
        if (packetLength < 128 && packetLength > 9000) {
            printf("# Error in burstSize value (use values between 128 and 9000)\n");
            exit(-1);
        }
    }
    if (argc >= 6 && argv[5]) {
        burstSize = atoi(argv[5]);
        if (burstSize < 1 || burstSize > 100) {
            printf("# Error in burstSize value (use values between 0 to 100)\n");
            exit(-1);
        }
    }
    if (argc == 7 && argv[6]) {
        burstInterval = atoi(argv[6]);
    }
    
    FILE *FP = fopen(argv[3], "rb");
    if (FP == NULL) {
        printf ("# Error reading the media input file %s\n", argv[3]);
        print_usage();
    }

    printf("Forwarding media: %s to %s\n", argv[3], argv[1]);
    int iter = 1;
    int bufferLength = packetLength-28;
    while (fread (&buff, bufferLength, 1, FP) == 1) {
        if (iter > burstSize) {
            iter = 1;
            usleep (burstInterval);
        }
        int numBytes = 0, totalBytes = 0;
        numBytes = sendto(sock, buff+totalBytes, bufferLength-totalBytes, 0, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
        if  (numBytes >= 0) {
            totalBytes = totalBytes + numBytes;
            if (totalBytes >= sizeof (buff)) {
                break;
            }
        }
        printf(".");
        iter ++;
    }
    printf ("\n");
    fclose(FP);
    close(sock);
}
