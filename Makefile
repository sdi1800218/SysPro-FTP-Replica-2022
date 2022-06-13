# Compiler options
COMPILER = gcc
CFLAGS 	 = -Wall -Werror -g -c
LDFLAGS  = -lpthread # needed to spawn children as separate processes

# Objects and executables
OBJS_SERV = dataServer.o worker_thread.o comms_thread.o pthreadpool.o
OBJS_CLI = remoteClient.o
EXEC_SERV = dataServer
EXEC_CLI = remoteClient

PTP = pthreadpool

pthreadpool.o: $(PTP)/pthreadpool.c
		$(COMPILER) $(CFLAGS) $(PTP)/pthreadpool.c $(PTP)/pthreadpool.h

worker_thread.o: worker_thread.c
		$(COMPILER) $(CFLAGS) worker_thread.c

comms_thread.o: comms_thread.c
		$(COMPILER) $(CFLAGS) comms_thread.c

dataServer.o: dataServer.c
		$(COMPILER) $(CFLAGS) dataServer.c dataServer.h

dataServer: $(OBJS_SERV)
		$(COMPILER) $(OBJS_SERV) -o $(EXEC_SERV) $(LDFLAGS)

remoteClient.o: remoteClient.c
		$(COMPILER) $(CFLAGS) remoteClient.c remoteClient.h

remoteClient: $(OBJS_CLI)
		mkdir -p ./client
		$(COMPILER) $(OBJS_CLI) -o client/$(EXEC_CLI) $(LDFLAGS)

clean:
		rm -f $(OBJS_SERV) $(OBJS_CLI) $(EXEC_SERV) $(EXEC_CLI) *.gch

run: $(EXEC_SERV) $(EXEC_CLI)
		./$(EXEC_SERV) -p 2022 -s 2 -q 2 -b 512 &
		sleep 2; ./$(EXEC_CLI) -i 127.0.0.1 -p 2022 -d target
		sleep 4; ./$(EXEC_CLI) -i 127.0.0.1 -p 2022 -d target/1

debug: $(EXEC)
		gdb ./$(EXEC) -p target

deliver:
		make clean
		git archive --format=tar.gz -o ./CharalamposPantazisProject2.tar.gz master
		chmod 755 ./CharalamposPantazisProject2.tar.gz