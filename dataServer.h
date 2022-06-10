#define READ  0
#define WRITE 1

#define PERM 0644

#define BUFSIZE 1024
#define MAXWORKERS 50

#define WORKERNAME "worker"
#define OUTDIR "output"

typedef struct pkg {
    int sock;
    int block_size;
} pkg;

// Our 'as-function' Communication handling thread-child
void *child_communicator(void *);
// Our 'as-function' Worker
//void child_worker(char *);

/* Function prototypes */
void usage(char *);
void perror_exit(char *);
void sigchld_handler(int sig);
void sanitize (char* str);