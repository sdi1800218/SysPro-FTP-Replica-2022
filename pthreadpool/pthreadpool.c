#include <stdlib.h>

#include "pthreadpool.h"

/* Initialize thread pool values */
pthreadpool_t* pthreadpool_create(int thread_number, int queue_size) {
    int i;
    pthreadpool_t pool;

    pool = (pthreadpool_t)malloc(sizeof(struct pthreadpool));
    if (pool == NULL) {
        perror("malloc");
        exit(0);
    }

    pool->thread_num = thread_number;
    pool->queue_sz = queue_size + 1;
    //pool->num_threads = ;
    pool->tpid = NULL;
    pool->head = 0;
    pool->tail = 0;
    pool->task_queue_close = 0;
    pool->pthreadpool_close = 0;

    if (pthread_mutex_init(&pool->queue_mutex, NULL) == -1) {
        perror("[pthreadpool] pthread_mutex_init");
        free(pool);
        exit(0);
    }

    if (pthread_cond_init(&pool->queue_not_full, NULL) == -1) {
        perror("pthread_mutex_init");
        free(pool);
        exit(0);
    }

    if (pthread_cond_init(&pool->queue_not_empty, NULL) == -1) {
        perror("pthread_mutex_init");
        free(pool);
        exit(0);
    }

    if (pthread_cond_init(&pool->queue_empty, NULL) == -1) {
        perror("pthread_mutex_init");
        free(pool);
        exit(0);
    }

    if ((pool->queue = malloc(sizeof(struct task) *
            pool->queue_sz)) == NULL) {
        perror("malloc");
        free(pool);
        exit(0);
    }

    if ((pool->tpid = malloc(sizeof(pthread_t) * num_worker_threads)) == NULL) { 
        perror("malloc"); 
        free(pool); 
        free(pool->queue); 
        exit(0); 
    }

    for (i = 0; i < num_worker_threads; i++) {
        if (pthread_create(&pool->tpid[i], NULL, tpool_thread,
            (void *)pool) != 0) {
        perror("pthread_create");
        exit(0);
        }
    }

  *tpoolp = pool;

}

int pthreadpool_destroy(pthreadpool_t *pool) {
    assert(pool != NULL);
    pthread_mutex_lock(&(pool->queue_mutex));

    //The thread pool has exited, just return directly
    if (pool->task_queue_close || pool->pthreadpool_close) {
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }

    //Set queue close flag
    pool->queue_close = 1;
    while (pool->queue_cur_num != 0) {
        pthread_cond_wait(&(pool->queue_empty), &(pool->mutex));  //Waiting queue is empty
    }

    pool->pool_close = 1;      //Set thread pool close flag
    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_broadcast(&(pool->queue_not_empty));  //Wake up the blocking thread in the thread pool
    pthread_cond_broadcast(&(pool->queue_not_full));   //The threadpool_add_job function to wake up the added task
    int i;
    for (i = 0; i < pool->thread_num; ++i) {
        pthread_join(pool->pthreads[i], NULL);    //Wait for all threads in the thread pool to finish executing
    }

    //Clean up resources
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_empty));
    pthread_cond_destroy(&(pool->queue_not_full));
    free(pool->pthreads);
    struct job *p;
    while (pool->head != NULL) {
        p = pool->head;
        pool->head = p->next;
        free(p);
    }
    free(pool);

    return 0;
}