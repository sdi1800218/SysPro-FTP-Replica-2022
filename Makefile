# Compiler options
COMPILER = gcc
CFLAGS 	 = -Wall -Werror -g -c
LDFLAGS  = -lpthread # needed to spawn children as separate processes

# Objects and executables
OBJS_SERV = dataServer.o child_worker.o child_communicator.o pthreadpool.o # Queue.o
OBJS_CLI = remoteClient.o
EXEC_SERV = dataServer
EXEC_CLI = remoteClient

# Q = queue_sedgewick

# Queue.o:
# 		$(COMPILER) $(CFLAGS) $(Q)/Queue.c $(Q)/Queue.h

PTP = pthreadpool

pthreadpool.o: $(PTP)/pthreadpool.c
		$(COMPILER) $(CFLAGS) $(PTP)/pthreadpool.c $(PTP)/pthreadpool.h

child_worker.o: child_worker.c
		$(COMPILER) $(CFLAGS) child_worker.c

child_communicator.o: child_communicator.c
		$(COMPILER) $(CFLAGS) child_communicator.c

dataServer.o: dataServer.c
		$(COMPILER) $(CFLAGS) dataServer.c dataServer.h

dataServer: $(OBJS_SERV)
		$(COMPILER) $(OBJS_SERV) -o $(EXEC_SERV) $(LDFLAGS)

remoteClient.o: remoteClient.c
		$(COMPILER) $(CFLAGS) remoteClient.c remoteClient.h

remoteClient: $(OBJS_CLI)
		$(COMPILER) $(OBJS_CLI) -o $(EXEC_CLI) $(LDFLAGS)

clean:
		rm -f $(OBJS_SERV) $(OBJS_CLI) $(EXEC_SERV) $(EXEC_CLI) *.gch worker*

# TODO
run: $(EXEC)
		./$(EXEC) -p ./

debug: $(EXEC)
		gdb ./$(EXEC) -p target

deliver:
		make clean
		git archive --format=tar.gz -o ./CharalamposPantazisProject2.tar.gz master
		chmod 755 ./CharalamposPantazisProject2.tar.gz