# Compiler options
COMPILER = gcc
CFLAGS 	 = -Wall -Werror -g -c
LDFLAGS  = -lpthread # needed to spawn children as separate processes

# Objects and executables
OBJS_SERV = dataServer.o child_worker.o child_communicator.o Queue.o
OBJS_CLI = remoteClient.o
EXEC_SERV = dataServer
EXEC_CLI = remoteClient

Q = queue_sedgewick

Queue.o:
		$(COMPILER) $(CFLAGS) $(Q)/Queue.c $(Q)/Queue.h

child_worker.o: child_worker.c
		$(COMPILER) $(CFLAGS) child_worker.c

child_communicator.o: child_communicator.c
		$(COMPILER) $(CFLAGS) child_communicator.c

dataServer.o: dataServer.c
		$(COMPILER) $(CFLAGS) dataServer.c

dataServer: $(OBJS_SERV)
		$(COMPILER) $(OBJS_SERV) -o $(EXEC) $(LDFLAGS)

clean:
		rm -f $(OBJS_*) $(EXEC_*) *.gch worker*

# TODO
run: $(EXEC)
		./$(EXEC) -p ./

debug: $(EXEC)
		gdb ./$(EXEC) -p target

deliver:
		make clean
		git archive --format=tar.gz -o ./CharalamposPantazisProject1.tar.gz master
		chmod 755 ./CharalamposPantazisProject1.tar.gz