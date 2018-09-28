#include <sys/types.h>
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
#include <string.h>
#include<sys/wait.h>

/* You will have to modify the program below */

#define MAXBUFSIZE 100

int main (int argc, char * argv[] )
{


	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
    socklen_t rSize = sizeof(remote);
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //use ipv4 protocol and using datagrams
	{
		printf("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	//remote_length = sizeof(remote);

	//waits for an incoming message
    
    char temp[MAXBUFSIZE];
    void* fileBuffer = &temp;
    int fp;
    while(1) {
        bzero(buffer,sizeof(buffer)); //reset buffer each iteratiom
        nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr*) &remote, &rSize); //recieve user input on from client
    
        //printf("The client says %s\n", buffer);
        
        if ((strcmp(buffer, "Exit") == 0) || (strcmp(buffer, "exit") == 0)) { //user says exit, send back closing connection, and exit
            char msg[] = "Closing connection...";
            nbytes = sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
            close(sock);
            exit(1);
        }
        
        
        else if ((strstr(buffer, "Get ") != NULL) || (strstr(buffer, "get ") != NULL)) { //user chose get
            int bytes;
            char* token;
            token = strtok(buffer, " ");
            token = strtok(NULL, " "); //isolate the name of the file to get
            printf( "%s\n", token);
            fp = open(token, O_RDONLY); //open that file for reading
            int sentBytes;
            while ((bytes = read(fp, temp, MAXBUFSIZE)) > 0) { //read bytes out of file into temp
                sentBytes = sendto(sock, temp, bytes, 0, (struct sockaddr*) &remote, rSize); //send bytes from temp
                //printf("Bytes sent: %d\n", bytes);
                bzero(temp, MAXBUFSIZE); //reset buffer
            }
            char msg[] = "END"; //send end so server knows when to stop waiting for data
            nbytes = sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
        }
        
        else if ((strstr(buffer, "Put ") != NULL) || (strstr(buffer, "put ") != NULL)) { //use chose put
            char* name;
            name = strtok(buffer, " ");
            name = strtok(NULL, " "); //isolate name of file to put
            printf( "%s\n", name);
            FILE *putFile;
            int fp;
            fp = open(name, O_RDWR|O_CREAT, 0755); //open new file with filename for reading and writing with permissions 755
            if (fp < 0) { //error check
                char msg[] = "-1";
                sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
                continue;
            }
            else {
                char msg[] = "SUCCESS"; //let client know to start sending
                sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
            }
            char receivedFile[MAXBUFSIZE];
            while ((nbytes = recvfrom(sock, receivedFile, MAXBUFSIZE, 0, (struct sockaddr*) &remote, &rSize))) { //loop over recieved input
                receivedFile[nbytes] = 0;
                if (strcmp("END", receivedFile) == 0) { //check if client is that the end of the file
                    break;
                }
                int writeBytes;
                writeBytes = write(fp, receivedFile, nbytes); //write input from buffer into file
                //printf("%s", receivedFile);
                //bzero(receivedMessage, MAXBUFSIZE);
            }
            printf("Successfully recieved %s\n", name); //show success
            continue;
        }
        
        else if (strcmp(buffer, "ls") == 0) { //user chose ls
            FILE *file;
            char lsOutput[MAXBUFSIZE];
            char lsFinal[MAXBUFSIZE];
            file = popen("ls", "r"); //us p open to run ls command on server and mark it for reading
            if (file == NULL) {
                printf("Failed to run command\n" );
                continue;
            }
            while (fgets(lsOutput, sizeof(lsOutput)-1, file) != NULL) { //loop over ls ouput and store in lsOutput
                nbytes = sendto(sock, lsOutput, sizeof(lsOutput), 0, (struct sockaddr*) &remote, rSize); //send data from lsOutput to client
                bzero(lsOutput, MAXBUFSIZE); //reset buffer
            }
            char msg[] = "END";
            nbytes = sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
            //close(file);
        }
        
        else if ((strstr(buffer, "Delete ") != NULL) || (strstr(buffer, "delete ") != NULL)) { //client chose delete
            char *deleteName;
            deleteName = strtok(buffer, " ");
            deleteName = strtok(NULL, " "); //isolate file name
            //printf( "%s\n", deleteName);
            int check = remove(deleteName); //use c lib function remove to delete file
            if (check == -1) { //error check
                char msg[] = "Failed to delete file. Sorry :(";
                sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
                continue;
            }
            char msg[] = "Successfully deleted!"; //send success
            sendto(sock, msg, sizeof(msg), 0, (struct sockaddr*) &remote, rSize);
        }
        
    }

	close(sock);
}

