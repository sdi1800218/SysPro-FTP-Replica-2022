/* dataServer.c */
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

/* Extracts the filename from inotifywait events */
char *get_filename(char *str) {

    char *result = NULL;
    char *inbetween;
    const char *CREATEpattern = "CREATE ";
    const char *MOVEDpattern = "MOVED_TO ";

    inbetween = strstr(str, CREATEpattern);
    if (inbetween != NULL) {
        result = inbetween + strlen(CREATEpattern);
        //printf("%s\n", result);
    }

    inbetween = strstr(str, MOVEDpattern);
    if (inbetween != NULL) {
        result = inbetween + strlen(MOVEDpattern);
        //printf("%s\n", result);
    }

    return result;
}

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

/* helper shorthand */
void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

/* Ensure the system doesn't die by the end of execution */
void ensure_slash(char *str) {
    char *src, *dest;

    /* Traverse */
    for (src = dest = str; *src; src++)
        if (*src == '/' || isalnum(*src) || *src == '.')
            *dest++ = *src;

    /* Ensure '/' at the end of dir */
    if(*(dest-1) != '/')
        *dest++ = '/';

    *dest = '\0';
}

/* Ensure the system doesn't die by the end of execution */
void sanitize(char *str) {
    char *src, *dest;
    for (src = dest = str; *src; src++)
        if (*src == '/' || isalnum(*src) || *src == '.')
            *dest++ = *src;

    *dest = '\0';
}

/* dataServer:  */
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

    /* Make-a-TCP-socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
        perror_exit("[dataServer] Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    /* Bind with a port */
    //if ( bind_on_port(sock, port, &server) == -1 ) {
    if (bind(sock, serverptr, sizeof(server))) {
        perror_exit("[dataServer] Socket bind() failed!");
        exit(EXIT_FAILURE);
    }

    /* Listen for incoming connections */
    if (listen(sock, 5) < 0) {
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
		pthread_t comms_thread;
		int *temp_sock = malloc(sizeof(int *));
		*temp_sock = new_sock;

        /* Mak-a-pkg */
        pkg p;
        p.sock = *temp_sock;
        p.block_size = block_sz;
        p.pool = PTP;

        /* Make-a-thread */
		if (pthread_create(&comms_thread, NULL, child_communicator, &p) < 0) {
			perror_exit("[dataServer] Thread creation error");
		}

		// Now join the thread, so that we dont terminate before the thread
		// pthread_join(comms_thread, NULL);
	}

    // TODO: Destroy Thread Pool

/*
    close(listen[WRITE]);
    //dup2(listen[READ], 0);

    // Wait for new reads
    while(1) {
        //printf("hello\n");
        int fifo_fd, bytes_write;
        pid_t chosen_worker;
        char read_buffer[BUFSIZE];

        // Read from pipe
        int bytes_read = read(listen[READ], read_buffer, sizeof(read_buffer));

        if ( bytes_read > 0 ) {

            //ensure proper C string
            read_buffer[bytes_read-1] = '\0';

            // 1. Read
            printf("[Manager] Read %d bytes: %s \n", bytes_read, read_buffer);

            // 2. Extract Filename
            char *filename = get_filename(read_buffer);
            printf("[Manager] Filename: %s \n", filename);

            char *pipe;
            // 3. Get next available worker
            if (QUEUEempty(avail_workers)) {
                // Make a new one
                chosen_worker = make_new_worker(numofworkers);
                ++numofworkers;

                if ( (fifo_fd = open(pipe, O_WRONLY | O_NONBLOCK)) < 0) {
                    perror_exit("[Manager] FIFO open error");
                    printf("[Manager] %s\n", pipe);
                    exit(1);
                }

                printf("[Manager] Opened FIFO\n");

                //printf("%d\n", numofworkers);
            }
            else {
                chosen_worker = QUEUEget(avail_workers);
                pipe = fetch_pipe(chosen_worker);

                //printf("[Manager] Opening FIFO: %s\n", pipe);
                if ( (fifo_fd = open(pipe, O_WRONLY)) < 0) {
                    perror_exit("[Manager] FIFO open error");
                    printf("[Manager] %s\n", pipe);
                    exit(1);
                }
                printf("[Manager] Opened FIFO\n");
            }

            // Add watched path in the filename to open
            char *fixed_filename = strcat(path, "/");
            char *full_filename = strncat(fixed_filename, filename, strlen(filename));

            //printf("[Manager] Writing..\n");
            if (( bytes_write = write(fifo_fd, full_filename, strlen(full_filename)+1) ) == -1) {
                perror_exit ("[Manager] Error in Writing");
                exit(2);
            }
            printf("[Manager] Write completed in FIFO\n");

            // 5. Send sigcont
            printf("[Manager] Sending SIGCONT\n");
            kill(chosen_worker, SIGCONT);

            //close(fifo_fd);
            //fflush(stdout);
        }

        //close(listen[READ]);
    }
*/

    return 0;
}