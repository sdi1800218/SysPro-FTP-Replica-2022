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
int rACK(int sock) {
    char ack[4];
    read(sock, &ack, sizeof(ack));

    if (strncmp(ack, "ACK", 3) != 0) {
        perror("[remoteClient] read() ACK");
        fprintf(stderr, "%s\n", ack);
        return -1;
    }

    return 0;
}

/* Helper that sends ACK messages */
void wACK(int sock) {
    char *ack = "ACK";

    send(sock, ack, strlen(ack) + 1, 0);
}

/* Facilitates Handshake phase of the protocol a la B.A.T.M.A.N */
int proto_cli_phase_one(int sock) {

    /* (1) Read "hello" handshake */
    //fprintf(stderr, "[remoteClient] Waiting server handshake...");

    char handshake[6];

    read(sock, handshake, 6);

    if (strncmp(handshake, "hello", 5) != 0) {
        perror("[remoteClient] read() handshake");
        fprintf(stderr, "%s\n", handshake);
        return -1;
    }

    /* (2) Write "hello" handshake */
    char *message = "hello";
    send(sock, message, strlen(message), 0);

    //fprintf(stderr, "[remoteClient] Sent handshake...\n");

    return 0;
}

/* Facilitates noisy session phase 2 exchanging data with the server
    Must rACK after write and wACK after read */
int proto_cli_phase_two(int sock, int *blk_sz, int *file_cnt, char *dir) {

    int block_size, read_size, congest, file_count;

    /* (1) Read block size */
    read_size = read(sock, &congest, sizeof(congest));

    if (read_size <= 0) {
        perror("[remoteClient] read() block size");
        return -1;
    }

    block_size = ntohs(congest);

    /* wACK */
	wACK(sock);

    fprintf(stderr, "[remoteClient] Received block size = %d\n", block_size);
    *blk_sz = block_size;

    /* (2) Write directory to fetch */
    send(sock, dir, block_size, 0);

    /* rACK */
    if (rACK(sock) != 0) {
        perror("[remoteClient] rACK() directory");
        return -1;
    }

    /* (3) Read file_count */
    read_size = read(sock, &congest, sizeof(congest));

    if (read_size <= 0) {
        perror("[remoteClient] read() file count");
        return -1;
    }

    file_count = ntohs(congest);

    /* wACK */
	wACK(sock);

    fprintf(stderr, "[remoteClient] Received file_count = %d\n", file_count);
    *file_cnt = file_count;

    return 0;
}

int proto_cli_phase_three(int sock, int files, int block_size) {
    for (int file = 0; file < files; ++file) {

        /* (1) Read "FILE" handshake */

        /* (2) Receive metadata
                (a) file path
                (b) file metadata
        */
        meta metadata;
        read(sock, &metadata, sizeof(metadata));

        fprintf(stderr, "Read metadata with values: %s\n", metadata.file_path);

        /* (3) Make path for the file */

        /* (4) Receive file, block-by-block */
        /* TODO */

        /* (5) Read "ELIF" end of file */

    }

    close(sock);

    return 0;
}

/* remoteClient: TODO */
int main(int argc, char *argv[]) {

    int opt, sock, port;
    int block_size, file_count;
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
    fprintf(stderr, "serverIP: %s\n", addr);
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
    if (connect(sock, serverptr, sizeof(server)) == -1) {
        perror("[remoteClient] Socket connect() failure!");
        exit(EXIT_FAILURE);
    }
    else {
	    fprintf(stderr, ("[remoteClient] Connected\n"));
    }

    /* ~~~~~~~~~~~~~~~~~~ BEGIN PROTOCOL ~~~~~~~~~~~~~~~~~~ */

    /* ~~~~~~~~~ PHASE 1: HELLO HANDSHAKE ~~~~~~~~~ */
    fprintf(stderr, "[PHASE 1]:\t");

    if (proto_cli_phase_one(sock) == -1) {
        perror("[remoteClient] Protocol Phase 1 failure!");
        exit(EXIT_FAILURE);
    }
    else {
		fprintf(stderr, "SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 1 END ~~~~~~~~~*/

    /* ~~~~~~~~~ PHASE 2: SESSION DATA ~~~~~~~~~ */
    fprintf(stderr, "[PHASE 2]:\n");

    if (proto_cli_phase_two(sock, &block_size, &file_count, dir) == -1) {
        perror("[remoteClient] Protocol Phase 2 failure!");
        exit(EXIT_FAILURE);
    }
    else {
		fprintf(stderr, "SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 2 END ~~~~~~~~~ */

    /* ~~~~~~~~~ PHASE 3: FILES ~~~~~~~~~ */
    fprintf(stderr, "[PHASE 3]:\n");

    if (proto_cli_phase_three(sock, file_count, block_size) == -1) {
        perror("[remoteClient] Protocol Phase 3 failure!");
        exit(EXIT_FAILURE);
    }
    else {
		fprintf(stderr, "SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 3 END ~~~~~~~~~ */

    /* ~~~~~~~~~~~~~~~~~~ END PROTOCOL ~~~~~~~~~~~~~~~~~~ */
    return 0;

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
}