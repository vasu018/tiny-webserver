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

#define MAX_BUFF_SIZE 65535 // Changing buffer value for each test

int main(int argc, char *argv[])
{
    int sock, length;
    struct sockaddr_in serverInfo;
    char buff[MAX_BUFF_SIZE];
    int ret_b;
    int lport = 1025;
    int uport = 65535;
    int bufferLength = 512; /* Default = 512 */
    int jitter = 0; /* Default = 1 */
    int burstInterval = 0; /* Default in microseconds */

    if (argc < 4) {
        printf ("\n# Usage: ./udp host port filename [bufferLength] [jitter] [burstInterval]\n");
        printf ("\n# Examples: 1. ./udp host port filename\n");
        printf ("#           2. ./udp host port filename 512 none 1\n");
        //printf ("Example: ./stage2 blacklist.txt 2222\n");
        return(0);
    }
    int serverPort = atoi(argv[2]);
    if (serverPort < lport || serverPort > uport) {
        printf ("Invalid Port: %s\n", argv[2]);
        return(0);
    }
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Error in creating a socket");
        exit(-1);
    }

    memset (&serverInfo, '0', sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(atoi(argv[2]));
    serverInfo.sin_addr.s_addr = inet_addr(argv [1]);
    //serverInfo.sin_addr.s_addr=htonl(argv [1]);
  
    if (argv[4]) {
        bufferLength = atoi(argv[4]);
    }
    printf ("Coming till here\n");
    if (argv[5]) {
        jitter = atoi(argv[5]);
    }
    if (argv[6]) {
        burstInterval = atoi(argv[6]);
    }
    if (jitter < 0 || jitter > 10) {
        printf("Error in jitter value (use values between 0 to 9)\n");
        exit(-1);
    }
    printf ("Coming till here\n");
    
    FILE *FP = fopen(argv[3], "rb");
    if (FP == NULL) {
        printf ("Error reading the blacklist input file %s\n", argv[3]);
        exit (1);
    }
    printf ("Coming till here\n");

    printf("Forwarding media: %s to %s\n", argv[3], argv[1]);
    //while (fread (&buff, MAX_BUFF_SIZE, 1 , FP) == 1) {
    while (fread (&buff, bufferLength, 1 , FP) == 1) {
	    int len = 0;
        len = sendto(sock, buff, bufferLength, 0 , (struct sockaddr*)&serverInfo, sizeof(serverInfo));
        //printf("Data sent of len: %d\n", len);
        printf(".");
        usleep (burstInterval);
    }
    printf ("\n");

    //fclose(fpointer);
    //close(sock);
    return(0);
}
