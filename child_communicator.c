#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>     /* for strlen */
#include <sys/socket.h> /* for recv */

#include "dataServer.h"

/* Listener, monitors folder via inotifywait() */
void *child_communicator(void *sock) {

    //Get the socket descriptor
	int sockr = *(int*)sock;
	int read_size;
	char *message , client_message[2000];

	//Send some messages to the client
	message = "Greetings! I am your connection handler\n";
	write(sockr , message , strlen(message));

	message = "Now type something and i shall repeat what you type \n";
	write(sockr , message , strlen(message));

	//Receive a message from client
	while( (read_size = recv(sockr, client_message , 2000 , 0)) > 0 )
	{
		//Send the message back to client
		write(sockr , client_message , strlen(client_message));
	}

	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}

	//Free the socket pointer
	free(sock);

	return 0;
}