#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>		/* for stat */
#include <sys/socket.h> 	/* for recv */
#include <arpa/inet.h>      /* for hton */
#include <pthread.h>        /* I am a thread duh */

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

/* SIGKILL Handler */
void kill_handler(int sig) {
    printf("\t[Worker::Handler] Received a SIGKILL!\n");
    printf("\t[Worker::Handler] Shuting down..");
    exit(EXIT_SUCCESS);
}

/* Worker Thread: TODO */
void *child_worker(void *arg) {

    int sock, congest, block_size;
    char handshake[5], *message, file_path[MAXFILENAME];
    //fprintf(stderr, "[Thread: %lu]: I am a Worker Thread!!!\n", pthread_self());

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
        perror("[child_communicator::phase_one] read() handshake");
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
        perror_exit("[child_worker] stat()");
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
