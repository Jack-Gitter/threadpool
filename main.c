#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// generic work for thread to execute
typedef struct thread_work {
  void *(*func)(void *);
  void *args;
} thread_work_t;

typedef struct threadpool {
  // shared lock for grabbing work
  pthread_mutex_t mutex;

  // references to thread handlers in the pool
  pthread_t *workers;

  pthread_cond_t work_available;

  // how many worker threads we have
  int workers_len;

  // list of work for threads to work off
  thread_work_t *work;

  // how much work there is in the pool
  int thread_work_queue_len;

  // how much work can be stored in the pool at once
  int thread_work_queue_capacity;

  bool shutdown;

} threadpool_t;

void *thread_func(void *args) {
  threadpool_t *pool = (threadpool_t *)args;

  while (1) {

    pthread_mutex_lock(&pool->mutex);

    while (!pool->shutdown && pool->thread_work_queue_len == 0) {
      pthread_cond_wait(&pool->work_available, &pool->mutex);
    }

    if (pool->shutdown && pool->thread_work_queue_len == 0) {
      pthread_mutex_unlock(&pool->mutex);
      pthread_exit(NULL);
    }

    thread_work_t work = pool->work[pool->thread_work_queue_len - 1];
    pool->thread_work_queue_len -= 1;

    pthread_mutex_unlock(&pool->mutex);

    work.func(work.args);
  }

  return NULL;
}

int threadpool_cleanup(threadpool_t *pool) {
  free(pool->work);
  free(pool->workers);
  free(pool);
  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->work_available);
  return 0;
}

int threadpool_shutdown(threadpool_t *pool) {
  pthread_mutex_lock(&pool->mutex);
  pool->shutdown = true;
  pthread_cond_broadcast(&pool->work_available);
  pthread_mutex_unlock(&pool->mutex);

  for (int i = 0; i < pool->workers_len; i++) {
    pthread_join(pool->workers[i], NULL);
  }

  threadpool_cleanup(pool);

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

  pthread_cond_signal(&p->work_available);

  return 0;
}

threadpool_t *threadpool_init(int thread_capacity) {
  threadpool_t *pool = malloc(sizeof(threadpool_t));

  int ret = pthread_mutex_init(&pool->mutex, NULL);

  if (ret != 0) {
    fprintf(stderr, "unable to create mutex\n");
  }

  pool->shutdown = false;
  pool->workers = malloc(sizeof(pthread_t) * thread_capacity);
  pool->workers_len = thread_capacity;
  pool->thread_work_queue_len = 0;
  pool->thread_work_queue_capacity = 100;
  pool->work = malloc(sizeof(thread_work_t) * pool->thread_work_queue_capacity);

  pthread_cond_init(&pool->work_available, NULL);

  for (int i = 0; i < thread_capacity; i++) {
    pthread_create(&pool->workers[i], NULL, thread_func, pool);
  }

  return pool;
}

void *add_two_nums(void *nums) {
  int *vals = (int *)nums;
  printf("value is %d\n", vals[0] + vals[1]);
  return NULL;
}

int main() {
  threadpool_t *pool = threadpool_init(10);

  int args[2] = {1, 2};
  int args1[2] = {2, 2};
  int args2[2] = {3, 2};
  int args3[2] = {4, 2};

  thread_work_t work = {add_two_nums, (void *)args};
  thread_work_t work1 = {add_two_nums, (void *)args1};
  thread_work_t work2 = {add_two_nums, (void *)args2};
  thread_work_t work3 = {add_two_nums, (void *)args3};
  threadpool_add_work(pool, work);
  threadpool_add_work(pool, work1);
  threadpool_add_work(pool, work2);
  threadpool_add_work(pool, work3);

  threadpool_shutdown(pool);
}
