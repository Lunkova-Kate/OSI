#include "mythread.h"

#define STACK_SIZE (2 * 1024 * 1024)
#define GUARD_PAGE_SIZE 4096

#define THREAD_RUNNING 0
#define THREAD_FINISHED 1

static void free_thread_resources(mythread_dt *thread_data) {
  if (thread_data == NULL)
    return;

  if (thread_data->stack != NULL) {
    munmap(thread_data->stack, thread_data->stack_size);
    thread_data->stack = NULL;
  }

  free(thread_data);
}

mythread_t mythread_self(void) { return (mythread_t)(long)syscall(SYS_gettid); }

static int thread_entry(void *arg) {
  mythread_dt *thread_data = (mythread_dt *)arg;

  void *result = thread_data->start_routine(thread_data->arg);

  thread_data->return_value = result;
  __atomic_store_n(&thread_data->finished, THREAD_FINISHED, __ATOMIC_SEQ_CST);
  futex(&thread_data->finished, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);

  syscall(SYS_exit, 0);

  return 0;
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *),
                    void *arg) {
  size_t total_size = GUARD_PAGE_SIZE + STACK_SIZE;
  int protection = PROT_NONE;
  int map_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK;

  void *memory_region = mmap(NULL, total_size, protection, map_flags, -1, 0);
  if (memory_region == MAP_FAILED) {
    perror("mmap failed");
    return FAILED;
  }

  void *stack_start = (char *)memory_region + GUARD_PAGE_SIZE;

  mythread_dt *thread_data = malloc(sizeof(mythread_dt));
  if (thread_data == NULL) {
    perror("malloc failed");
    munmap(memory_region, total_size);
    return FAILED;
  }

  protection = PROT_READ | PROT_WRITE;
  if (mprotect(stack_start, STACK_SIZE, protection) == FAILED) {
    perror("mprotect failed");
    munmap(memory_region, total_size);
    free(thread_data);
    return FAILED;
  }

  memset(thread_data, 0, sizeof(mythread_dt));
  thread_data->stack = memory_region;
  thread_data->stack_size = total_size;
  thread_data->start_routine = start_routine;
  thread_data->arg = arg;
  thread_data->join_called = 0;
  thread_data->finished = THREAD_RUNNING;
  thread_data->return_value = NULL;
  thread_data->tid = 0;

  void *stack_top = (char *)stack_start + STACK_SIZE;

  int clone_flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
                    CLONE_THREAD | CLONE_SYSVSEM;

  pid_t pid = clone(thread_entry, stack_top, clone_flags, thread_data);
  if (pid == FAILED) {
    perror("clone failed");
    munmap(memory_region, total_size);
    free(thread_data);
    return FAILED;
  }

  thread_data->tid = pid;
  *thread = (mythread_t)thread_data;

  return SUCCESS;
}

int mythread_join(mythread_t thread, void **retval) {
  mythread_dt *thread_data = (mythread_dt *)thread;

  if (thread_data == NULL) {
    errno = ESRCH;
    return FAILED;
  }

  if (thread_data->join_called) {
    errno = EINVAL;
    return FAILED;
  }

  thread_data->join_called = 1;

  while (__atomic_load_n(&thread_data->finished, __ATOMIC_SEQ_CST) !=
         THREAD_FINISHED) {
    int futex_result = futex(&thread_data->finished, FUTEX_WAIT, THREAD_RUNNING,
                             NULL, NULL, 0);

    if (futex_result == FAILED) {
      if (errno == EAGAIN) {
        continue;
      } else if (errno == EINTR) {
        continue;
      } else {
        perror("futex wait failed");
        return FAILED;
      }
    }
  }

  if (retval != NULL) {
    *retval = thread_data->return_value;
  }

  free_thread_resources(thread_data);

  return SUCCESS;
}
