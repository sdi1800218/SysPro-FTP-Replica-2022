#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>     	/* for strlen */
#include <sys/socket.h> 	/* for recv */
#include <arpa/inet.h>      /* for hton */
#include <pthread.h> 		/* por dios */
#include <dirent.h> 		/* for TODO */

#include "dataServer.h"

#define MAXFILES 1024
#define MAXFILENAME 1024

pkg paketo;

/* Recursively fetch the files of the given dir */
void recurse_dir(char *dir_path) {

	struct dirent *myent;
	DIR *dir = opendir(dir_path);

	if (dir == NULL) {
		perror("[child_comm::recurse_dir] opendir()");
		fprintf(stderr, "%s\n", dir_path);
		return;
	}

	while ( (myent = readdir(dir)) != NULL) {

		//printf("%s\n", dir_path);

		if ( (strcmp(myent->d_name, ".") != 0)
				&& (strcmp(myent->d_name, "..") != 0) ) {

			/* if dir -> recurse; if file add to the queue */
			if (myent->d_type == DT_DIR) {

				/* Mak-a-newdirpath */
				char new_dir[MAXFILENAME];
				strcpy(new_dir, dir_path);
				strcat(new_dir, myent->d_name);
				strcat(new_dir, "/");

				//printf("%s\n", new_dir);

				/* Recurse on child dir */
				recurse_dir(new_dir);
			}
			else {
				//fprintf(stderr, "%s%s\n", dir_path, myent->d_name);

				char file_path[MAXFILENAME];
				strcpy(file_path, dir_path);
				strcat(file_path, myent->d_name);

				/* Add task in queue */
				fprintf(stderr, "[Thread: %lu]: Adding file %s to the queue...\n",
						pthread_self(), file_path);

				pkg2 pasa;
				pasa.sock = paketo.sock;
				pasa.block_size = paketo.block_size;
				pasa.filename = file_path;

				pthreadpool_add_task(paketo.pool, child_worker, &pasa);
			}
		}
    }

    closedir(dir);
}

/* Listener, monitors folder via inotifywait() */
void *child_communicator(void *p) {

	fprintf(stderr, "\t [Thread %lu]\n", pthread_self());

	/* Decompress our package */
	paketo = *(pkg *)p;
	int new_sock = paketo.sock;
	int block_sz = paketo.block_size;
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

	/* (5) List the directory contents recursively */
	fprintf(stderr, "[Thread %lu] Gonna scan dir %s\n", pthread_self(), directory);

	//files = malloc(MAXFILES * sizeof(char *));
	recurse_dir(directory);
	//fprintf(stderr, "Count %d\n", filename_count);

	//while( (read_size = recv(new_sock, client_buffer, block_sz, 0)) > 0 ) {
		/* Send the message back to client */
		//write(new_sock, client_buffer, strlen(client_buffer));
	//}

	//Free the socket pointer
	close(new_sock);

	return 0;
}