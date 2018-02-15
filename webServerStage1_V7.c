#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

// Global Variables
int lport = 1025;
int uport = 65535;
int serverPort;
int numConnServer = 27;
int readBuffSize = 65535;
char readBuff[65535]; 
char readBuff2[65535]; 
int writeBuffSize = 65535;
char teamName[] = "";
struct hostent *sDetail = 0;
struct in_addr serveraddr;
struct addrinfo details, *detailsptr, *p; // So no need to use memset global variables
char hostIP[256];
void     sig_handler(int);
int socketfd = 0;

//void sig_handler(int signo, int socketfd)
void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("Received Ctrl C to stop the Program. Terminating!!!\n");
        shutdown (socketfd, 2);
        exit (0);
    }
}

/* Send 200 Code to client if successfully processed GET request from client*/
int sendSuccess200msg (int recvfd3) {

    char writeBuff1[65535]; 
    bzero(writeBuff1, writeBuffSize);
    sprintf(writeBuff1, "HTTP/1.1 200 OK\r\n");
    send(recvfd3, writeBuff1, strlen(writeBuff1), 0);
    
    bzero(writeBuff1, writeBuffSize);
    sprintf(writeBuff1, "Content-Type: text/html\r\n");
    send(recvfd3, writeBuff1, strlen(writeBuff1), 0);
    
    bzero(writeBuff1, writeBuffSize);
    strcpy(writeBuff1, "\r\n");
    send(recvfd3, writeBuff1, strlen(writeBuff1), 0);
    return 0;  
}

//int doParse(int clientfd2, const char *readBuff2, int recvBytes) {
int doParse(int clientfd2, char *readBuff2, int recvBytes) {

    int successRet;
    char writeBuff2[65535]; 
    successRet = sendSuccess200msg (clientfd2);
    if (successRet != 0 ) {
        printf ("Error in sending Success 200 code to client\n");
    }
    
    bzero(writeBuff2, writeBuffSize);
    sprintf(writeBuff2, "%s", readBuff2);
    int numBytes, totalBytes;

    while (1) {
        numBytes = send(clientfd2, writeBuff2, strlen(writeBuff2), 0);
        printf ("%s", writeBuff2);
        totalBytes = totalBytes + numBytes;
        if (totalBytes >= strlen(writeBuff2)) {
            break;
        }
    }
    
    char * firstline = NULL; /* Host IP */
    char * secondline = NULL; /* Port Number */
    char * thirdline_temp = NULL; /* Temp Path */
    char * thirdline = NULL; /* Path */
    char *words;
    char delimiter[] = "\r";
    int errorflag = 0;
    words = strtok (writeBuff2,delimiter);
    while (words) {
        if (strstr(words, "GET") != NULL) {
            printf ("GET inside. Here inside counter 2\n");
            thirdline_temp = malloc (strlen(words));
            if (thirdline_temp != NULL) {
                strcpy (thirdline_temp, words);
            }
        }
        if (strstr(words, "Host:") != NULL) {
            printf ("Host inside. Here inside counter 2\n");
            int counter = 1;
            const char *wordslist1;
            wordslist1 = strtok (words, ":");
            while (wordslist1 != NULL) {
                if (counter == 2) {
                    char *firstline_temp = NULL;
                    printf ("Host inside. Inside while\n");
                    firstline_temp = malloc (strlen(wordslist1));
                    if (firstline_temp != NULL) {
                        strcpy (firstline_temp, wordslist1);
                        printf ("Alloc success: \n");
                    }
                    else {
                        continue;
                    }
                    details.ai_family = AF_INET; // AF_INET means IPv4 only addresses
                    int retGetaddr = 0;
                    printf ("Here inside counter 2: \n");
                    if ((firstline_temp[0] == '\n') || (firstline_temp[0] == ' ')) {
                        printf ("0. Here inside counter if");
                        retGetaddr = getaddrinfo(firstline_temp+1, NULL, &details, &detailsptr);
                        printf ("1. Here inside counter if");
                    }
                    else {
                        printf ("0. Here inside counter else");
                        retGetaddr = getaddrinfo(firstline_temp, NULL, &details, &detailsptr);
                        printf ("2. Here inside counter else");
                    }
                    //printf ("%s\n", firstline_temp);
                    //retGetaddr = getaddrinfo(firstline_temp, NULL, &details, &detailsptr);
                    if (retGetaddr != 0) {
                        printf("Getaddrinfo: %d\n",retGetaddr);
                        errorflag =1;
                    }
                    if (errorflag != 1) {
                        for(p = detailsptr; p != NULL; p = p->ai_next) {
                            getnameinfo(p->ai_addr, p->ai_addrlen, hostIP, sizeof(hostIP), NULL, 0, NI_NUMERICHOST);
                            printf ("Server IP: %s\n",hostIP);
                        }
                    }
                    firstline = malloc (strlen(wordslist1)+25);
                    strcpy (firstline, "HOST = ");
                    strcat (firstline, wordslist1);
                    if (errorflag != 1) {
                        strcat (firstline, " (");
                        strcat (firstline, hostIP);
                        strcat (firstline, ")");
                        freeaddrinfo(detailsptr);
                        printf ("Freeing firstline_temp\n");
                        free (firstline_temp);
                    }
                    else {
                        strcat (firstline, " (");
                        strcat (firstline, "0.0.0.0");
                        strcat (firstline, ")");
                    }
                }
                if (counter == 3) {
                    secondline = malloc (strlen(wordslist1)+7);
                    strcpy (secondline, "PORT = ");
                    strcat (secondline, wordslist1);
                }
                counter++;
                wordslist1 = strtok (NULL, ":");
            }
        }
        words = strtok (NULL,delimiter);
    }
    int sendRet1, sendRet2, sendRet3;
    char stringStart[] = "";
    char stringbr1[] = "<style>body{color:black} span{color:#FF0000}</style></br>";
    char stringbr2[] = "<span></br> </br>";
    char stringEnd[] = "</span>";
    sendRet1 = send(clientfd2, stringbr2, strlen(stringbr2), 0);
    if (firstline != NULL ) {
        printf ("%s\n", firstline);
        sendRet1 = send(clientfd2, firstline, strlen(firstline), 0);
        printf ("Freeing firstline\n");
        free (firstline); 
        //free (firstline_temp); 
    }
    if (sendRet1 < 0) {
        printf ("Error Sending Host: %d", sendRet1);
    }

    sendRet1 = send(clientfd2, stringbr1, strlen(stringbr1), 0);
    if (secondline != NULL) {
        printf ("%s\n", secondline);
        sendRet2 = send(clientfd2, secondline, strlen(secondline), 0);
        printf ("Freeing secondline\n");
        free (secondline); 
    }
    if (sendRet2 < 0) {
        printf ("Error Sending Port: %d", sendRet2);
    }
    
    sendRet3 = send(clientfd2, stringbr1, strlen(stringbr1), 0);
    if (thirdline_temp != NULL) {
        int counter = 1;
        char *wordslist3;
        wordslist3 = strtok (thirdline_temp, " ");
        while (wordslist3 != NULL) {
            if (counter == 2) {
                thirdline = malloc (strlen(wordslist3) + 6);
                strcpy (thirdline, "PATH = ");
                strcat (thirdline, wordslist3);
            }
            counter++;
            wordslist3 = strtok (NULL, " ");
        }
        if (thirdline != NULL) {
            printf ("%s\n", thirdline);
            sendRet3 = send(clientfd2, thirdline, strlen(thirdline), 0);
            printf ("Freeing thirdline\n");
            free (thirdline); 
            printf ("Freeing thirdline_temp\n");
            //free (thirdline_temp); 
            printf ("After Freeing thirdline_temp\n");
            if (sendRet3 < 0) {
                printf ("Error Sending Path: %d", sendRet3);
            }
        }
    }
    return 0;
}

