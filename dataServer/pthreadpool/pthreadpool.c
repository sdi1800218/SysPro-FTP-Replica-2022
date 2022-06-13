#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "pthreadpool.h"

/* Initialize thread pool values */
pthreadpool* pthreadpool_create(int thread_number, int queue_size) {

    pthreadpool *pool;

    pool = (pthreadpool *)malloc(sizeof(struct pthreadpool));
    if (pool == NULL) {
        perror("[pthreadpool] malloc()");
        exit(EXIT_FAILURE);
    }

    /* Init pthreadpool members */
    pool->thread_num = thread_number;
    pool->queue_sz = queue_size;

    pool->front = 0;
    pool->rear = 0;

    pool->task_queue_sz = 0;
    pool->task_queue_close = 0;
    pool->pthreadpool_close = 0;


    /* Init pthread mutex and cond members */
    if (pthread_mutex_init(&pool->queue_mutex, NULL) == -1) {
        perror("[pthreadpool] pthread_mutex_init() Qmutex");
        free(pool);
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&pool->queue_not_full, NULL) == -1) {
        perror("[pthreadpool] pthread_cond_init() QnF");
        free(pool);
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&pool->queue_not_empty, NULL) == -1) {
        perror("[pthreadpool] pthread_cond_init() QnE");
        free(pool);
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&pool->queue_empty, NULL) == -1) {
        perror("[pthreadpool] pthread_cond_init() QE");
        free(pool);
        exit(EXIT_FAILURE);
    }

    /* malloc() the task queue */
    pool->queue = malloc(sizeof(struct task) * pool->queue_sz);
    if (pool->queue == NULL) {
        perror("[pthreadpool] malloc() queue");
        free(pool);
        exit(EXIT_FAILURE);
    }

    /* malloc() the thread workers */
    pool->workers = malloc(sizeof(pthread_t) * thread_number);
    if (pool->workers == NULL) {
        perror("[pthreadpool] malloc() workers");
        free(pool->queue);
        free(pool);
        exit(EXIT_FAILURE);
    }

    /* Fill the pool */
    for (int i = 0; i < thread_number; ++i) {
        if (pthread_create(&pool->workers[i], NULL,
                            pthreadpool_worker_function, (void *)pool) != 0) {
            perror("[pthreadpool] pthread_create() workers");
            exit(EXIT_FAILURE);
        }
    }

    return pool;
}

/* Clean up thread pool */
int pthreadpool_destroy(pthreadpool *pool) {

    assert(pool != NULL);

    pthread_mutex_lock(&(pool->queue_mutex));

    /* If the worker pool exited, return */
    if (pool->task_queue_close || pool->pthreadpool_close) {
        pthread_mutex_unlock(&(pool->queue_mutex));
        return EXIT_FAILURE;
    }

    /* Close the queue */
    pool->task_queue_close = 1;
    while (pool->task_queue_sz != 0) {
        pthread_cond_wait(&(pool->queue_empty), &(pool->queue_mutex));
    }

    /* Close the pool of workers */
    pool->pthreadpool_close = 1;
    pthread_mutex_unlock(&(pool->queue_mutex));
    pthread_cond_broadcast(&(pool->queue_not_empty));
    pthread_cond_broadcast(&(pool->queue_not_full));

    /* Wait workers to finish/join */
    for (int i = 0; i < pool->thread_num; ++i) {
        pthread_join(pool->workers[i], NULL);
    }

    /* Release resources */
    pthread_mutex_destroy(&(pool->queue_mutex));

    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_empty));
    pthread_cond_destroy(&(pool->queue_not_full));

    /* TODO: Check no mem leaks */
    free(pool->workers);
    free(pool->queue);
    free(pool);

    return 0;
}

/* Add a new task to the pthreapool queue */
int pthreadpool_add_task(pthreadpool *pool, void* (*function)(void *), void *arg) {

    assert(pool != NULL);
    assert(function != NULL);
    assert(arg != NULL);

    //fprintf(stderr, "[pthreadpool] task\n");

    /* Acquire mutex */
    pthread_mutex_lock(&pool->queue_mutex);

    /* Critical Section BEGINS here */

    /* If queue is full then wait */
    while (pool->task_queue_sz == pool->queue_sz) {
        pthread_cond_wait(&(pool->queue_not_full), &(pool->queue_mutex));
    }

    /* Make sure that both the pool and queue are not closed */
    if (pool->pthreadpool_close || pool->task_queue_close) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1;
    }

    /* Add task to queue */
    pool->queue[pool->rear].function = function;
    pool->queue[pool->rear].arg = arg;

    /* Update rear and curr task count */
    pool->rear = pool->rear + 1;
    pool->task_queue_sz++;

    /* If queue was empty and workers were waiting, notify them */
    if (pool->front == 0) {
        pthread_cond_broadcast(&(pool->queue_not_empty));
    }

    /* END of Critical Section */

    /* Release mutex */
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}

void* pthreadpool_worker_function(void *arg) {

    pthreadpool *pool = (pthreadpool *)arg;
    task t;

    //fprintf(stderr, "[pthreadpool] I am thread %lu\n", pthread_self());

    while( 1 ) {
        pthread_mutex_lock(&(pool->queue_mutex));
        /* Critical BEGINS */

        /* If queue is empty then wait */
        while ((pool->task_queue_sz == 0) && !pool->pthreadpool_close) {
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->queue_mutex));
        }

        /* If pool closed, exit the thread */
        if (pool->pthreadpool_close) {
            pthread_mutex_unlock(&(pool->queue_mutex));
            pthread_exit(NULL);
        }

        /* Get task from queue front */
        t.function = pool->queue[pool->front].function;
        t.arg = pool->queue[pool->front].arg;

        /* Update front and curr task count */
        pool->front++;
        pool->front = (pool->front == pool->queue_sz) ? 0 : pool->front;
        pool->task_queue_sz--;

        /* If queue empty, then signal */
        if (pool->task_queue_sz == 0) {
            pool->front = pool->rear = 0;
            pthread_cond_signal(&(pool->queue_empty));
        }

        /* If */
        if ( pool->rear != pool->queue_sz ){
            pthread_cond_broadcast(&(pool->queue_not_full));
        }

        /* Critical ENDS */
        pthread_mutex_unlock(&(pool->queue_mutex));

        /* Run the task */
        (*(t.function))(t.arg);
    }
    //return;
}