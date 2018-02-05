#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h>

// Global Variables
char teamName[] = "pg355";
int lport = 1024;
int uport = 65535;
int serverPort;
int numConnServer = 27;
int readBuffSize = 1024;
char readBuff[1024]; 
int writeBuffSize = 1024;
char writeBuff[1024]; 

struct hostent *sDetail = 0;
//struct sockaddr_in serveraddr;
struct in_addr serveraddr;

// References from the internet that i used.
// http://web.theurbanpenguin.com/adding-color-to-your-output-from-c/
// http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html

void red () {
  printf("\033[1;31m");
}

void reset () {
  printf("\033[0m");
}

#if 0
void replaceAll(char * str, char oldChar, char newChar)
{
    int i = 0;

    /* Run till end of string */
    while(str[i] != '\0')
    {
        /* If occurrence of character is found */
        if(str[i] == oldChar)
        {
            str[i] = newChar;
        }

        i++;
    }
}

#endif

/* Send 200 Code to client if successfully processed GET request from client*/
int sendSuccess200msg (int recvfd3) {

    bzero(writeBuff, writeBuffSize);
    sprintf(writeBuff, "HTTP/1.1 200 OK\r\n");
    send(recvfd3, writeBuff, strlen(writeBuff), 0);
    
    bzero(writeBuff, writeBuffSize);
    sprintf(writeBuff, "Content-Type: text/html\r\n");
    send(recvfd3, writeBuff, strlen(writeBuff), 0);
    
    bzero(writeBuff, writeBuffSize);
    strcpy(writeBuff, "\r\n");
    send(recvfd3, writeBuff, strlen(writeBuff), 0);
    return 0;  
}

