#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pthread.h>    /* I am a thread duh */

#include "dataServer.h"

void cont_handler(int);
void kill_handler(int);

/* SIGCONT Handler */
/*
void cont_handler(int sig) {
    printf("\t[Worker::Handler] I'm continuing now!\n");
    return;
}
*/

/* Helper that waits for ACK message */
int read_ack(int sock) {
    char ack[4];
    read(sock, &ack, sizeof(ack));

    if (strncmp(ack, "ACK", 3) == 0) {
        fprintf(stderr, "\tSUCCESS\n");
    }
    else {
        perror("[remoteClient] recv() handshake");
        fprintf(stderr, "%s\n", ack);
        return -1;
    }

    return 0;
}

void write_ack(int sock) {
    char *ack = "ACK";

    write(sock, ack, strlen(ack) + 1);
}

/* SIGKILL Handler */
void kill_handler(int sig) {
    printf("\t[Worker::Handler] Received a SIGKILL!\n");
    printf("\t[Worker::Handler] Shuting down..");
    exit(EXIT_SUCCESS);
}

/* Simple worker, uses low level IO to extract urls from files.
        Operates via read()ing a named pipe */
void *child_worker(void *arg) {

    int sock;
    //fprintf(stderr, "[Thread: %lu]: I am a Worker Thread!!!\n", pthread_self());

    pkg2 paketo = *(pkg2 *)arg;
    fprintf(stderr, "[Thread: %lu]: Received task: <%s, %d>\n",
                    pthread_self(), paketo.filename, paketo.sock);

    /* Open the paketo */
    sock = paketo.sock;

    /* TODO: Lock the mutex here */

    write_ack(sock);

    /* (1) Send file path and file metadat (both on meta struct) */
    meta metadata;
    strncpy(metadata.file_path, paketo.filename, strlen(paketo.filename) + 1);

    fprintf(stderr, "Sending file metadata\n");
    write(sock, &metadata, sizeof(metadata));
    read_ack(sock);

    /* (2) Begin sending file block-by-block */

    /* (3) Send END message */

    //while( (read_size = recv(new_sock, client_ackfer, block_sz, 0)) > 0 ) {
		/* Send the message back to client */
		//write(new_sock, client_ackfer, strlen(client_ackfer));
	//}

    // int fifo_fd;
    // char filename[ackSIZE+1];

    // pid_t me = getpid(), pops = getppid();
    // printf("\t[Worker] I am Worker child %d and my pops is %d\n", me, pops);

    // // Ignore SIGINT
    // signal(SIGINT, SIG_IGN);

    // // Handle SIGCONT
    // //signal(SIGCONT, cont_handler);

    // // Handle SIGKILL
    // signal(SIGKILL, kill_handler);

    // // Open named pipe
    // if ((fifo_fd = open(fifo, O_RDONLY | O_NONBLOCK)) < 0) {
    //     perror("\t[Worker] FIFO open problem");
    //     exit(3);
    // }

    // while (1) {
    //     // Block until continued

    //     printf("\t[Worker] I'm gonna block myself!\n");
    //     raise(SIGSTOP);

    //     /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    //     // Waiting for SIGCONT
    //     /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    //     char **urls = NULL;
    //     int count, bytes_read;

    //     // Read from named pipe
    //     if ( (bytes_read = read(fifo_fd, filename, ackSIZE)) < 0) {
    //         perror("\t[Worker] Problem in reading from FIFO") ;
    //         exit(5);
    //     }

    //     // Sanitize
    //     filename[bytes_read-1] = '\0';

    //     printf ("\t[Worker] Filename Received: %s\n", filename) ;
    //     //fflush (stdout);

    //     // Extract all urls from filename
    //     count = extractURLs(filename, urls);
    //     printf("\t[Worker] Count %d\n", count);

    //     //for (int i = 0; i < count; ++i)
    //         //printf("\t[Worker] URLs array index %d contains: %s\n", i, urls[i]);

    //     // Uniq/count and output
    //     //cropURLs(urls, count);

    //     // Count occurences of URLs and write to output file
    //     //count_and_out(urls, filename);
    // }

    return 0;
}
