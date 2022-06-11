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

/* Simple worker, uses low level IO to extract urls from files.
        Operates via read()ing a named pipe */
void *child_worker(void *arg) {

    fprintf(stderr, "\t [Thread %lu] I am a Worker Thread!!!\n", pthread_self());

    // int fifo_fd;
    // char filename[BUFSIZE+1];

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
    //     if ( (bytes_read = read(fifo_fd, filename, BUFSIZE)) < 0) {
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
