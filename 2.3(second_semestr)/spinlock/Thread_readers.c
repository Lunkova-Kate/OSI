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
    
    pthread_spin_lock(&storage->head_lock);
    Node* current_node = storage->first;
    

    if (!current_node || !current_node->next) {
        pthread_spin_unlock(&storage->head_lock);
        return SUCCESS;
    }
    
    Node* node1 = current_node;

    pthread_spin_lock(&node1->lock);
    pthread_spin_unlock(&storage->head_lock);
    
    while (node1 && node1->next) {
        Node* node2 = node1->next;
        
        pthread_spin_lock(&node2->lock);
        int len1 = strlen(node1->value);
        int len2 = strlen(node2->value);
        
        if (predicate(len1, len2)) {
            count++;
        }
        

        pthread_spin_unlock(&node1->lock);
        node1 = node2;  
    }

    if (node1) {
        pthread_spin_unlock(&node1->lock);
    }
    return count;
}

void* generic_thread(void* arg) {
    ThreadArgs* t = (ThreadArgs*)arg;
    
    while (atomic_load(&running)) {
        int cnt = count_pairs(t->storage, t->predicate);
        atomic_fetch_add(t->pairs_counter, cnt);
        atomic_fetch_add(t->iter_counter, 1);

    }
    return NULL;
}