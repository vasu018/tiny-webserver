#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>

// Global Variables
int lport = 1025;
int uport = 65535;
int serverPort;
int numConnServer = 27;
int buffSize = 65535;
char readBuff[65536]; 
char writeBuff[65536]; 
struct hostent *sDetail = 0;
struct in_addr serveraddr;
struct addrinfo details, *detailsptr, *p; // So no need to use memset global variables
char hostIP[16];
void sig_handler(int);
int socketfd = 0;

/* Blacklisted domains: Enter list of blacklisted domains to be blacklisted domains/IPs (witout www)*/
char *blacklists[] = {"cplusplus.com", "edsmart.org", "192.168.100.1", "198.204.255.114", "njit.edu"};
char *blacklistsIPs[100];

char *okMsg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
int okMsgLen = 44;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received Ctrl C to stop the Program. Terminating!!!\n");
        printf("Cleaning up memory space allocated for blacklist IP addresses\n");
        for (int index1 = 0; index1 < sizeof(blacklists)/sizeof(blacklists[0]) ; index1++) {
            free(blacklistsIPs[index1]);
        }
        shutdown (socketfd, 2);
        exit (0);
    }
}

/* will make sure the complete request is sent until successful */
int sendReq(int fd, const char *buf, int bufLen) {
    int numBytes = 0, totalBytes = 0;
    while (1) {
        if((numBytes = send(fd, buf+totalBytes, bufLen-totalBytes, 0)) >= 0) {
            totalBytes = totalBytes + numBytes;
            if (totalBytes >= bufLen) {
	            break;
	        }
        }
        else return -1;
    }
    return totalBytes;
}

/* Send 200 Code to client if successfully processed GET request from client */
int sendSuccess200msg (int fd) {
    sendReq(fd, okMsg, okMsgLen);
    return 0;  
}

int sendAccessDenied (int clientntfd, const char *hostName) {
    printf ("Domain is Blacklisted: %s\n", hostName);
    char firstline [] = "Accessed Denied to this site. Contact Admin: admin@njit.edu";
    send(clientntfd, firstline, strlen(firstline), 0);
    return 0;
}

int isBlacklist (int clientfd2, const char *hostName1) {
    for (int index = 0; index < sizeof(blacklists)/sizeof(blacklists[0]) ; index++) {
        int retCmp = strcmp(hostName1, blacklists[index]);
        if (retCmp == 0) {
            int ret = sendAccessDenied (clientfd2, hostName1);
            if (ret == 0) {
                return 0;
            }
        }
    }
    return -1;
}

int isBlacklistIP (int clientfd2, const char *hostIP1) {
    for (int index = 0; index < sizeof(blacklists)/sizeof(blacklists[0]) ; index++) {
        int retCmp = strcmp(hostIP1, blacklistsIPs[index]);
        if (retCmp == 0) {
            int ret = sendAccessDenied (clientfd2, hostIP1);
            if (ret == 0) {
                return 0;
            }
        }
    }
    return -1;
}

int extractDomainIPs () {
    for (int index = 0; index < sizeof(blacklists)/sizeof(blacklists[0]) ; index++) {
        details.ai_family = AF_INET; // AF_INET means IPv4 only addresses
        int retGetaddr = 0;
        retGetaddr = getaddrinfo(blacklists[index], NULL, NULL, &detailsptr);
        if (retGetaddr == 0) {
            p = detailsptr;              
            getnameinfo(p->ai_addr, p->ai_addrlen, hostIP, sizeof(hostIP), NULL, 0, NI_NUMERICHOST);
        }
        else {
            if(strlen(blacklists[index]) > sizeof(hostIP)) goto lerror;
            strcpy(hostIP, blacklists[index]);
        }
        char *ip = NULL;
        ip = (char *) malloc(16);
        strcpy (ip, hostIP);
        blacklistsIPs[index] = ip;
        freeaddrinfo(detailsptr);
    }
    return 0;

lerror:
    return -1;
}

