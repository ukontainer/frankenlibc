#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

static sem_t sem;

void *thread_func(void* args)
{
	int err;

	do {
		err = sem_wait(&sem);
	} while (err < 0 && errno == EINTR);

	fprintf(stderr, "thread_func\n");
	return NULL;
}

void *post_func(void* args)
{
	sem_post(&sem);
	fprintf(stderr, "post_func\n");
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thread;

	sem_init(&sem, 0, 0);
	if (pthread_create(&thread, NULL, thread_func, (void *)NULL) != 0) {
		perror("pthread_create");
		return -1;
	}

	if (pthread_create(&thread, NULL, post_func, (void *)NULL) != 0) {
		perror("pthread_create post");
		return -1;
	}

	if (pthread_join(thread, NULL) != 0) {
		perror("pthread_join");
		return -1;
	}

	return 0;
}
