/* remoteClient.c: client to the defined server.
    Communicates to server via the protocol in 3 phases.
    Requests a directory in the server tree,
    recursively gets all files and dirs in the tree
    and copies them locally.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>         /* for access */
#include <dirent.h>         /* for *dir functions */
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>     /* for socket handling */
#include <netinet/in.h>     /* for sockaddr_in */
#include <arpa/inet.h>      /* for hton */
#include <ctype.h>          /* for isalnum */


#include "remoteClient.h"

/* Print usage */
void usage(char *exec_name) {
    fprintf(stderr, "Usage: %s -i <server_ip> -p <server_port> " \
                    "-d <dir>\n", exec_name);
    exit(EXIT_FAILURE);
}

/* Helper that makes the dirs on a given path on cwd */
void make_dir_path(char file_path[MAXFILENAME]) {

    int depth_count = 0, i;
    char *dir_token = "/";
    char *helper;
    char curr_full[MAXFILENAME] = { "" };

    /* Parse once to get the count */
    for (char *c = file_path; *c != '\0'; ++c) {
        if (*c == '/')
            depth_count++;
        //fprintf(stderr, "%d\n", depth_count);
    }

    /* Tokenize and as you go create dirs */
    for (helper = file_path, i = 0; i < depth_count; ++i, helper = NULL) {
        char *curr;

        /* Extract current token to curr */
        curr = strtok(helper, dir_token);

        if (curr == NULL)
            break;

        /* Recreate the path */
        strcat(curr_full, curr);
        strcat(curr_full, "/");

        DIR* dir = opendir(curr_full);

        if (dir) {
            closedir(dir);
        }
        else if (ENOENT == errno) {
            // if it doesn't exist, create it
            mkdir(curr_full, S_IRWXU);
        }

        //fprintf(stderr, "Making dir: %s\n", curr_full);

    }
}

/* Facilitates Handshake phase of the protocol a la B.A.T.M.A.N */
int proto_cli_phase_one(int sock) {

    /* (1) Recv "hello" handshake */
    //fprintf(stderr, "[remoteClient] Waiting server handshake...");

    char handshake[6];

    recv(sock, handshake, 6, 0);

    if (strncmp(handshake, "hello", 5) != 0) {
        perror("[remoteClient] recv() handshake");
        fprintf(stderr, "%s\n", handshake);
        return -1;
    }

    /* (2) Send "hello" handshake */
    char *message = "hello";
    send(sock, message, strlen(message), 0);

    //fprintf(stderr, "[remoteClient] Sent handshake...\n");

    return 0;
}

/* Facilitates noisy session phase 2 exchanging data with the server
    Must rACK after send and wACK after recv */
int proto_cli_phase_two(int sock, int *blk_sz, int *file_cnt, char *dir) {

    int block_size, read_size, congest, file_count;

    /* (1) Recv block size */
    read_size = recv(sock, &congest, sizeof(congest), 0);

    if (read_size <= 0) {
        perror("[remoteClient] recv() block size");
        return -1;
    }

    block_size = ntohs(congest);

    /* wACK */
	wACK(sock);

    fprintf(stderr, "[remoteClient] Received block size = %d\n", block_size);
    *blk_sz = block_size;

    /* (2) Write directory to fetch */
    send(sock, dir, strlen(dir) + 1, 0);

    /* rACK */
    if (rACK(sock) != 0) {
        perror("[remoteClient] rACK() directory");
        return -1;
    }

    /* (3) Read file_count */
    read_size = recv(sock, &congest, sizeof(congest), 0);

    if (read_size <= 0) {
        perror("[remoteClient] recv() file count");
        return -1;
    }

    file_count = ntohs(congest);

    /* wACK */
	wACK(sock);

    fprintf(stderr, "[remoteClient] Received file_count = %d\n", file_count);
    *file_cnt = file_count;

    return 0;
}

/* Facilitates files phase 3 of the protocol.
    For each file: creates its path, receives its data and copies them in */
