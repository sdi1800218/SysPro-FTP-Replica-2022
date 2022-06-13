#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>     	/* for strlen */
#include <sys/socket.h> 	/* for recv */
#include <arpa/inet.h>      /* for hton */
#include <pthread.h> 		/* por dios */
#include <dirent.h> 		/* for *dir */

#include "dataServer.h"

/* Helper that waits for ACK message */
int rACK(int sock) {
    char ack[4];
    read(sock, &ack, sizeof(ack));

    if (strncmp(ack, "ACK", 3) != 0) {
        perror("[child_communicator] read() ACK");
        fprintf(stderr, "%s\n", ack);
        return -1;
    }

    return 0;
}

void wACK(int sock) {
    char *ack = "ACK";

    send(sock, ack, strlen(ack) + 1, 0);
}

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

				/* Ensure null terminated */
				sanitize(file_path);

				/* Add file_path in files and raise count */
				//files[*count] = file_path;
				files[*count] = malloc( (strlen(file_path) + 1) * sizeof(char *));
				strncpy(files[*count], file_path, strlen(file_path) + 1);

				*count = *count + 1;
			}
		}
    }

    closedir(dir);
}

/* Facilitates Handshake phase of the protocol a la B.A.T.M.A.N */
int proto_serv_phase_one(int sock) {

	/* (1) Write "hello" handshake to client */
	char *message = "hello";
	send(sock, message, strlen(message), 0);

	/* (2) Recv "hello" handshake from client */
	//fprintf(stderr, "[child_communicator::phase_one] Waiting client handshake...");

	char handshake[6];

	read(sock, handshake, 6);

	if (strncmp(handshake, "hello", 5) != 0) {
        perror("[child_communicator::phase_one] read() handshake");
        fprintf(stderr, "%s\n", handshake);
		return -1;
	}

	return 0;
}

/* Facilitates noisy session phase 2 exchanging data with the server
    Must rACK after write and wACK after read */
int proto_serv_phase_two(int sock, int block_sz,
	int *file_cnt, char *files[MAXFILES]) {

	int congest;
	char client_buffer[block_sz], directory[block_sz];

	/* (1) Write block size */
	congest = htons(block_sz);
	send(sock, &congest, sizeof(congest), 0);

	/* rACK */
	if (rACK(sock) != 0) {
        perror("[child_communicator::phase_two] rACK() block size");
        return -1;
    }

	/* (2) Recv requested directory */
	recv(sock, client_buffer, MAXFILENAME, 0);

	/* wACK */
	wACK(sock);

	/* Post process */
	sanitize(client_buffer);
	ensure_slash(client_buffer);

	/* Transfer for safety */
	sprintf(directory, "%s", client_buffer);

	//fprintf(stderr, "[dataServer] I read %s\n", directory);

	/* (3) Send back the file count */
	fprintf(stderr, "[Thread: %lu]: Gonna scan dir %s\n", pthread_self(), directory);

	int file_count = 0;
	/* Recurse on the directory and scrap the files */
	recurse_dir(directory, files, &file_count);
	*file_cnt = file_count;

	/* Debug print
	fprintf(stderr, "Count %d\n", file_count);
	for (int i = 0; i < file_count; ++i) {
		fprintf(stderr, "File $%d is: %s\n", i, files[i]);
	}
	*/

	/* pack n go */
	congest = htons(file_count);
	send(sock, &congest, sizeof(congest), 0);

	/* rACK */
	if (rACK(sock) != 0) {
        perror("[child_communicator::phase_two] rACK() file count");
        return -1;
    }

	return 0;
}

/* Communication Thread, takes care of communication with the client;
	Must follow the protocol */
void *child_communicator(void *p) {

	//fprintf(stderr, "\t [Thread %lu]:\n", pthread_self());

	/* Decompress our package */
	pkg paketo = *(pkg *)p;
	int new_sock = paketo.sock;
	int block_sz = paketo.block_size;
	int file_count;

	/* Mutex for the ages */
	pthread_mutex_t socket_mutex;
	if (pthread_mutex_init(&socket_mutex, NULL) == -1) {
        perror("[child_comms] pthread_mutex_init() socket mutex");
        exit(EXIT_FAILURE);
    }

	/* Make some VLAs */
	char *files[MAXFILES] = { NULL };

	/* ~~~~~~~~~~~~~~~~~~ BEGIN PROTOCOL ~~~~~~~~~~~~~~~~~~ */

    /* ~~~~~~~~~ PHASE 1: HELLO HANDSHAKE ~~~~~~~~~ */

	fprintf(stderr, "[Thread: %lu]: PHASE 1: ", pthread_self());
    if (proto_serv_phase_one(new_sock) == -1) {
		close(new_sock);
        perror_exit("[child_communicator] Protocol Phase 1 failure!");
    }
	else {
		fprintf(stderr, "SUCCESS\n");
	}
    /* ~~~~~~~~~ PHASE 1 END ~~~~~~~~~*/

	/* ~~~~~~~~~ PHASE 2: SESSION DATA ~~~~~~~~~ */
	fprintf(stderr, "[Thread: %lu]: PHASE 2: START\n", pthread_self());
    if (proto_serv_phase_two(new_sock, block_sz, &file_count, files) == -1) {
        close(new_sock);
		perror_exit("[remoteClient] Protocol Phase 2 failure!");
    }
	else {
		fprintf(stderr, "[Thread: %lu]: PHASE 2: SUCCESS\n", pthread_self());
	}
    /* ~~~~~~~~~ PHASE 2 END ~~~~~~~~~ */

    /* ~~~~~~~~~ PHASE 3: FILES ~~~~~~~~~ */
	//fprintf(stderr, "[PHASE 3]:\n");

	/* (1) For each file, schedule a task in the pool and pass the package */
	pkg2 pases[file_count];
	for (int t = 0; t < file_count; ++t) {

		pases[t].sock = paketo.sock;
		pases[t].block_size = paketo.block_size;
		strncpy(pases[t].filename, files[t], strlen(files[t]) + 1);
		//pasa.filename = files[t];
		pases[t].socket_mutex = &socket_mutex;

		fprintf(stderr, "[Thread: %lu]: Adding file %s to the queue...\n",
							pthread_self(), pases[t].filename);

		pthreadpool_add_task(paketo.pool, child_worker, &pases[t]);
	}
	/* ~~~~~~~~~ PHASE 3 END ~~~~~~~~~ */

    /* ~~~~~~~~~~~~~~~~~~ END PROTOCOL ~~~~~~~~~~~~~~~~~~ */


	return 0;
}