#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#include <pthread.h>    /* for everything */

/* Worker task */
struct task {
    void* (*function)(void *arg);       // Worker function pointer
    void *arg;                          // Function parameters
    struct task *next;
};

/* Thread pool struct for syncing */
typedef struct pthreadpool{
    int thread_num;                     // The number of open threads in the thread pool
    int queue_sz;                       // The maximum number of jobs in the queue
    struct task *head;                  // Head pointer to job
    struct task *tail;                  // The tail pointer to the job
    pthread_t *pthreads;                // Pthread_t of all threads in the thread pool
    pthread_mutex_t queue_mutex;        // Mutually Exclusive Semaphore
    pthread_cond_t queue_empty;         // Condition variable with empty queue
    pthread_cond_t queue_not_empty;     // Condition variable whose queue is not empty
    pthread_cond_t queue_not_full;      // Condition variable that the queue is not full
    int task_queue_sz;                  // Current tasks in queue
    int task_queue_close;               // Whether the queue is closed
    int pthreadpool_close;              // Whether the thread pool is closed
} *pthreadpool_t;

/* API */
pthreadpool_t* pthreadpool_create(int, int);
void pthreadpool_add_task();
int pthreadpool_destroy(pthreadpool_t*);

#endif /* __PTHREADPOOL_H__ */