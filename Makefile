# Compiler options
COMPILER = gcc
CFLAGS 	 = -Wall -Werror -g -c
LDFLAGS  = -lpthread # needed to spawn children as separate processes

# Objects and executables
SERV = dataServer/
CLI = client/
OBJS_SERV = $(SERV)dataServer.o $(SERV)worker_thread.o $(SERV)comms_thread.o pthreadpool.o common.o
OBJS_CLI = $(CLI)remoteClient.o common.o
EXEC_SERV = $(SERV)dataServer
EXEC_CLI = $(CLI)remoteClient

PTP = $(SERV)pthreadpool

common.o: common/common.c
		$(COMPILER) $(CFLAGS) common/common.c common/common.h

pthreadpool.o: $(PTP)/pthreadpool.c
		$(COMPILER) $(CFLAGS) $(PTP)/pthreadpool.c $(PTP)/pthreadpool.h

worker_thread.o: $(SERV)worker_thread.c
		$(COMPILER) $(CFLAGS) $(SERV)worker_thread.c

comms_thread.o: $(SERV)comms_thread.c
		$(COMPILER) $(CFLAGS) $(SERV)comms_thread.c

dataServer.o: $(SERV)dataServer.c
		$(COMPILER) $(CFLAGS) $(SERV)dataServer.c $(SERV)dataServer.h

dataServer: $(OBJS_SERV)
		$(COMPILER) $(OBJS_SERV) -o $(EXEC_SERV) $(LDFLAGS)

remoteClient.o: $(CLI)remoteClient.c
		$(COMPILER) $(CFLAGS) $(CLI)remoteClient.c $(CLI)remoteClient.h

remoteClient: $(OBJS_CLI)
		$(COMPILER) $(OBJS_CLI) -o $(EXEC_CLI) $(LDFLAGS)

clean:
		rm -f $(OBJS_SERV) $(OBJS_CLI) $(EXEC_SERV) $(EXEC_CLI) */*.gch
		rm -f dataServer.o remoteClient.o
		# rm -r $(CLI)target

run: $(EXEC_CLI)
		./$(EXEC_CLI) -i 127.0.0.1 -p 2022 -d target &
		./$(EXEC_CLI) -i 127.0.0.1 -p 2022 -d target/1 &

example:
		mkdir -p $(SERV)target/1/2
		mkdir -p $(SERV)target/1/3
		echo "hello" > $(SERV)target/1/4.txt
		touch $(SERV)target/1/5.pdf
		touch $(SERV)target/1/2/6.mp4
		echo "#include <stdio.h>" > $(SERV)target/1/3/7.c

deliver:
		make clean
		git archive --format=tar.gz -o ./CharalamposPantazisProject2.tar.gz master
		chmod 755 ./CharalamposPantazisProject2.tar.gz
