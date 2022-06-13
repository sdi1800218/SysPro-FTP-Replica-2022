#include "../common/common.h"

/* Prototypes */
void usage(char *);
void make_dir_path(char file_path[MAXFILENAME]);
int proto_cli_phase_one(int sock);
int proto_cli_phase_two(int sock, int *, int *, char *);
int proto_cli_phase_three(int sock, int, int);