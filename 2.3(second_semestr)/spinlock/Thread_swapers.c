#include "Node.h"

bool swap_first_two(Storage* s) {
    pthread_mutex_lock(&s->head_lock);

    Node* A =s->first;

    if(!A || !A->next) {
        pthread_mutex_unlock(&s->head_lock);
    return false;
    }
    Node* B = A->next;
    pthread_mutex_lock(&A->lock);
    pthread_mutex_lock(&B->lock);

    bool need_swap = (strlen(A->value) > strlen(B->value));

    if(need_swap) {
        s->first = B;
        A->next = B->next;
        B->next = A;
    }

    pthread_mutex_unlock(&B->lock);
    pthread_mutex_unlock(&A->lock);
    pthread_mutex_unlock(&s->head_lock);

    return need_swap;
}

void* check_swap(void* arg) {
    Storage* s = (Storage*)arg;
    while(running) {
        if(swap_first_two) {
            atomic_fetch_add(&swap_count, 1);
        }
        usleap(500);
    }
    return NULL;
}