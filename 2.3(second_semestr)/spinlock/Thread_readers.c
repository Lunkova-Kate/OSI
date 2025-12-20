#include "Node.h"

bool is_increasing(int len1, int len2) { 
    return len1 < len2; 
}
bool is_decreasing(int len1, int len2) { 
    return len1 > len2; 
}
bool is_equal_len(int len1, int len2) { 
    return len1 == len2; 
}

count_pairs(Storage* storage, bool(*predicate)(int len1, int len2)) {
    pthread_mutex_lock(&storage->head_lock);
    Node* current_node = storage->first;
    pthread_mutex_unlock(&storage->head_lock);

    if(!current_node || !current_node->next) return 0;

    int count = 0;
    while (current_node) {
        Node* next_node;
        pthread_mutex_lock(&current_node->lock);
        next_node=current_node->next;
        if (!next_node) {
            pthread_mutex_unlock(&current_node->lock);
            break;
        }
        pthread_mutex_lock(&next_node->lock);

        int len1 = strlen(current_node->value);
        int len2 = strlen(next_node->value);
        
        if (predicate(len1,len2)) {
            count++;
        }

        pthread_mutex_unlock(&next_node->lock);
        pthread_mutex_unlock(&current_node->lock);
        current_node = next_node;
    }   
    return count;
}

void* generic_thread(void* arg) {
    ThreadArgs* t = (ThreadArgs*)arg;
    while (running) {
        int cnt = count_pairs(t->storage, t->predicate);
        atomic_fetch_add(t->pairs_counter, cnt);
        atomic_fetch_add(t->iter_counter, 1);
    }
    return NULL;
}
