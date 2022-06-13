#define MAXFILES 1024
#define MAXFILENAME 4096

/* Prototypes */
void usage(char *);
void sanitize(char *str);
void make_dir_path(char file_path[]);
void wACK(int sock);
int rACK(int sock);
int proto_cli_phase_one(int sock);
int proto_cli_phase_two(int sock, int *, int *, char *);
int proto_cli_phase_three(int sock, int, int);