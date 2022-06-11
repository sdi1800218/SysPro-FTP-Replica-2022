#include "pthreadpool/pthreadpool.h"

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
    char *filename;
} pkg2;

// Our 'as-function' Communication handling thread-child
void *child_communicator(void *);
// Our 'as-function' Worker
void *child_worker(void *);

/* Function prototypes */
void usage(char *);
void perror_exit(char *);
void sigchld_handler(int sig);
void sanitize (char* str);

/* Child Comm prototypes */
//void recurse_dir(char *);