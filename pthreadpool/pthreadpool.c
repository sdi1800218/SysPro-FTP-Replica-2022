/* Reset semaphore to 0 */
static void pth_sem_reset(pth_sem *pth_sem_p) {
	pth_sem_init(pth_sem_p, 0);
}


/* Post to at least one thread */
static void pth_sem_post(pth_sem *pth_sem_p) {
	pthread_mutex_lock(&pth_sem_p->mutex);
	pth_sem_p->v = 1;
	pthread_cond_signal(&pth_sem_p->cond);
	pthread_mutex_unlock(&pth_sem_p->mutex);
}

/* Post to all threads */
static void pth_sem_post_all(pth_sem *pth_sem_p) {
	pthread_mutex_lock(&pth_sem_p->mutex);
	pth_sem_p->v = 1;
	pthread_cond_broadcast(&pth_sem_p->cond);
	pthread_mutex_unlock(&pth_sem_p->mutex);
}


/* Wait on semaphore until semaphore has value 0 */
static void pth_sem_wait(pth_sem* pth_sem_p) {
	pthread_mutex_lock(&pth_sem_p->mutex);
	while (pth_sem_p->v != 1) {
		pthread_cond_wait(&pth_sem_p->cond, &pth_sem_p->mutex);
	}
	pth_sem_p->v = 0;
	pthread_mutex_unlock(&pth_sem_p->mutex);
}