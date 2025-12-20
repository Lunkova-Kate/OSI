#ifndef MYTHREAD_H
#define MYTHREAD_H
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
typedef pid_t mythread_t;

typedef struct {
    void*(*start_routine)(void*);
    void* arg;
    void* stack;
}mythread_dt;

mythread_t mythread_self(void);
int mythread_create(mythread_t* thread, void*(*start_routine)(void*), void* arg);

typedef enum {
   FAILED = -1,
   SUCCESS = 0,
}mythread_error_code;

#endif // MYTHREAD_H
