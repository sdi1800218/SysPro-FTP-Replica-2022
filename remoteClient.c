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

#include "queue_sedgewick/Queue.h"
#include "remoteClient.h"

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
    fprintf(stderr, "Usage: %s -i <server_ip> -p <server_port> " \
                    "-d <dir>\n", exec_name);
    exit(EXIT_FAILURE);
}

/* Connect socket to remote server */
int connect2server(int sock, int port, struct sockaddr_in *server) {
    //server.sin_addr.s_addr = inet_addr("");


	//Connect to remote server
	return connect(sock, (struct sockaddr *)&server , sizeof(server));
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
    else
	    puts("[remoteClient] Connected\n");

    /* (1) Recv "hello" handshake */
    fprintf(stderr, "[remoteClient] Waiting server handshake...");

    char handshake[6];

    read_size = read(sock, handshake, 6);

    if (read_size != 6) {
        perror("[remoteClient] recv() handshake");
    }

    fprintf(stderr, "\tSUCCESS\n");

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

    // Worker availability queue
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
*/
    //close(listen[WRITE]);
    //dup2(listen[READ], 0);

/*
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