#include "Thread_readers.h" 


bool is_increasing(int len1, int len2) { 
    return len1 < len2; 
}
bool is_decreasing(int len1, int len2) { 
    return len1 > len2; 
}
bool is_equal_len(int len1, int len2) { 
    return len1 == len2; 
}

int count_pairs(Storage* storage, bool(*predicate)(int len1, int len2)) {
    int count = 0;
    
    pthread_rwlock_rdlock(&storage->head_lock);
    Node* current_node = storage->first;
    

    if (!current_node || !current_node->next) {
        pthread_rwlock_unlock(&storage->head_lock);
        return SUCCESS;
    }
    

    Node* node1 = current_node;
    pthread_rwlock_rdlock(&node1->rwlock);
    pthread_rwlock_unlock(&storage->head_lock);

    while (node1 && node1->next) {
        Node* node2 = node1->next;
        
        pthread_rwlock_rdlock(&node2->rwlock);
        

        int len1 = strlen(node1->value);
        int len2 = strlen(node2->value);
        
        if (predicate(len1, len2)) {
            count++;
        }
    
         pthread_rwlock_unlock(&node1->rwlock);
    
        node1 = node2;  
    }
    
    if (node1) {
        pthread_rwlock_unlock(&node1->rwlock);
    }
    
    return count;
}

void* generic_thread(void* arg) {
    ThreadArgs* t = (ThreadArgs*)arg;
    
    while (atomic_load(&running)) {
        int cnt = count_pairs(t->storage, t->predicate);
        atomic_fetch_add(t->pairs_counter, cnt);
        atomic_fetch_add(t->iter_counter, 1);
       // nanosleep(&sleep_time, NULL);
    }
    return NULL;
}