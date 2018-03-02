#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>

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

char *pOKMsg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
int OKMsgLen = 44;

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("Received Ctrl C to stop the Program. Terminating!!!\n");
        shutdown (socketfd, 2);
        exit (0);
    }
}

/* will make sure the complete request is sent until successful */
int wsSend( int fd, const char *buf, int bufLen )
{
    int numBytes = 0, totalBytes = 0;
    //printf ("%s\n", buf);

    while (1) 
    {
        if( (numBytes = send(fd, buf+totalBytes, bufLen-totalBytes, 0)) >= 0 )
        {
            totalBytes = totalBytes + numBytes;
            if (totalBytes >= bufLen) 
	    {
	        break;
	    }
        }
        else return -1;
    }

    return totalBytes;
}

/* Send 200 Code to client if successfully processed GET request from client*/
int sendSuccess200msg (int fd) {

    wsSend(fd, pOKMsg, OKMsgLen);
    return 0;  
}

/* get the server details, send the request,
 * get the resposne from the server, send it back to the client
 */
int doRequest(int clntfd, const char *httpBuf, int httpBufLen) 
{
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

    /* as of now handles only GET request */
    pPath = memmem( httpBuf, 4, "GET ", 4 );
    if( pPath )
    {
        pPath += 4;
	pEnd = memchr( pPath, ' ', httpBufLen - (pPath - httpBuf) );
	if( pEnd )
	{
            memcpy( pathBuf, pPath, pEnd - pPath );
	    remHttpBufLen = httpBufLen - (pEnd - httpBuf);
	    pHttpBuf = pEnd;
            //printf("PATH = %s\n", pathBuf);
	}
        else goto lerror;
    }
    else goto lerror;

    pHost = memmem( pHttpBuf, remHttpBufLen, "Host: ", 6 );
    if( pHost )
    {
        pHost += 6;
	pEnd = memchr( pHost, '\r', remHttpBufLen - (pHost - httpBuf) );
	if( pEnd )
	{
	    hostLen = pEnd - pHost;
	    memcpy( hostBuf, pHost, hostLen );
	}
    }
    else goto lerror;

    if( hostLen )
    {
        pPort = strchr( hostBuf, ':' );
	if( pPort )
	{
	    strcpy( portBuf, pPort+1 );
	    *pPort = 0;
            port = atoi(portBuf);
	}
                    
	details.ai_family = AF_INET; // AF_INET means IPv4 only addresses
        int retGetaddr = 0;
        retGetaddr = getaddrinfo(hostBuf, NULL, &details, &detailsptr);
        if (retGetaddr == 0) 
	{
            p = detailsptr;              
            getnameinfo(p->ai_addr, p->ai_addrlen, hostIP, sizeof(hostIP), NULL, 0, NI_NUMERICHOST);
        }
        else
        {
            if( strlen(hostBuf) > sizeof(hostIP) ) goto lerror;
            strcpy(hostIP, hostBuf);
        }

        printf("Connecting to Host = %s with IP = %s and Port = %d\n", hostBuf, hostIP, port);

        if ((svrfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Socket creation error \n");
            goto lerror;
        }

        memset(&serv_addr, '0', sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        /* exit if not a valid IP */
        if( inet_pton(AF_INET, hostIP, &serv_addr.sin_addr) <= 0 ) goto lerror;

        if (connect(svrfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("Connection to server Failed \n");
            goto lerror;
        }
       
        if( wsSend( svrfd, httpBuf, httpBufLen ) < 0 ) goto lerror;

        FD_ZERO(&recvset);
        FD_SET(svrfd, &recvset);
        timeout.tv_sec  = 2;
        timeout.tv_usec = 0;

        /* don't wait for more than 2 sec for a response from the server */
        while( select(svrfd + 1, &recvset, NULL, NULL, &timeout) )
        {
            /* get the response from the server */
            recvLen = recv(svrfd, &readBuff, buffSize, 0);
            if( recvLen )
            {
                /* send the response to the client */
                wsSend(clntfd, readBuff, recvLen);
            }
            else return 0;
        }

        close( svrfd );
    }

    return 0;

lerror:
    return -1;
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
    printf("\n * ### Press Ctl-C  To graciously stop the server ###\n");

    /* Create the Server Socket. */
    struct sockaddr_in server_sock;
    //int socketfd = 0;
    int bindret = 0;
    //int clientfd = 0;

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

        if( doRequest (clientfd, readBuff, n) != 0 )
        {
            printf ("Failed handling the request\n");
        }

        close (clientfd); 
    }

    return 0;
}
