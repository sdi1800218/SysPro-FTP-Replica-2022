/* remoteClient.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>     /* for socket handling */
#include <netinet/in.h>     /* for sockaddr_in */
#include <arpa/inet.h>      /* for hton */

#include "remoteClient.h"

/* Print usage */
void usage(char *exec_name) {
    fprintf(stderr, "Usage: %s -i <server_ip> -p <server_port> " \
                    "-d <dir>\n", exec_name);
    exit(EXIT_FAILURE);
}

/* Helper that waits for ACK message */
int read_ack(int sock) {
    char ack[4];
    read(sock, &ack, sizeof(ack));

    if (strncmp(ack, "ACK", 3) == 0) {
        fprintf(stderr, "\tSUCCESS\n");
    }
    else {
        perror("[remoteClient] read() handshake");
        fprintf(stderr, "%s\n", ack);
        return -1;
    }

    return 0;
}

/* Helper that sends ACK messages */
void write_ack(int sock) {
    char *ack = "ACK";

    write(sock, ack, strlen(ack) + 1);
}

/* Manager: creates a pipe and a listener, handles the workers */
int main(int argc, char *argv[]) {

    int opt, sock, port;
    int block_size, read_size;
    char *dir, *addr;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *)&server;
    //struct hostent *rem;

    /* Assert correct number of cmd arguments */
    if (argc != 7) {
        usage(argv[0]);
    }

    /* Parse cmd arguments */
    while ((opt = getopt(argc, argv, "i:p:d:")) != -1) {
        switch (opt) {
        case 'i':
            addr = optarg;
            server.sin_addr.s_addr = inet_addr(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            dir = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }

    fprintf(stderr, "Client parameters are:\n");
    fprintf(stderr, "serverIP: %s\n",addr);
    fprintf(stderr, "port: %d\n", port);
    fprintf(stderr, "directory: %s\n", dir);

    /* Make-a-socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
        perror("[remoteClient] Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Connecting to %s on port %d\n", addr, port);

    /* Find server a d d r e s s */
    //if (( rem = gethostbyname(addr) ) == NULL) {
    //    herror("gethostbyname");
    //    exit(1);
    //}

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    //memcpy (&server.sin_addr, rem->h_addr, rem->h_length) ;

    /* Connect to remote server */
    if (connect(sock, serverptr, sizeof(server))) {
        perror("[remoteClient] Socket connect() failure!");
        exit(EXIT_FAILURE);
    }
    else {
	    fprintf(stderr, ("[remoteClient] Connected\n"));
    }

    /* (1) Read "hello" handshake */
    fprintf(stderr, "[remoteClient] Waiting server handshake...");

    char handshake[6];

    read_size = read(sock, handshake, 6);

    if (strncmp(handshake, "hello", 5) == 0) {
        fprintf(stderr, "\tSUCCESS\n");
    }
    else {
        perror("[remoteClient] read() handshake");
        fprintf(stderr, "%s\n", handshake);
        exit(EXIT_FAILURE);
    }

    /* (2) Write "hello" handshake */
    char *message = "hello";
    write(sock, message, strlen(message));

    fprintf(stderr, "[remoteClient] Sent handshake...\n");

    /* (3) Read block size */
    int helper;
    read_size = read(sock, &helper, sizeof(helper));

    if (read_size <= 0) {
        perror("[remoteClient] read() block size");
    }

    block_size = ntohs(helper);

    fprintf(stderr, "[remoteClient] Received block size = %d\n", block_size);

    /* (4) Write directory to fetch */
    write(sock, dir, block_size);

    read_ack(sock);

    /* (5) Receive metadata
            (a) file path
            (b) file metadata
    */
    meta metadata;
    read(sock, &metadata, sizeof(metadata));

    fprintf(stderr, "Read metadata with values: %s\n", metadata.file_path);

    write_ack(sock);

    /* (6) Receive file, block-by-block */
    // TODO
/*
    // Assert path exists
    if (access(path, F_OK) != 0) {
        perror("[Manager::access] Path does not exist!");
        exit(EXIT_FAILURE);
    }

    // Assert path is RW
    if (access(path, R_OK|W_OK) != 0) {
        perror("[Manager::access] Path is not R/W!");
        exit(EXIT_FAILURE);
    }

    //close(listen[WRITE]);
    //dup2(listen[READ], 0);
*/
    return 0;
}