int doParse(int clientfd2, const char *readBuff2, int recvBytes) {

    /* Send the Code 200 to client for successful*/
    int successRet;
    successRet = sendSuccess200msg (clientfd2);
    if (successRet != 0 ) {
        printf ("Error in sending Success 200 code to client\n");
    }
    
    bzero(writeBuff, writeBuffSize);
    sprintf(writeBuff, readBuff2);
       
    int numBytes, totalBytes;

    while (1) {
        numBytes = send(clientfd2, writeBuff, strlen(writeBuff), 0);
        totalBytes = totalBytes + numBytes;
        //printf("Bytes sent: %d", numBytes);
        if (totalBytes >= strlen(writeBuff)) {
            break;
        }
    }

    
    //char string[50] ="Test,string1,Test,string2:Test:string3";
    //printf ("Before: %s\n", writeBuff);
    //replaceAll(writeBuff,":\s", ":");
    //printf ("After: %s\n", writeBuff);
    
    //char * str = malloc (sizeof(writeBuff));
    //strcpy(str, writeBuff);
    //printf ("String copy%s", str);
    
    char * firstline; /* Host IP */

    char * secondline; /* Port Number */

    char * thirdline; /* Path */
    

    char *words;
    //printf ("String is split into tokens:\n %s \n",writeBuff);
    char delimiter[] = "\r";
    words = strtok (writeBuff,delimiter);
    //words = strtok (writeBuff,"\r");
    //while (words!= NULL) {
    while (words) {
        //printf ("\n Words: %s", words);
#if 0
        if (strstr(words, "GET") != NULL) {
            int counter = 1;
            printf ("Counter Value: %d", counter);
            //printf("Matching GET: %s\n", words);
            char *wordslist3;
            //wordslist3 = strtok (words, " ");
            while (wordslist3 != NULL) {
                if (counter == 2) {
                    //thirdline = malloc (strlen(wordslist3));
                    //strcpy (thirdline, wordslist3);
                }
                //printf ("Counter is: %d\n", counter);
                counter++;
                //wordslist3 = strtok (NULL, " ");
            }
            //words = strtok (NULL, "\r");
        }
#endif
        if (strstr(words, "Host:") != NULL) {
            int counter = 1;
            printf("Matching Host: %s\n", words);
            //printf ("Counter Value: %d", counter);
            char *wordslist1;
            wordslist1 = strtok (words, ":");
            while (wordslist1 != NULL) {
                //printf ("Counter is: %d => %s\n", counter,wordslist1);
                if (counter == 2) {
                    printf ("Firstline is: %s\n", wordslist1);
                    secondline = malloc (strlen(wordslist1));
                    strcpy (secondline, wordslist1);
                    sDetail = gethostbyname (wordslist1);
                    if (!sDetail) {
                        printf ("Error: Hostname can not be resolved\n");
                        //exit(0);
                    }
                    else {
                        while (*sDetail->h_addr_list) {
                            bcopy(*sDetail->h_addr_list++, (char *) &serveraddr, sizeof(serveraddr));
                            printf("address: %s\n", inet_ntoa(serveraddr));
                        }
                    }
                }
                if (counter == 3) {
                    printf ("Thirdline is: %s\n", wordslist1);
                    firstline = malloc (strlen(wordslist1));
                    strcpy (firstline, wordslist1);
                }
                counter++;
                wordslist1 = strtok (NULL, ":");
            }
            //words = strtok (NULL, "\r");
            //words = strtok (NULL,delimiter);
        }
//#endif
#if 0
        if (strstr(words, "Referer:") != NULL) {
            int counter = 1;
            //printf("Matching Host: %s\n", words);
            printf("Matching Referer: %s\n", words);
            char *wordslist2;
            wordslist2 = strtok (words, ":");
            while (wordslist2 != NULL) {
                //printf ("Word is: %s\n", wordslist2);
                if (counter == 3) {
                    printf ("Thirdline is: %s\n", wordslist2);
                    //secondline = malloc (strlen(wordslist2));
                    //strcpy (secondline, wordslist2);
                }
                if (counter == 4) {
                    printf ("Fourthline is: %s\n", wordslist2);
                    //secondline = malloc (strlen(wordslist2));
                    //strcpy (secondline, wordslist2);
                }
                counter++;
                wordslist2 = strtok (NULL, ":");
            }
            //words = strtok (NULL,delimiter);
        }
#endif
        words = strtok (NULL,delimiter);
    }


    /* Red color text testing.*/
    //char stringPrepend[] = "<html><body> <p style=\"color:#FF0000\"\;>";
    //char strTest = "Poornima Testing";
    //char stringPost[] = "</body></html>";

    //char * finalString ;
    //if((finalString = malloc(strlen(stringPrepend)+strlen(stringPost)+strlen(strTest)+1)) != NULL){
    //    finalString[0] = '\0';   // ensures the memory is an empty string
    //    strcat(finalString, stringPrepend);
    //    strcat(finalString, strTest);
    //    strcat(finalString, stringPost);
    //} else {
    //    printf(" Error: malloc failed!\n");
    //    // exit?
    //}

    int sendRet1;
    //sendRet = send(clientfd2, readBuff2, strlen(readBuff2), 0);
    char testStr[] = "Testing this String varibale";
    char string2[] = "<html><body> <p style=\"color:#FF0000\";> Testing the labels in red:<var> testStr</var></p> </body></html>";
    //printf("address: %s\n", inet_ntoa(serveraddr));
    sendRet1 = send(clientfd2, firstline, strlen(firstline), 0);
    if (sendRet1 < 0) {
        printf ("Error Sending data: %d", sendRet1);
    }
    else {
        free (firstline); 
    }
    
    int sendRet2;
    sendRet2 = send(clientfd2, secondline, strlen(secondline), 0);
    if (sendRet2 < 0) {
        printf ("Error Sending data: %d", sendRet2);
    }
    //printf ("Sending Back to Client: \n %s\n", readBuff2);

    //sendRet = send(clientfd2, "\r\n", strlen("\r\n"), 0);
    //if (sendRet < 0) {
    //    printf ("Error Sending data: %d", sendRet);
    //}
  
    //free (secondline); 
    //free (thirdline); 

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
        red();
        printf ("Invalid Port: %s\n", argv[1]);
        reset();
        return(0);
    } 

    /* Create the Server Socket. */
    struct sockaddr_in server_sock;
    int socketfd = 0;
    int bindret = 0;
    //int clientfd = 0;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        printf ("Unable to create socket (Error code): %d\n", socketfd);
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
    }

    /* Listening on serverPort defined above. */
    listen(socketfd, numConnServer); 
  
    int counter = 0;
    while(1) {

        int clientfd = 0;
        /* Accept the connections from the clients. */
        clientfd = accept (socketfd, (struct sockaddr*)NULL, NULL);
        bzero(readBuff, readBuffSize);
        int n = 0;

        /* Receive the data from clients. */
        n = recv(clientfd, &readBuff, readBuffSize, 0);
        if (n < 0) {
            printf("ERROR receiving data from the socket\n");
        }
        
        /* Send the Code 200 to client for successful*/
        //int successRet;
        //successRet = sendSuccess200msg (clientfd);
        //if (successRet != 0 ) {
        //    printf ("Error in sending Success 200 code to client\n");
        //}

        //int readCount = read (recvfd, readBuff, 255);
        //if (readCount < 0) {
        //    printf("ERROR reading from socket\n");
        //}
        
        int parseRet;
        //counter++;
        //printf ("COunter is: %d", counter);
        parseRet = doParse (clientfd, readBuff, n);
        if (parseRet != 0) {
            printf ("Failed Parsing the received data\n");
        }
        close (clientfd); 
        
        //int writeCount = write (recvfd, writeBuff, writeBuffSize); 
        //if (writeCount < 0) {
        //    printf("ERROR Writing to socket\n");
        //}       

#if 0

        int counter =0;
        char * lines[1024];
        lines = strtok(readBuff,"\n");
        if(lines == NULL) {
            printf("%d\n", counter);
            counter++;
        }
        while(lines != NULL){                   // the `while` serves as `else`
            printf("%d ", counter);
            printf("%s\n" , lines);
            counter++;
            lines = strtok(NULL, "\n");
        }

        bzero(writeBuff, writeBuffSize);
        sprintf(writeBuff, "HTTP/1.0 200 OK\r\n");
        send(recvfd, writeBuff, strlen(writeBuff), 0);
    
        bzero(writeBuff, writeBuffSize);
        sprintf(writeBuff, "Content-Type: text/html\r\n");
        send(recvfd, writeBuff, strlen(writeBuff), 0);
    
        bzero(writeBuff, writeBuffSize);
        strcpy(writeBuff, "\r\n");
        send(recvfd, writeBuff, strlen(writeBuff), 0);
    
#endif


    }
 
}
//////////////////////////////////////////
    //while (readBuffLoc != NULL) {
    //    printf ("%s\n", partitioned);
    //}

    //if ((recvBytes > 0) && strcmp("\n", readBuffLoc)) {
    //    printf ("Entering the If newline condition \n");
    //    //printf ("%d\n", recvBytes);
    //    printf ("%s\n", readBuffLoc); 
    //}
    //else if ((recvBytes > 0) && strcmp("\r", readBuffLoc)) {
    //    printf ("Entering the Else newline condition \n");
    //    //printf ("%d\n", recvBytes);
    //    printf ("%s\n", readBuffLoc); 
    //}
