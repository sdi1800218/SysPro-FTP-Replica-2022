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

/* Recursively fetch the files of the given dir */
void recurse_dir(char *dir_path, char *files[MAXFILES], int *count) {

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
				recurse_dir(new_dir, files, count);
			}
			else {
				//fprintf(stderr, "%s%s\n", dir_path, myent->d_name);

				char file_path[MAXFILENAME];
				strcpy(file_path, dir_path);
				strcat(file_path, myent->d_name);

				/* Add file_path in files and raise count */
				//files[*count] = file_path;
				files[*count] = malloc(strlen(file_path) * sizeof(char *));
				strcpy(files[*count], file_path);

				*count = *count + 1;
			}
		}
    }

    closedir(dir);
}

/* Listener, monitors folder via inotifywait() */
void *child_communicator(void *p) {

	fprintf(stderr, "\t [Thread %lu]\n", pthread_self());

	/* Decompress our package */
	pkg paketo = *(pkg *)p;
	int new_sock = paketo.sock;
	int block_sz = paketo.block_size;

	/* Make some VLAs */
	char *message, client_buffer[block_sz], directory[block_sz];
	char *files[MAXFILES] = { NULL };

	/* (1) Write "hello" handshake to client */
	message = "hello";
	write(new_sock, message, strlen(message));

	/* (2) Recv "hello" handshake from client */
	fprintf(stderr, "[dataServer] Waiting client handshake...");

	char handshake[6];

	read(new_sock, handshake, 6);

	if (strncmp(handshake, "hello", 5) == 0) {
        fprintf(stderr, "\tSUCCESS\n");
    }
    else {
        perror("[remoteClient] recv() handshake");
        fprintf(stderr, "%s\n", handshake);
        exit(EXIT_FAILURE);
    }

	/* (3) Write block size */
	int congest = htons(block_sz);
	write(new_sock, &congest, sizeof(congest));

	/* (4) Recv requested directory */
	read(new_sock, client_buffer, block_sz);

	sanitize(client_buffer);

	snprintf(directory, block_sz, "%s", client_buffer);

	fprintf(stderr, "[dataServer] I read %s\n", directory);

	/* (5) Send back the file count */
	fprintf(stderr, "[Thread %lu] Gonna scan dir %s\n", pthread_self(), directory);

	int file_count = 0;
	recurse_dir(directory, files, &file_count);

	/* Debug print
	fprintf(stderr, "Count %d\n", file_count);
	for (int i = 0; i < file_count; ++i) {
		fprintf(stderr, "File $%d is: %s\n", i, files[i]);
	}
	*/

	/* TODO: Send count */
	/* wait for ack */

	/* (6) List the directory contents recursively;
			for each file, schedule a task in the pool and send to the client */
	for (int t = 0; t < file_count; ++t) {

		pkg2 pasa;
		pasa.sock = paketo.sock;
		pasa.block_size = paketo.block_size;
		pasa.filename = files[t];

		fprintf(stderr, "[Thread: %lu]: Adding file %s to the queue...\n",
							pthread_self(), files[t]);

		pthreadpool_add_task(paketo.pool, child_worker, &pasa);

	}

	return 0;
}