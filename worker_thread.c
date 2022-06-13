#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>         /* for signal */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>		/* for stat */
#include <sys/socket.h> 	/* for recv */
#include <arpa/inet.h>      /* for hton */
#include <pthread.h>        /* I am a thread duh */

#include "dataServer.h"

/* Worker Thread: Takes care of Phase 3 of the protocol.
    Is called via the thread_pool by being programmed as a task.
    Reads the requested file and returns it to the client via the open socket.
*/
void *worker_thread(void *arg) {

    int sock, congest, block_size;
    char handshake[5], *message, file_path[MAXFILENAME];
    //fprintf(stderr, "[Thread: %lu]: I am a Worker Thread!!!\n", pthread_self());

    /* Ignore SIGINT */
    //signal(SIGINT, SIG_IGN);

    pkg2 paketo = *(pkg2 *)arg;
    fprintf(stderr, "[Thread: %lu]: Received task: <%s, %d>\n",
                    pthread_self(), paketo.filename, paketo.sock);

    /* Open the paketo */
    sock = paketo.sock;
    block_size = paketo.block_size;
    strncpy(file_path, paketo.filename, strlen(paketo.filename) + 1);

    /* (1) Lock the socket mutex here */
    pthread_mutex_lock(paketo.socket_mutex);

    /* (2) Send "FILE" handshake */
    message = "FILE";
    send(sock, message, strlen(message) + 1, 0);

    /* (3) Receive confirm "ELIF" */
    recv(sock, handshake, 5, 0);

	if (strncmp(handshake, "ELIF", 5) != 0) {
        perror("[comms_thread::phase_one] recv() handshake");
        fprintf(stderr, "%s\n", handshake);
	}

    /* (3) Send file path and file metadata (both on meta struct) */
    send(sock, file_path, strlen(file_path) + 1, 0);

    /* rACK */
    if (rACK(sock) != 0) {
        perror("[remoteClient] rACK() directory");
    }

    struct stat sb;
    if (stat(paketo.filename, &sb) == -1) {
        perror_exit("[worker_thread] stat()");
    }

    congest = htons(sb.st_size);
	send(sock, &congest, sizeof(congest), 0);

    /* rACK */
    if (rACK(sock) != 0) {
        perror("[remoteClient] rACK() directory");
    }

    //fprintf(stderr, "[Thread: %lu]: Sending file metadata\n", pthread_self());

    /* (4) Begin sending file block-by-block */
    FILE *fp = fopen(file_path, "r");

    int packets;
    /* Check whether packets fit perfectly on block size */
    if ((sb.st_size % (block_size - 1)) == 0 ) {
        packets = sb.st_size / (block_size - 1) ;
    }
    else {
        packets = (sb.st_size / (block_size - 1)) + 1;
    }

    fprintf(stderr, "[Thread: %lu]: About to read file %s\n", pthread_self(), file_path);

    for (int packet = 0; packet < packets; ++packet) {

        char block[block_size];

        fgets(block, block_size, fp);

        send(sock, block, block_size, 0);

    }

    fclose(fp);

    /* (5) Send "ELIF" end of file */
    message = "ELIF";
    send(sock, message, strlen(message) + 1, 0);

    /* (6) Unlock the socket mutex now */
    pthread_mutex_unlock(paketo.socket_mutex);

    //fprintf(stderr, "[Thread: %lu]: FINISHED!!!\n", pthread_self());

    return 0;
}
