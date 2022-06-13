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

/* Communication Thread func */
void *comms_thread(void *);

/* Worker Thread func */
void *worker_thread(void *);

/* Signal handlers */
void sigchld_handler(int sig);
void sigint_handler(int sig);

/* Function prototypes */
void usage(char *);
void perror_exit(char *);
void sanitize (char* str);
void ensure_slash(char* str);
void wACK(int sock);
int rACK(int sock);

/* Child Comm prototypes */
void recurse_dir(char *, char *[MAXFILES], int *);
int proto_serv_phase_one(int sock);
int proto_serv_phase_two(int sock, int,	int *, char *[MAXFILES]);