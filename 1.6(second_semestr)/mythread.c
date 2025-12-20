#include "mythread.h"

#define STACK_SIZE (2 * 1024 * 1024)
#define GUARD_PAGE_SIZE 4096

mythread_t mythread_self(void){
    return syscall(SYS_gettid);
}

static int thread_entry(void* arg){
    mythread_dt* thread_with_data = (mythread_dt*)arg;
    
    thread_with_data->start_routine(thread_with_data->arg);

    syscall(SYS_exit, SUCCESS);

    return SUCCESS;
}



int mythread_create(mythread_t* thread, void*(*start_routine)(void*), void* arg){
    size_t total_size = GUARD_PAGE_SIZE + STACK_SIZE + sizeof(mythread_dt);
    int protection = PROT_NONE;  
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK;

    void* memory_region = mmap(NULL, total_size, protection, flags, -1, 0);
    if (memory_region == MAP_FAILED) {
        perror("mmap failed");
        return FAILED;
    }

    void* stack_start = (char*)memory_region + GUARD_PAGE_SIZE;
    mythread_dt* thread_data = (mythread_dt*)((char*)memory_region + total_size - sizeof(mythread_dt));

    protection = PROT_READ | PROT_WRITE;
    if (mprotect(stack_start, STACK_SIZE, protection) == FAILED) {
        perror("mprotect failed");
        munmap(memory_region, total_size);
        return FAILED;
    }

    thread_data->stack = memory_region;
    thread_data->start_routine = start_routine;
    thread_data->arg = arg;

    void* stack_top = (char*)stack_start + STACK_SIZE;

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
                CLONE_THREAD | CLONE_SYSVSEM;
    pid_t pid = clone(thread_entry, stack_top, flags, thread_data);
    if (pid == FAILED) {
        perror("clone failed");
        munmap(memory_region, total_size);
        return FAILED;
    }
    *thread = pid;
    return SUCCESS;
}