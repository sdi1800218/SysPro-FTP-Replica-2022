# Project 2
## Friendly Title: FTP Replica

- sdi1800218

## Build/Run directions
- `make dataServer`: makes the server inside the dataServer dir
- `make remoteClient`: makes the client inside the client dir
- `make example`: makes the example server dir tree with a few data
- `make clean`: cleans objects and executables

## Analysis

### Abstract

As a bonus to the assignment I wanted to implement secure tunneling functionality powered by WireGuard tunnels.
The code for embedding WireGuard tunnels can be found at (https://git.zx2c4.com/wireguard-tools/tree/contrib/embeddable-wg-library).
It would be like a deliberate implementation of sftp. The two reasons for not doing that were (a) time restrictions (debugging) and
(b) the above implementation needs root permission to create the wg network interfaces and the constraint that the code must run to the uni's linux machines was violated (at least on my part, I had no root in order to test).

### The protocol

The protocol of communication between client and server has 3 Phases:
- Phase 1: Handshake, "hello" back and forth like B.A.T.M.A.N
- Phase 2: Session data, exchange the variables necessary to start the file transfer.
            The variables are in order block size(s2c), directory(c2s), count of files to deliver(s2c).
            Each variable exchange is terminated by an ACK from the other end.
- Phase 3: Files, for each file, each thread sends (a) file path (b) metadata;
            then, sends a "FILE" handshake and after ACK starts sending the file in blocks.

### dataServer

- Creates the socket, binds it and listens to it; then loops in a while waiting for connections to accept.
- It has 2 thread "functions" the comms_thread and worker_thread. When a new connection arrives it crafts a package with data needed by the comms_thread and then switches the connection handling to the comms_thread.
- It should have a sigint handler, but it was buggy how I did it and also I saw somewhere mentioned that it is out of scope.

- The comms_worker():
    * Performs phases 1 and 2 of the protocol in the side of the server.
    * Respectively the proto_serv_phase_one() and proto_serv_phase_two() serve each.
    * One is trivial; in phase 2 it fetches the names of all files found in the directory after it exchanges the data with the client.
    * Effectively it starts Phase 3 of the protocol since it calls pthreadpool_add_task() to schedule the tasks ( function calls to worker_thread() ) in the thread pool;
        it also creates a new pkg (pkg2) with data that the worker_thread needs in order to continue.

- The thread_worker():
    * Performs Phase 3 of the protocol.
    * It uses a pthread mutex to lock the socket when it starts transmission.
    * It uses normal IO to open and read files. Because it uses fgets(), the block size is effectively smaller by 1 because of '\0'; this is reflected in the calculation of the packets to be transmitted.

- Other notes:
    * The Server can use any directory in it's local path as the file tree root, the dir is not hard-coded. I used ./target/ as my testcase.

### remoteClient

- Creates a socket and connect()s to the server.
- It runs all pahses of the protocol.
- Each phase is handled by a function.
- 1 and 2 are trivial. proto_cli_phase_three() performs phase 3 and it's the receiving mirror of the functionality of worker_thread();
- After all are done it dies.
- The directory that is requested gets copied to the current working directory.

### pthreadpool

- The thread pool follows a generic implementation. It is made in such a way to be provided as a library, so that it can be reused.
- It provides an API and works with the power of function pointers.
- The argument passed is of type void * and gets cast depending on the task.
- It implements the queue data struct for tasks internally and defines 3 pthread conditions (pthread_cond_t) to schedule the waiting threads.

### common

- perror_exit() and sanitize() functions are from the slides.

### Others
- Whitespace consistent and 80 Column proud.