int proto_cli_phase_three(int sock, int files, int block_size) {

    int congest, read_size, file_size;

    /* FILES */
    for (int file = 0; file < files; ++file) {

        /* (1) Read "FILE" handshake */
        char handshake[5], *message;

        recv(sock, handshake, 5, 0);

        if (strncmp(handshake, "FILE", 5) != 0) {
            perror("[remoteClient::phase_three] recv() \"FILE\" handshake");
            fprintf(stderr, "%s\n", handshake);
            return -1;
        }

        /* (2) Send confirmation "ELIF" */
        message = "ELIF";
        send(sock, message, strlen(message) + 1, 0);

        /* (3) Receive metadata
                (a) file path
                (b) file metadata
        */
        char buffer[block_size], file_path[block_size];

        /* file path */
	    read_size = recv(sock, buffer, MAXFILENAME, 0);
        //fprintf(stderr, "%d\n", read_size);

        sanitize(buffer);

        // Transfer
        sprintf(file_path, "%s", buffer);

        /* wACK */
    	wACK(sock);

        /* file count */
        read_size = recv(sock, &congest, sizeof(congest), 0);

        if (read_size <= 0) {
            perror("[remoteClient] recv() file count");
            return -1;
        }

        file_size = ntohs(congest);

        /* wACK */
    	wACK(sock);

        /* (4) Make path for the file */
        char path[MAXFILENAME];
        strncpy(path, file_path, strlen(file_path) + 1);
        make_dir_path(path);

        /* (5) Create file */
        FILE *fp;
        if (access(file_path, F_OK) == 0) {

            //fprintf(stderr, "trying to remove file %s\n", file_path);

            /* if it exists: remove and create */
            if (remove(file_path) == -1) {
                perror("[remoteClient]: remove() file path");
                return -1;
            }

            /* with the "w" flag, it creates the file */
            fp = fopen(file_path, "w");

            if (fp == NULL) {
                perror("[remoteClient]: failed to create file");
                return -1;
            }
        }
        else {
            // if it doesn't: just create
            // with "w" it creates
            fp = fopen(file_path, "w");

            if (fp == NULL) {
                perror("[remoteClient]: failed to create file");
                return -1;
            }
        }

        /* (6) Receive file, block-by-block */

        int packets;
        /* Check whether packets fit perfectly on block size */
        if ((file_size % (block_size - 1)) == 0) {
            packets = file_size / (block_size - 1);
        }
        else {
            packets = (file_size / (block_size - 1)) + 1;
        }

        /* Recv packets and store each in the file */
        for (int packet = 0; packet < packets; ++packet) {

            char data[block_size];

            recv(sock, buffer, block_size, 0);

            snprintf(data, block_size, "%s", buffer);

            //fprintf(stderr, "Received packet %d, containing: %s\n", packet, data);

            fputs(data, fp);

        }

        fprintf(stderr, "Received: %s\n", file_path);

        fclose(fp);

        /* (7) Read "ELIF" end of file */
        recv(sock, handshake, 5, 0);

        if (strncmp(handshake, "ELIF", 5) != 0) {
            perror("[remoteClient::phase_three] recv() \"ELIF\" handshake");
            fprintf(stderr, "%s\n", handshake);
            return -1;
        }
    }

    close(sock);

    return 0;
}

/* remoteClient: Look at line 1 */
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
    fprintf(stderr, "[PHASE 1]: ");

    if (proto_cli_phase_one(sock) == -1) {
        perror("[remoteClient] Protocol Phase 1 failure!");
        exit(EXIT_FAILURE);
    }
    else {
		fprintf(stderr, "SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 1 END ~~~~~~~~~*/

    /* ~~~~~~~~~ PHASE 2: SESSION DATA ~~~~~~~~~ */
    fprintf(stderr, "[PHASE 2]: START\n");

    if (proto_cli_phase_two(sock, &block_size, &file_count, dir) == -1) {
        perror("[remoteClient] Protocol Phase 2 failure!");
        exit(EXIT_FAILURE);
    }
    else {
		fprintf(stderr, "[PHASE 2]: SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 2 END ~~~~~~~~~ */

    /* ~~~~~~~~~ PHASE 3: FILES ~~~~~~~~~ */
    fprintf(stderr, "[PHASE 3]: START\n");

    if (proto_cli_phase_three(sock, file_count, block_size) == -1) {
        perror("[remoteClient] Protocol Phase 3 failure!");
        close(sock);
        exit(EXIT_FAILURE);
    }
    else {
		fprintf(stderr, "[PHASE 3]: SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 3 END ~~~~~~~~~ */

    /* ~~~~~~~~~~~~~~~~~~ END PROTOCOL ~~~~~~~~~~~~~~~~~~ */
    return 0;


}