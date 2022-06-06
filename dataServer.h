#define READ  0
#define WRITE 1

#define PERM 0644

#define BUFSIZE 1024
#define MAXWORKERS 50

#define WORKERNAME "worker"
#define OUTDIR "output"

// Our 'as-function' Communication handling thread-child
void *child_communicator(void *);
// Our 'as-function' Worker
void child_worker(char *);

/* Function prototypes */
void usage(char *);
int bind_on_port(int, short);
void sanitize (char* str);