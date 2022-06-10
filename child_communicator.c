#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>     	/* for strlen */
#include <sys/socket.h> 	/* for recv */
#include <arpa/inet.h>      /* for hton */
#include <pthread.h> 		/* por dios */

#include "dataServer.h"

/* Listener, monitors folder via inotifywait() */
void *child_communicator(void *p) {

	fprintf(stderr, "\t [Thread %lu]\n", pthread_self());

	pkg pk = *(pkg *)p;
	int new_sock = pk.sock;
	int block_sz = pk.block_size;
	int read_size;
	char *message, client_buffer[block_sz], directory[block_sz];

	/* (1) Write "hello" handshake to client */
	message = "hello";
	write(new_sock, message, strlen(message));

	/* (2) Recv "hello" handshake from client */
	fprintf(stderr, "[dataServer] Waiting client handshake...\n");

	char handshake[6];

	read_size = read(new_sock, handshake, 6);

	if (read_size != 6) {
        perror("[dataServer] recv");
    }

	if (strncmp(message, handshake, 6)) {
    	fprintf(stderr, "\tSUCCESS\n");
	}

	/* (3) Write block size */
	int congest = htons(block_sz);
	write(new_sock, &congest, sizeof(congest));

	/* (4) Recv requested directory */
	read(new_sock, client_buffer, block_sz);

	sanitize(client_buffer);

	snprintf(directory, block_sz, "%s", client_buffer);

	fprintf(stderr, "[dataServer] I read %s\n", directory);



	//while( (read_size = recv(new_sock, client_buffer, block_sz, 0)) > 0 ) {
		/* Send the message back to client */
		//write(new_sock, client_buffer, strlen(client_buffer));
	//}

	//Free the socket pointer
	close(new_sock);

	return 0;
}