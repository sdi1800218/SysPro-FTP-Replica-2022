#define READ  0
#define WRITE 1

#define PERM 0644

#define BUFSIZE 1024
#define MAXWORKERS 50

#define WORKERNAME "worker"
#define OUTDIR "output"

// prototypes
void int_handler(int);
void child_handler(int);

// Our 'as-function' Listener
void child_listener(int (*)[2], char *);
// Our 'as-function' Worker
void child_worker(char *);

int bind_on_port(int, short);
