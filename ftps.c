#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFSIZE 1000


/* Author: Patrick Suijk
 * This program acts a the server side in a tcp communication between two computers
 * sharing a file from client to server. This program tkaes a port number as a commandline argument.
 * First the server reads the size of the file name,
 * the the file name itself, then the size of the file, and finally the file itself. The
 * server writes the bytes from this file to a new file on disk called 'new_filename' and then sends
 * the amount of bits read back to the client for confirmation. Once this is complete, the server
 * goes back to listen for a new connection. */
int main(int argc, char *argv[]) {
    int sd; /* socket descriptor */
    int connected_sd; /* socket descriptor */
    int rc; /* return code from recvfrom */
    struct sockaddr_in server_address;
    struct sockaddr_in from_address;
    char buffer[BUFFSIZE];
    socklen_t fromLength;
    int portNumber;
    FILE* outputfile;
    char* newFileName;

    if (argc < 2){
        printf ("Usage is: server <portNumber>\n");
        exit (1);
    }

    portNumber = atoi(argv[1]);
    sd = socket (AF_INET, SOCK_STREAM, 0);

    fromLength = sizeof(struct sockaddr_in);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = INADDR_ANY;

    rc = bind (sd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (rc < 0) {
        perror("bind");
        exit(1);
    }

    while (1) {
        printf("listening... \n");
        listen (sd, 5);
        connected_sd = accept (sd, (struct sockaddr *) &from_address, &fromLength);
        memset(buffer, 0 , 1000);

        //read size of file name
        int sizeOfFileName = 0;
        rc = read(connected_sd, &sizeOfFileName, sizeof(int));
        if (rc < 0) {
            perror("read");
            exit(1);
        } else if (rc == 0) {
            continue;
        }
        printf("read %d bytes to get the size of filename\n", rc);
        printf("the size of the filename before converting is %d bytes\n", sizeOfFileName);
        sizeOfFileName = ntohl(sizeOfFileName);
        printf("the size of the filename after converting is %d bytes\n", sizeOfFileName);

        //allocate memory for filename
        char* fileName;
        fileName = calloc(sizeOfFileName, sizeof(char));

        //read file name
        int totalBytes = 0;
        char *ptr = fileName;
        while (totalBytes < sizeOfFileName) {
            rc = read(connected_sd, ptr, sizeOfFileName - totalBytes);
            if (rc <=0) {
                perror("read");
                exit(1);
            } else if (rc == 0) {
                free(fileName);
                continue;
            }
            printf("in loop, read %d bytes\n", rc);
            totalBytes += rc;
            ptr += rc;
        }
        printf("read %d bytes to get the filename\n", rc);
        printf("the filename is %s\n", fileName);
        newFileName = calloc(5 + sizeOfFileName, sizeof(char));
        strcpy(newFileName, "new_");
        strcat(newFileName, fileName);

        //read size of file
        int sizeOfFile = 0;
        rc = read(connected_sd, &sizeOfFile, sizeof(int));
        if (rc < 0) {
            perror("read");
            exit(1);
        } else if (rc == 0) {
            free(fileName);
            continue;
        }
        printf("read %d bytes to get the size of file\n", rc);
        printf("the size of the file before converting is %d bytes\n", sizeOfFile);
        int converted_fileSize = ntohl(sizeOfFile); //convert to host order
        sizeOfFile = converted_fileSize;
        printf("the size of the file after converting is %d bytes\n", converted_fileSize);

        //try to open file
        if ((outputfile = fopen(newFileName, "w")) == NULL) {
            printf("Error opening the output file '%s\n", newFileName);
            exit(1);
        }

        //read file
        int numBytesRead = 0;
        totalBytes = 0;
        ptr = buffer; //reset ptr
        while (totalBytes < sizeOfFile) {
            rc = read (connected_sd, ptr, sizeOfFile - totalBytes);
            if (rc < 0) {
                perror("read");
                exit(1);
            } else if (rc == 0) {
                free(fileName);
                continue;
            }
            printf("in loop, read %d bytes\n", rc);
            numBytesRead = rc;
            totalBytes += rc;

            rc = fwrite(buffer, 1, numBytesRead, outputfile);
            if (rc != numBytesRead) {
                perror("fwrite");
                exit(1);
            }
            printf("in loop, wrote %d bytes to file\n", rc);
            ptr = buffer;
        }

        //send total bytes read to client
        int converted_totalBytes = htonl(totalBytes);
        rc = write(connected_sd, &converted_totalBytes, sizeof(converted_totalBytes));
        if (rc < 0) {
            perror("write");
            exit(1);
        }

        rc = fclose(outputfile);
        if (rc < 0) {
            perror("close");
            exit(1);
        }
    }
    return 0;
}
