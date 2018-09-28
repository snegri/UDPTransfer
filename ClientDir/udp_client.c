#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#define MAXBUFSIZE 100

/* You will have to modify the program below */


void menuOutput() { //This function simply prints the menu
    printf(" ----------------\n");
    printf("| Put <filename> |\n");
    printf("| Get <filename> |\n");
    printf("| ls <filename>  |\n");
    printf("| Exit           |\n");
    printf(" ----------------\n");
}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remote;              //"Internet socket address structure"
    socklen_t rSize = sizeof(remote);
    
	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //AF_INET tells it to use ipv4 and we are using datagrams
	{
		printf("unable to create socket");
	}

	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
    
    char choice[MAXBUFSIZE];
    char receivedMessage[MAXBUFSIZE];
    int len = sizeof(remote);
    menuOutput();
    for (;;) {
        
        printf(">> ");
        scanf(" %[^\n]", choice);
        //printf("%s\n", choice);
        
        if (strcmp(choice, "ls") == 0) { //User chose ls
            if (sendto(sock, choice, sizeof(choice), 0, (struct sockaddr*) &remote, rSize) == -1) { //Send "ls" to server
                printf("Error sending to server...");
            }
            
            while((nbytes = recvfrom(sock, receivedMessage, MAXBUFSIZE, 0, (struct sockaddr*) &remote, &rSize)) > 0) { //loop over bytes recieved
                if (strcmp(receivedMessage, "END") == 0) {
                    break;
                }
                printf("%s", receivedMessage); //print ls output
            }
            continue;
        }
        else if ((strstr(choice, "Get ") != NULL) || (strstr(choice, "get ") != NULL)) { //User chose get
            char* name;
            if (sendto(sock, choice, sizeof(choice), 0, (struct sockaddr*) &remote, rSize) == -1) { //Send get to server
                printf("Error sending to server...");
            }
            name = strtok(choice, " ");
            name = strtok(NULL, " "); //isolate name of file in buffer
            //printf( "%s\n", name);
            FILE *getFile;
            int fp;
            fp = open(name, O_RDWR|O_CREAT, 0755); //open file for read/write with 755 permissions
            while ((nbytes = recvfrom(sock, receivedMessage, MAXBUFSIZE, 0, (struct sockaddr*) &remote, &rSize))) { //loop over recieved bytes
                receivedMessage[nbytes] = 0; //null terminate string (just in case)
                if (strcmp("END", receivedMessage) == 0) { //check for END flag, meaning server has reached the end of the file
                    break;
                }
                int writeBytes; //store bytes
                writeBytes = write(fp, receivedMessage, nbytes); //write incoming bytes to file
                //printf("%d\n", nbytes);
                //bzero(receivedMessage, MAXBUFSIZE);
            }
            printf("Successfully recieved %s\n", name); //Print success!

            continue;
        }
        
        else if ((strstr(choice, "Put ") != NULL) || (strstr(choice, "put ") != NULL)) { //user chose Put
            char temp[MAXBUFSIZE];
            if (sendto(sock, choice, sizeof(choice), 0, (struct sockaddr*) &remote, rSize) == -1) { //send put to the server so it knows to begin receiveing the desired file
                printf("Error sending to server...");
            }
            char receivedMessage[MAXBUFSIZE];
            recvfrom(sock, receivedMessage, MAXBUFSIZE, 0, (struct sockaddr*) &remote, &rSize); //recieve input based on whether or not the file was successfully opened
            //on the server side
            printf("%s\n", receivedMessage);
            if (strcmp(receivedMessage, "SUCCES") != 0) { //check that it was opened successfully
                int bytes;
                char* token;
                token = strtok(choice, " "); //isolate name in "put ..." char buffer
                token = strtok(NULL, " ");
                //printf( "%s\n", token); //check
                int fp2;
                fp2 = open(token, O_RDONLY); //open new file for reading
                int sentBytes;
                while ((bytes = read(fp2, temp, MAXBUFSIZE)) > 0) { //read bytes out of buffer and send to server
                    sentBytes = sendto(sock, temp, bytes, 0, (struct sockaddr*) &remote, rSize);
                    printf("Bytes sent: %d\n", sentBytes);
                    bzero(temp, MAXBUFSIZE); //zero out the buffer just in case
                }
                char msg[] = "END"; //send end to let server know client is done sending
                nbytes = sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
                printf("Successfully sent file :)\n");
                
            }
            else {
                printf("Failed to open file.");
                continue;
            }
        }
        
        else if (strcmp(choice, "-help") == 0) { //-help for menu output
            menuOutput();
            continue;
        }
        else if (strcmp(choice, "Exit") == 0) { //user chose exit
            if (sendto(sock, choice, sizeof(choice), 0, (struct sockaddr*) &remote, rSize) == -1) { //send exit to server to tell it to exit
                printf("Error sending to server...");
            }
            
            if (recvfrom(sock, receivedMessage, MAXBUFSIZE, 0, (struct sockaddr*) &remote, &rSize) == -1) { //server sends closing connection...
                printf("Error receiving from server...\n");
            }
            printf("%s\n", receivedMessage);
            exit(1); //exit the loop and code
        }
        
        else if ((strstr(choice, "Delete ") != NULL) || (strstr(choice, "delete ") != NULL)) { // send whole delete char buffer to server so it can delete said file
            if (sendto(sock, choice, sizeof(choice), 0, (struct sockaddr*) &remote, rSize) == -1) {
                printf("Error sending to server...");
            }
        }
        
        
        
    }
    
	close(sock);

}

