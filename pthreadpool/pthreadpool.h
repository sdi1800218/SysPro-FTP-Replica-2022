#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#include <pthread.h>    /* for everything */

/* Worker task */
typedef struct task {
    void* (*function)(void *arg);       // Worker function pointer
    void *arg;                          // Function parameters
    //struct task *next;
} task;

/* Thread pool struct for syncing */
typedef struct pthreadpool{
    int thread_num;                     // Number of threads
    int queue_sz;                       // Queue size
    task *queue;                        // Queue of tasks
    int front, rear;                    // Queue front and rear indexes
    pthread_t *workers;                // Threads in the pool
    pthread_mutex_t queue_mutex;        // Mutex Semaphore
    pthread_cond_t queue_empty;         // Condition for empty queue
    pthread_cond_t queue_not_empty;     // Condition for queue not empty
    pthread_cond_t queue_not_full;      // Condition for queue not full
    int task_queue_sz;                  // Number of current tasks
    int task_queue_close;               // Queue closed flag
    int pthreadpool_close;              // Thread pool closed flag
} pthreadpool;

/* API */
pthreadpool* pthreadpool_create(int, int);
int pthreadpool_add_task(pthreadpool *, void* (*function)(void*), void *arg);
int pthreadpool_destroy(pthreadpool *);

/* Functional Stub */
void* pthreadpool_worker_function(void *arg);

#endif /* __PTHREADPOOL_H__ */