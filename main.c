#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// generic work for thread to execute
typedef struct thread_work {
  void (*func)(void *);
  void *args;
} thread_work_t;

typedef struct threadpool {
  // shared lock for grabbing work
  pthread_mutex_t mutex;

  // references to thread handlers in the pool
  pthread_t *workers;

  // how many worker threads we have
  int workers_len;

  // list of work for threads to work off
  thread_work_t *work;

  // how much work there is in the pool
  int thread_work_queue_len;

  // how much work can be stored in the pool at once
  int thread_work_queue_capacity;

} threadpool_t;

void *thread_func(void *args) {
  (void)args;

  for (int i = 0; i < 1; i++) {
    printf("in the thread!\n");
  }

  return NULL;
}

threadpool_t *threadpool_init(int thread_capacity) {
  threadpool_t *pool = malloc(sizeof(threadpool_t));

  int ret = pthread_mutex_init(&pool->mutex, NULL);

  if (ret != 0) {
    fprintf(stderr, "unable to create mutex\n");
  }

  pool->workers = malloc(sizeof(pthread_t) * thread_capacity);
  pool->workers_len = thread_capacity;

  for (int i = 0; i < thread_capacity; i++) {
    pthread_create(&pool->workers[i], NULL, thread_func, NULL);
  }

  pool->thread_work_queue_len = 0;
  pool->thread_work_queue_capacity = 100;

  return pool;
}

int threadpool_cleanup(threadpool_t *pool) {
  free(pool->workers);
  free(pool);
  return 0;
}

int threadpool_join(threadpool_t *pool) {
  for (int i = 0; i < pool->workers_len; ++i) {
    int ret = pthread_join(pool->workers[i], NULL);
    if (ret != 0) {
      fprintf(stderr, "failed to join thread\n");
    }
  }
  return 0;
}

int threadpool_add_work(threadpool_t *p, thread_work_t work) {
  int ret = pthread_mutex_lock(&p->mutex);

  if (ret != 0) {
    fprintf(stderr, "failed to aquire mutex\n");
    return 1;
  }

  if (p->thread_work_queue_len == p->thread_work_queue_capacity) {
    fprintf(stderr, "work queue full\n");
    pthread_mutex_unlock(&p->mutex);
    return 1;
  }

  p->work[p->thread_work_queue_len] = work;
  ++p->thread_work_queue_len;

  ret = pthread_mutex_unlock(&p->mutex);

  if (ret != 0) {
    fprintf(stderr, "failed to release mutex\n");
  }

  return 0;
}

int main() {
  threadpool_t *pool = threadpool_init(10);
  threadpool_join(pool);
  threadpool_cleanup(pool);
}
