#include "common.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>         /* for exit */
#include <ctype.h>          /* for isalnum */
#include <sys/socket.h>     /* for socket handling */

/* helper shorthand */
void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

/* Ensure the system doesn't die by the end of execution */
void ensure_slash(char *str) {
    char *src, *dest;

    /* Traverse */
    for (src = dest = str; *src; src++)
        if (*src == '/' || isalnum(*src) || *src == '.')
            *dest++ = *src;

    /* Ensure '/' at the end of dir */
    if(*(dest-1) != '/')
        *dest++ = '/';

    *dest = '\0';
}

/* Ensure the system doesn't die by the end of execution */
void sanitize(char *str) {
    char *src, *dest;
    for (src = dest = str; *src; src++)
        if (*src == '/' || isalnum(*src) || *src == '.')
            *dest++ = *src;

    *dest = '\0';
}

/* Helper that waits for ACK message */
int rACK(int sock) {
    char ack[4];
    recv(sock, &ack, sizeof(ack), 0);

    if (strncmp(ack, "ACK", 3) != 0) {
        perror("[remoteClient] recv() ACK");
        fprintf(stderr, "%s\n", ack);
        return -1;
    }

    return 0;
}

/* Helper that sends ACK messages */
void wACK(int sock) {
    char *ack = "ACK";

    send(sock, ack, strlen(ack) + 1, 0);
}