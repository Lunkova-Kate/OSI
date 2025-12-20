
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>

typedef struct _Node {
char value[100]; 
struct _Node* next;
pthread_mutex_t lock;
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

_Atomic int iter_inc = 0, pairs_inc = 0;
_Atomic int iter_dec = 0, pairs_dec = 0;
_Atomic int iter_eq  = 0, pairs_eq  = 0;

volatile _Atomic int running = 1;
_Atomic int swap_count = 0;