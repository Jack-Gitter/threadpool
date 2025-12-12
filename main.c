#include <pthread.h>

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

  // how many worker threads we can have total
  int workers_capacity;

  // list of work for threads to work off
  thread_work_t *work;

  // how much work there is in the pool
  int thread_work_queue_len;

  // how much work can be stored in the pool at once
  int thread_work_queue_capacity;

} threadpool_t;

int threadpool_add_work(thread_work_t *work) {
  // aquire lock
  // check that we are not at capacity
  // add work
  // increase counters
  // release mutex
  return 0;
}

void *thread_func(void *args) {

  // race for work
  return NULL;
}

int main() {}
