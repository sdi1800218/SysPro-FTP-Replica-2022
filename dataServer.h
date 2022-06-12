#include "pthreadpool/pthreadpool.h"

#define MAXFILES 1024
#define MAXFILENAME 4096

/* Passed to child communicator */
typedef struct pkg {
    int sock;
    int block_size;
    pthreadpool *pool;
} pkg;

/* Passed to child worker */
typedef struct pkg2 {
    int sock;
    int block_size;
    char filename[MAXFILENAME];
    pthread_mutex_t *socket_mutex;
} pkg2;

typedef struct meta {
    char file_path[MAXFILENAME];
    // TODO: File metadata
} meta;

/* Communication Thread func */
void *child_communicator(void *);

/* Worker Thread func */
void *child_worker(void *);

/* Function prototypes */
void usage(char *);
void perror_exit(char *);
void sigchld_handler(int sig);
void sanitize (char* str);
void ensure_slash(char* str);
void wACK(int sock);
int rACK(int sock);

/* Child Comm prototypes */
void recurse_dir(char *, char *[MAXFILES], int *);