/* dataServer.c: Server that handles multiple clients in parallel.
    Replies to clients requesting files from the local file structure.
    Uses a thread pool to handle different task workloads.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>     /* for socket handling */
#include <arpa/inet.h>      /* for hton */
#include <netinet/in.h>     /* for sockaddr_in */
#include <sys/wait.h>
#include <pthread.h>        /* for threads, with -lpthread */
#include <ctype.h>          /* for isalnum */

#include "pthreadpool/pthreadpool.h"
#include "dataServer.h"

/* Signal flag */
volatile sig_atomic_t sigint_flag = 0;

/* Print usage */
void usage(char *exec_name) {
    fprintf(stderr, "Usage: %s -p <port> -s <thread_pool> "\
                    "-q <queue_size> -b <block_size>\n", exec_name);
    exit(EXIT_FAILURE);
}

/* Wait for all dead child processes */
void sigchld_handler(int sig) {
    while (waitpid( -1, NULL, WNOHANG) > 0);
}

/* Destroy the thread pool and exit */
void sigint_handler(int sig) {
    sigint_flag = 1;
    return;
}

/* dataServer: Look at line 1  */
int main(int argc, char *argv[]) {

    int sock, port, new_sock, thread_num, queue_sz, block_sz, opt;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);
    struct sockaddr *serverptr = (struct sockaddr *)&server;
    struct sockaddr *clientptr = (struct sockaddr *)&client;
    pthreadpool *PTP;

    /* Assert correct number of cmd arguments */
    if (argc != 9) {
        usage(argv[0]);
    }

    /* Reap dead children asynchronously */
    signal (SIGCHLD, sigchld_handler);

    /* Handle CTRL+C (SIGINT) */
    //signal (SIGINT, sigint_handler);

    /* Parse cmd arguments */
    while ((opt = getopt(argc, argv, ":p:s:q:b:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                thread_num = atoi(optarg);
                break;
            case 'q':
                queue_sz = atoi(optarg);
                break;
            case 'b':
                block_sz = atoi(optarg);
                break;
            default:
                //perror_exit("what");
                usage(argv[0]);
            }
    }

    /* Print Server Info */
    fprintf(stderr, "Server parameters are:\n");
    fprintf(stderr, "port:\t%d\n", port);
    fprintf(stderr, "thread_pool_size: %d\n", thread_num);
    fprintf(stderr, "queue_size:\t%d\n", queue_sz);
    fprintf(stderr, "block_size:\t%d\n", block_sz);

    /* Initialize Thread Pool of Worker Threads */
    PTP = pthreadpool_create(thread_num, queue_sz);

    /* Operate while no signal is received */
    while (!sigint_flag) {

        /* Make-a-TCP-socket */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
            perror_exit("[dataServer] Socket creation failed!");
        }

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(port);

        /* Bind with a port */
        if (bind(sock, serverptr, sizeof(server))) {
            perror_exit("[dataServer] Socket bind() failed!");
        }

        /* Listen for incoming connections */
        if (listen(sock, 64) < 0) {
            perror_exit("[dataServer] listen()");
        }

        /* Accept incoming connection; create thread to handle */
        fprintf(stderr, "[dataServer] Server was successfully initialized...\n");
        fprintf(stderr, "[dataServer] Waiting for incoming on port %d\n", port);

        //int c = sizeof(struct sockaddr_in);
        while( (new_sock = accept(sock, clientptr, &clientlen)) ) {

            if ( new_sock < 0 ) {
                perror_exit("[dataServer] accept()");
            }
            fprintf(stderr, "[dataServer] Connection accepted\n");

            /* It works! */
            pthread_t communication_thread;
            int *temp_sock = malloc(sizeof(int *));
            *temp_sock = new_sock;

            /* Mak-a-pkg */
            pkg p;
            p.sock = *temp_sock;
            p.block_size = block_sz;
            p.pool = PTP;

            /* Make-a-thread */
            if (pthread_create(&communication_thread, NULL, comms_thread, &p) < 0) {
                perror_exit("[dataServer] Thread creation error");
            }

            // Now join the thread, so that we dont terminate before the thread
            // pthread_join(comms_thread, NULL);
        }
    }

    /* SIGINT Received */
    /* Destroy Thread Pool */
    fprintf(stderr, "Received SIGINT\tStopping..\n");
    pthreadpool_destroy(PTP);

    return 0;
}