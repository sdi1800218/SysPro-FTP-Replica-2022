#define MAXFILES 1024
#define MAXFILENAME 4096

typedef struct meta {
    char file_path[MAXFILENAME];
    // TODO: File metadata
} meta;

// prototypes
void int_handler(int);
void child_handler(int);