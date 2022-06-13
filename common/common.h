#define MAXFILES 1024
#define MAXFILENAME 4096

void perror_exit(char *);
void sanitize (char* str);
void ensure_slash(char* str);
void wACK(int sock);
int rACK(int sock);