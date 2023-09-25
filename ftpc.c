#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFSIZE 1

/* Author: Patrick Suijk
 * this program is the client side in a tcp communication between two computers where
 * the client sends a file to the server. The program takes an IP address and a port number
 * as command lines arguments then prompts the user for a file name. It then sends the size
 * of that file name to the server, followed by the file name itself, then the size of the file,
 * then the bytes from the file itself. Once complete, it waits for acknolwedge men from the server
 * to make sure the correct number of bytes were received. It then prompts the user for a new file name
 * to send. If the user enters "DONE" as the file name, the program exits. */
int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in server_address;
    int portNumber;
    char serverIP[29];
    int rc = 0;
    char* filename = calloc(100, sizeof(char)); //set filename to size 20 chars
    char stop[5] = "DONE";
    FILE* inputfile;
    int fileSize;

    if (argc < 3){
        printf ("usage is client <ipaddr> <port>\n");
        exit(1);
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);

    portNumber = strtol(argv[2], NULL, 10);
    strcpy(serverIP, argv[1]);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = inet_addr(serverIP);

    if(connect(sd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in)) < 0) {
        close(sd);
        perror("error connecting stream socket");
        exit(1);
    }

    printf("What is the name of the file you'd like to send?");
    scanf("%s", filename);

    //while the file name given by the user is not "DONE", loop
    while(strcmp(stop, filename)) {
        printf("filename to send is '%s'\n", filename);
        int sizeOfFilename = strlen(filename);
        int converted_sizeOfFilename = htonl(sizeOfFilename); //convert to network order

        //send size of filename
        rc = write (sd, &converted_sizeOfFilename, sizeof(sizeOfFilename));
        printf("wrote %d bytes to send the size of filename\n", rc);
        if (rc < 0) {
            perror("write");
            exit(1);
        }

        //send filename
        rc = write (sd, filename, strlen(filename));
        printf("wrote %d bytes to send the filename\n", rc);
        if (rc < 0) {
            perror("write");
            exit(1);
        }

        //try to open file
        if ((inputfile = fopen(filename, "rb")) == NULL) {
            printf("Error opening the input file '%s\n", filename);
            exit(1);
        }

        //get file size
        fseek(inputfile, 0, SEEK_END); //set fp to end of file
        fileSize = ftell(inputfile); //gets number of bytes that make up file
        fseek(inputfile, 0 , SEEK_SET); //set pointer back to beginning of file

        printf("the filesize is %d bytes\n", fileSize);

        int converted_fileSize = htonl(fileSize); //convert to network order

        //send fileSize
        rc = write (sd, &converted_fileSize, sizeof(converted_fileSize));
        printf("wrote %d bytes to send the fileSize\n", rc);
        if (rc < 0) {
            perror("write");
            exit(1);
        }

        int totalBytesSent = 0;
        int numBytesRead = 0;
        unsigned char *buffer = calloc(1, sizeof(char));

        //send file to server
        while (totalBytesSent < fileSize) {
            numBytesRead = fread(buffer, 1, 1, inputfile);
            if (numBytesRead != 1) {
                perror("read");
                exit(1);
            }

            rc = write (sd, buffer, 1);
            printf("in loop, wrote %d bytes to send file\n", rc);
            if (rc < 0) {
                perror("write");
                exit(1);
            }
            totalBytesSent += rc;
        }

        rc = fclose(inputfile);
        if (rc<0) {
            perror("close");
            exit(1);
        }

        //read acknowledgement of bytes received from server
        int totalBytesServer = 0;
        rc = read(sd, &totalBytesServer, sizeof(int));
        if (rc < 0) {
            perror("read");
            exit(1);
        }
        int converted_total = ntohl(totalBytesServer); //convert to host order
        printf("server received a total of %d out of the %d bytes sent\n", converted_total, totalBytesSent);
        if (converted_total != totalBytesSent) {
            printf("error: server did not receive the correct number of bytes");
            exit(1);
        }

        //clean up and prepare for next file
        free(filename);
        filename = calloc(100, sizeof(char));
        free(buffer);
        buffer = calloc(1, sizeof(char));
        printf("What is the name of the file you'd like to send?");
        scanf("%s", filename);
    }

    //final cleanup
    close(sd);
    free(filename);
    printf("exiting client... \n");

    return 0 ;
}
