#ifndef MYTHREAD_H
#define MYTHREAD_H
#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct mythread_internal *mythread_t;

typedef struct mythread_internal {
  void *(*start_routine)(void *);
  void *arg;
  void *stack;
  size_t stack_size;
  pid_t tid;
  int join_called;
  void *return_value;
  int finished;
} mythread_dt;

mythread_t mythread_self(void);
int mythread_create(mythread_t *thread, void *(*start_routine)(void *),
                    void *arg);
int mythread_join(mythread_t thread, void **retval);

typedef enum {
  FAILED = -1,
  SUCCESS = 0,
} mythread_error_code;

static inline int futex(int *uaddr, int futex_op, int val,
                        const struct timespec *timeout, int *uaddr2, int val3) {
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

#endif // MYTHREAD_H
