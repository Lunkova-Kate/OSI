#ifndef NODE_H
#define NODE_H 
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h> 

typedef struct _Node {
char value[100]; 
struct _Node* next;
pthread_rwlock_t rwlock; 
} Node;

typedef struct _Storage {
Node *first;
pthread_mutex_t head_lock; 
} Storage;

typedef struct {
    Storage* storage;
    bool (*predicate)(int, int);
    atomic_int* iter_counter;
    atomic_int* pairs_counter;
} ThreadArgs;

extern atomic_int iter_inc, pairs_inc;
extern atomic_int iter_dec, pairs_dec;
extern atomic_int iter_eq, pairs_eq;

extern atomic_int running; 
extern atomic_int swap_count;

typedef enum {
    FAILED = -1,
    SUCCESS = 0,
}my_error_code;

#endif