int main (int argc, char *argv[]) {
    int i = 0;
    if (argc < 2) {
        printf ("\n# Usage: ./stage1 <server_port_no>\n\n");
        return(0);
    }
    int serverPort = atoi(argv[1]);
    if (serverPort < lport || serverPort > uport) {
        printf ("Invalid Port: %s\n", argv[1]);
        return(0);
    } 
    
    printf("stage 1 program by (PG355) listening on port (%d)  \n", serverPort);
    printf("### Press Ctl-C  To graciously stop the server ###\n");

    /* Create the Server Socket. */
    struct sockaddr_in server_sock;
    int bindret = 0;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        printf ("Unable to create socket (Error code): %d\n", socketfd);
        exit (1);
    }

    /* Set the attributes to Server socket (port number, protocol and so on). */
    memset(&server_sock, '0', sizeof(server_sock));
    server_sock.sin_family = AF_INET;
    server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sock.sin_port = htons(serverPort);
    
    /* Bind to the Socket on the port defined above. */
    bindret = bind(socketfd, (struct sockaddr*)&server_sock, sizeof(server_sock));  
    if (bindret < 0) {
        printf ("Unable to bind socket (Error code): %d\n", bindret);
        exit (1);
    }

    /* Listening on serverPort defined above. */
    int listenret = listen(socketfd, numConnServer);
    if (listenret < 0) {
        printf ("Unable to listen for connections: %d\n", listenret);
        exit (1);
    }

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("\nHandling CTRL C\n");
        //exit (1);
    }

    int counter = 0;
    while(1) {
        int clientfd = 0;
        /* Accept the connections from the clients. */
        clientfd = accept (socketfd, (struct sockaddr*)NULL, NULL);
        if (clientfd < 0) {
            printf ("Error in accepting connections from client\n");
            exit (1);
        }
            
        bzero(readBuff, readBuffSize);
        int n = 0;

        /* Receive the data from clients. */
        n = recv(clientfd, &readBuff, readBuffSize, 0);
        if (n < 0) {
            printf("ERROR receiving data from the socket\n");
        }
        
        int parseRet;
        parseRet = doParse (clientfd, readBuff, n);
        if (parseRet != 0) {
            printf ("Failed Parsing the received data from client\n");
        }
        close (clientfd); 
    }
}