/* Get the server details, send the request, get the resposne from the server, send it back to the client */
int doRequest(int clntfd, const char *httpBuf, int httpBufLen) {
    char *pPath = NULL;
    char *pHost = NULL;
    char *pPort = NULL;
    char *pEnd = NULL;
    char *pHttpBuf = NULL;
    char pathBuf[1024] = { 0 };
    char hostBuf[1024] = { 0 };
    char portBuf[8] = { 0 };
    int hostLen = 0;
    int svrfd = 0;
    int port = 80;
    int recvLen = 0;
    int remHttpBufLen = 0;
    struct sockaddr_in serv_addr;
    fd_set recvset;
    struct timeval timeout;
    char *hostSub = NULL;

    /* Handle GET request */
    pPath = memmem(httpBuf, 4, "GET ", 4);
    if(pPath) {
        pPath += 4;
	    pEnd = memchr(pPath, ' ', httpBufLen - (pPath - httpBuf));
	    if(pEnd) {
            memcpy(pathBuf, pPath, pEnd - pPath);
	        remHttpBufLen = httpBufLen - (pEnd - httpBuf);
	        pHttpBuf = pEnd;
	    }
        else goto lerror;
        //printf ("First stage of the code (Extracting Path)\n");
    }
    else goto lerror;
    //printf ("Second stage of the code (After path extraction)\n");

    pHost = memmem(pHttpBuf, remHttpBufLen, "Host: ", 6);
    if(pHost) {
        pHost += 6;
	    pEnd = memchr(pHost, '\r', remHttpBufLen - (pHost - httpBuf));
	    if(pEnd) {
	        hostLen = pEnd - pHost;
	        memcpy(hostBuf, pHost, hostLen);
            int ret1 = strncmp(hostBuf, "www.", 4);
            if(ret1 == 0) {
                hostSub = (char *) malloc(sizeof(char) * strlen(hostBuf -4) + 1);
                hostSub = strndup(hostBuf+4, strlen(hostBuf));
            }
            else {
                hostSub = (char *) malloc(sizeof(char) * strlen(hostBuf) + 1);
                strcpy(hostSub, hostBuf);
            }
            //int ret2 = isBlacklist (clntfd, hostSub);
            //if (ret2 == 0) {
            //    free (hostSub);
            //    close (clntfd);
            //    return 0;
            //}
	    }
    }
    else goto lerror;
    //printf ("Third stage of the code (Black list identified or proceeding to talk to server.)\n");

    if(hostLen) {
        pPort = strchr(hostBuf, ':');
	    if(pPort) {
	        strcpy(portBuf, pPort+1);
	        *pPort = 0;
            port = atoi(portBuf);
	    }
	    details.ai_family = AF_INET; // AF_INET means IPv4 only addresses
        int retGetaddr = 0;
        //retGetaddr = getaddrinfo(hostBuf, NULL, &details, &detailsptr);
        retGetaddr = getaddrinfo(hostBuf, NULL, NULL, &detailsptr);
        if (retGetaddr == 0) {
            p = detailsptr;              
            getnameinfo(p->ai_addr, p->ai_addrlen, hostIP, sizeof(hostIP), NULL, 0, NI_NUMERICHOST);
        }
        else {
            if(strlen(hostBuf) > sizeof(hostIP)) goto lerror;
            strcpy(hostIP, hostBuf);
        }
            
        int ret = isBlacklistIP (clntfd, hostIP);
        if (ret == 0) {
            free (hostSub);
            close (clntfd);
            return 0;
        }
        free (hostSub);
        printf("Connecting to Server = %s with IP = %s and Port = %d\n", hostBuf, hostIP, port);

        if ((svrfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Socket creation error \n");
            goto lerror;
        }
        //printf ("Fourth stage of the code (Socket to connect to Server)\n");

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        /* exit if not a valid IP */
        if(inet_pton(AF_INET, hostIP, &serv_addr.sin_addr) <= 0) goto lerror;

        if (connect(svrfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Connection to server Failed \n");
            goto lerror;
        }
        //printf ("Fifth stage of the code (Connect to server)\n");
        if(sendReq(svrfd, httpBuf, httpBufLen) < 0) goto lerror;
        //printf ("Fifth 2 stage of the code (Req sent to server)\n");

        FD_ZERO(&recvset);
        FD_SET(svrfd, &recvset);
        timeout.tv_sec  = 2; 
        timeout.tv_usec = 0;

        /* don't wait for more than 2 sec for a response from the server */
        while(select(svrfd + 1, &recvset, NULL, NULL, &timeout)) {
            /* get the response from the server */
            recvLen = recv(svrfd, &readBuff, buffSize, 0);
            //printf ("Fifth (3) stage of the code (Received Data from server)\n");
            if(recvLen) {
                /* send the response to the client */
                //printf ("Fifth (4) stage of the code (Sending Data to client)\n");
                if (sendReq(clntfd, readBuff, recvLen) < 0) goto lerror;
                //printf ("Fifth (5) stage of the code (Sent Data to client)\n");
            }
            else return 0;
        }
        //printf ("Sixth stage of the code (Recv complete from server)\n");
        close(svrfd);
    }
    //printf ("Seventh stage of the code (Finished)\n");
    close (clntfd);
    return 0;

lerror:
    return -1;
}

int main (int argc, char *argv[]) {
    int i = 0; 
    if (argc < 2) {
        printf ("\n# Usage: ./stage2 <server_port_no>\n\n");
        return(0);
    }
    int serverPort = atoi(argv[1]);
    if (serverPort < lport || serverPort > uport) {
        printf ("Invalid Port: %s\n", argv[1]);
        return(0);
    } 
    
    printf("stage 2 program by (PG355) listening on port (%d)  \n", serverPort);
    printf("\n * ### Press Ctl-C  To graciously stop the server ###\n");

    extractDomainIPs();
    //for (int index1 = 0; index1 < sizeof(blacklists)/sizeof(blacklists[0]) ; index1++) {
    //    printf ("Black list IP is: %d, %s \n", index1, blacklistsIPs[index1]);
    //}

    /* Create the Server Socket. */
    struct sockaddr_in server_sock;
    int bindret = 0;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        printf ("Unable to create socket (Error code): %d\n", socketfd);
        exit(1);
    }
    int option = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, 
         (const void *)&option , sizeof(int));
    /* Set the attributes to Server socket (port number, protocol and so on). */
    memset(&server_sock, '0', sizeof(server_sock));
    server_sock.sin_family = AF_INET;
    server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sock.sin_port = htons(serverPort);
    
    /* Bind to the Socket on the port defined above. */
    bindret = bind(socketfd, (struct sockaddr*)&server_sock, sizeof(server_sock));  
    if (bindret < 0) {
        printf ("Unable to bind socket (Error code): %d\n", bindret);
        exit(1);
    }

    /* Listening on serverPort defined above. */
    int listenret = listen(socketfd, numConnServer); 
    if (listenret < 0) {
        printf ("Unable to listen socket (Error code): %d\n", listenret);
        exit(1);
    }
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("\nHandling CTRL C\n");
        return -1;
    }

    int counter = 0;
    while(1) {
        int clientfd = 0;
        /* Accept the connections from the clients. */
        clientfd = accept (socketfd, (struct sockaddr*)NULL, NULL);
        bzero(readBuff, buffSize);
        int n = 0;

        /* Receive the data from clients. */
        n = recv(clientfd, &readBuff, buffSize, 0);
        if (n < 0) {
            printf("ERROR receiving data from the socket\n");
        }

        if(doRequest (clientfd, readBuff, n) != 0) {
            printf ("Failed handling the request\n");
        }
        //printf ("Eight stage (Closing client fd)");
        close (clientfd); 
    }
    return 0;
}
