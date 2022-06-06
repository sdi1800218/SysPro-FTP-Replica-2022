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
#include <sys/stat.h>
#include<pthread.h>         /* for threads, with -lpthread */

#include "queue_sedgewick/Queue.h"
#include "threadpool/pthreadpool.h"
#include "dataServer.h"

struct worker {
    pid_t pid;
    char *pipe;
} work[MAXWORKERS];

// Declare the avail workers Q global for the SIGCHLD handler to use
Q avail_workers;

/* Inits random number of workers and makes them available in the Queue */
int init_workers(Q *Queue) {

    printf("[Manager::init_workers] Initializing workers\n");

    // Initialize work array with 0s and NULLs
    for (int work_entry = 0; work_entry < MAXWORKERS; ++work_entry) {
        work[work_entry].pid = 0;
        //work[work_entry].pipe = NULL;

        // Create pipe of the form worker + num of child
        char num = ('0' + work_entry);
        char fifo[10] = WORKERNAME;

        strncat(fifo, &num, 1);

        work[work_entry].pipe = malloc(10 * sizeof(char));
        strncpy(work[work_entry].pipe, fifo, 10);
        //work[work_entry].pipe = fifo;
    }

    // rand is current pid mod 4 [1, 4]
    int random = (getpid() % 4) + 1;

    for (int child = 0; child < random; ++child) {

        // Test worker able to be created
        pid_t helper;
        if ((helper = fork()) < 0) {
            perror("[Manager] Failed to fork() worker");
            exit(EXIT_FAILURE);
        }

        if (helper > 0) {
            work[child].pid = helper;
            //printf("[Manager::init_workers] Created worker with PID: %d\n", helper);
        }

        // Call the worker
        if (helper == 0) {
            //printf("Child num: %d\tfifo name %s\n", child, fifo);
            child_worker(work[child].pipe);
            exit(0);
        }
    }

    for (int child = 0; child < random; ++child) {
        // Create named pipe
        if (mkfifo(work[child].pipe, PERM) == -1) {
            if ( errno != EEXIST ) {
                perror("[Manager] mkfifo");
                exit(6);
            }
        }
    }

    return random;
}

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

/* Creates a new worker child process */
pid_t make_new_worker(int curr_num) {
     // Test worker able to be created
    pid_t helper;
    if ((helper = fork()) < 0) {
        perror("[Manager] Failed to fork() worker");
        exit(EXIT_FAILURE);
    }

    if (helper > 0) {
        work[curr_num].pid = helper;
        //printf("[Manager::init_workers] Created worker with PID: %d\n", helper);
    }

    // Call the worker
    if (helper == 0) {
        //printf("Child num: %d\tfifo name %s\n", child, fifo);
        child_worker(work[curr_num].pipe);
        exit(0);
    }

    // Create named pipe
    if (mkfifo(work[curr_num].pipe, PERM) == -1) {
        if ( errno != EEXIST ) {
            perror("[Manager] mkfifo");
            exit(6);
        }
    }

    return helper;
}

/* Print usage */
void usage(char *exec_name) {
    fprintf(stderr, "Usage: %s -p <port> -s <thread_pool> \
                    -q <queue_size> -b <block_size>\n", exec_name);
    exit(EXIT_FAILURE);
}

/*
1 process per client model

    *Parent process*

    Create listening socket a
    loop
        Wait for client request on a
        Create two-way channel b with client
        Fork a child to handle the client
        Close file descriptor of b
    end loop

    *Child process*

    Close listening socket a
    Serve client requests through b
    Close private channel b

    Exit
*/

/* Bind a socket with a port */
int bind_on_port(int sock, short port) {
    struct sockaddr_in server ;

    server.sin_family = AF_INET ;
    server.sin_addr.s_addr = htonl(INADDR_ANY) ;
    server.sin_port = htons(port);

    return bind(sock, (struct sockaddr *) &server, sizeof(server));
}

/* dataServer:  */
int main(int argc, char *argv[]) {

    int sock, thread_num, queue_sz, block_sz, opt;
    short port; /* short because max is 65*** something */

    /* Assert correct number of cmd arguments */
    if (argc > 6) {
        usage(argv[0]);
    }

    /* Parse cmd arguments */
    while ((opt = getopt(argc, argv, "psqb:")) != -1) {
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
            usage(argv[0]);
        }
    }

    /* Make-a-TCP-socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
        perror("[dataServer] Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    /* Bind with a port */
    if ( bind_on_port(sock, port) == -1 ) {
        perror("[dataServer] Socket bind() failed!");
        exit(EXIT_FAILURE);
    }

    /* Listen for incoming connections */
    listen(sock, 3);

    /* Accept incoming connection; create thread to handle */
	fprintf(stderr, "Waiting for incoming connections...\n");

	//int c = sizeof(struct sockaddr_in);
    int new_sock, *temp_sock;
	while( (new_sock = accept(sock, NULL/*(struct sockaddr *)&client*/, NULL/*(socklen_t*)&c*/)) ) {
		fprintf(stderr, "Connection accepted\n");

		// Send ACKnoledgement to client
		// message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
		// write(new_sock, message , strlen(message));

		pthread_t comms_thread;
		temp_sock = malloc(1);
		*temp_sock = new_sock;

        /* Make-a-thread */
		if( pthread_create(&comms_thread, NULL, child_communicator, temp_sock) < 0) {
			perror("[dataServer] Thread creation error");
			return 1;
		}

		// Now join the thread, so that we dont terminate before the thread
		// pthread_join(comms_thread, NULL);
        fprintf(stderr, "Connection accepted\n");
	}

/*
    // Worker thread pool
    avail_workers = QUEUEinit(MAXWORKERS);
    numofworkers = init_workers(&avail_workers);

    // Create Listener Pipe
    if (pipe(listen) == -1) {
        perror("[Manager::pipe] Pipe creation call");
        exit(EXIT_FAILURE);
    }

    // fork() Listener
    if ( (listener = fork()) < 0) {
        perror("[Manager::fork] Failed to create listener");
        exit(EXIT_FAILURE);
    }

    if(listener > 0) {
        listener_pid = listener;
        //printf("[Manager] Listener PID: %d", (int)listener_pid);
    }

    // Call Listener
    if (listener == 0) {
        child_listener(&listen, path);
        exit(0);
    }

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
                    perror("[Manager] FIFO open error");
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
                    perror("[Manager] FIFO open error");
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
                perror ("[Manager] Error in Writing");
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