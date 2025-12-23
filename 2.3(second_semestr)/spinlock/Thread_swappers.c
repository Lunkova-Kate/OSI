#include "Thread_swappers.h"

bool swap_first_two(Storage* s)
{
    if (!s) return false;

    pthread_spin_lock(&s->head_lock);

    Node* A = s->first;
    if (!A || !A->next)
    {
        pthread_spin_unlock(&s->head_lock);
        return false;
    }

    Node* B = A->next;

    pthread_spin_lock(&A->lock);
    pthread_spin_lock(&B->lock);

    bool need_swap = (strlen(A->value) > strlen(B->value)) || (rand() % 4 == 0);

    if (need_swap)
    {
        s->first = B;
        A->next = B->next;
        B->next = A;
    }

    pthread_spin_unlock(&B->lock);
    pthread_spin_unlock(&A->lock);
    pthread_spin_unlock(&s->head_lock);

    return need_swap;
}

void* check_swap(void* arg)
{
    Storage* s = (Storage*)arg;

    while (atomic_load(&running))
    {
        if (swap_first_two(s))
        {
            atomic_fetch_add(&swap_count, 1);
        }
    }
    return NULL;
}