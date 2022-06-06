#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#include <pthread.h>    /* for everything */

/* Binary pthread-based semaphore */
typedef struct pth_sem {
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
	int v;
} pth_sem;

void pthreadpool_create();
void pthreadpool_destroy();

#endif /* __PTHREADPOOL_H__ */