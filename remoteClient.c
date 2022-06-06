/* dataServer.c
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
#include <sys/stat.h>

#include "queue_sedgewick/Queue.h"
#include "remoteClient.h"

pid_t listener_pid;

/* SIGINT Handler */
void int_handler(int signal) {
    printf("[Manager::int_handler] Received a SIGINT\n");

    // kill thy Listener TODO
    kill(listener_pid, SIGKILL);
    //wait(0);

    int count_active = 0;
    pid_t child;

    while ( !QUEUEempty(avail_workers) ) {
        child = QUEUEget(avail_workers);
        ++count_active;

        printf("[Manager::int_handler] Sending SIGKILL to Child: %d\n", child);
        kill(child, SIGKILL);
    }

    // SIGSEGV
    //free(avail_workers);

    // clear named pipes
    for(int i = 0; i < count_active; ++i) {
        printf("[Manager::int_handler] Removing fifo with name %s\n", work[i].pipe);
        remove(work[i].pipe);
    }

    // Wait workers
    for (int i = 0; i < count_active; ++i) {
        fprintf(stderr, "Child %d DIED\n", i+1);
        wait(0);
    }

    printf("[Manager::int_handler] Handled Succcessfully!\n");

    exit(0);
}

/* SIGCHLD Handler */
void child_handler(int signal) {
    pid_t child;
    int status;

    // Receive signal
    printf("[Manager::child_handler] Received a SIGCHLD");

    // Get PID of caller
    //while ( (child = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    if ( (child = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        printf(" from child with PID %d\n", (int) child);
        // Append PID of child in avail queue
        QUEUEput(avail_workers, child);
    }

    //printf("HANDLED\n");

    //return;
}

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

/* Given a pid of a worker, find its named pipe */
char *fetch_pipe(pid_t pid) {

    int i = 0;

    printf("[Manager::fetch_pipe] Fetching pipe for PID %d\n", (int) pid);

    while (work[i].pid != 0) {
        //printf("%s\n", work[i].pipe);

        if (pid == work[i].pid) {
            printf("[Manager::fetch_pipe] Pipe name: %s\n", work[i].pipe);
            return work[i].pipe;
        }

        ++i;
    }
    return NULL;
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
    fprintf(stderr, "Usage: %s -i <server_ip> -p <server_port> \
                    -d <dir>\n", exec_name);
    exit(EXIT_FAILURE);
}

/* Connect socket to remote server */
int connect_to_server(socket sock, struct sockaddr_in *server) {
    server.sin_addr.s_addr = inet_addr("74.125.235.20");
	server.sin_family = AF_INET;

	//Connect to remote server
	return connect(sock, (struct sockaddr *)&server , sizeof(server));
}


/* Manager: creates a pipe and a listener, handles the workers */
int main(int argc, char *argv[]) {

    int opt, listen[2], numofworkers;
    char *dir;
    pid_t listener;

    struct sockaddr_in server;

    // Enlist SIGINT and SIGCHLD signal handlers
    signal(SIGINT, int_handler);
    signal(SIGCHLD, child_handler);

    /* Assert correct number of cmd arguments */
    if (argc > 5) {
        usage(argv[0]);
    }

    /* Parse cmd arguments */
    while ((opt = getopt(argc, argv, "ipd:")) != -1) {
        switch (opt) {
        case 'i':
            server.sin_addr.s_addr = inet_addr(optarg);
            break;
        case 'p':
            server.sin_port = htons( atoi(optarg) );
            break;
        case 'd':
            dir = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }

    /* Make-a-TCP-socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
        perror("[remoteClient] Socket creation failed!");
        exit(EXIT_FAILURE);
    }

	/* Connect to remote server */
    if (connect_to_server(sock, &server) == -1) {
        perror("[remoteClient] Socket connect() failure!");
        exit(EXIT_FAILURE);
    }
    else
	    puts("[remoteClient] Connected\n");

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

    return 0;
}