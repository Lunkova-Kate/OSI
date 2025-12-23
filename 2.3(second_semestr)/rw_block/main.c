#include <stdio.h>
#include <time.h>  
#include <stdlib.h> 
#include "Node.h"
#include "Utils.h"
#include "Thread_readers.h"
#include "Thread_swappers.h"

atomic_int iter_inc = 0, pairs_inc = 0;
atomic_int iter_dec = 0, pairs_dec = 0;
atomic_int iter_eq  = 0, pairs_eq  = 0;

atomic_int running = 1;
atomic_int swap_count = 0;

int create_list_of_size(Storage* s, int node_size, int max_str_len) {

    for (int i = 0; i < node_size; i++) {
        char* random_str = generate_random_string(max_str_len);
        if (!random_str) {
            fprintf(stderr, "Failed to generate string\n");
            return FAILED;
        }
        if (add_node(s, random_str) != SUCCESS) {
            fprintf(stderr, "Error adding Node #%d\n", i);
            free(random_str);
            return FAILED;
        }
        
        free(random_str);

    }
    return SUCCESS;
}

void test(int node_size, int test_duration_sec) {
    printf("\n TESTING NODE SIZE: %d \n", node_size);
    Storage s;
    s.first = NULL;

    if (pthread_rwlock_init(&s.head_lock, NULL) != 0)  {
        fprintf(stderr, "Error: mutex init failed\n");
        return;
    }

    if (create_list_of_size(&s, node_size, 50) != SUCCESS) {
        fprintf(stderr, "Failed to create list\n");
        pthread_rwlock_destroy(&s.head_lock);
        return;
    }

    atomic_store(&iter_inc, 0);
    atomic_store(&pairs_inc, 0);
    atomic_store(&iter_dec, 0);
    atomic_store(&pairs_dec, 0);
    atomic_store(&iter_eq, 0);
    atomic_store(&pairs_eq, 0);
    atomic_store(&swap_count, 0);

    ThreadArgs args[3] = {
        { &s, is_increasing, &iter_inc, &pairs_inc },
        { &s, is_decreasing, &iter_dec, &pairs_dec },
        { &s, is_equal_len,  &iter_eq,  &pairs_eq  }
    };

    pthread_t readers[3], modifiers[3];

    int err = 0;
     for (int i = 0; i < 3; i++) {
        err = pthread_create(&readers[i], NULL, generic_thread, &args[i]);
        if (err != 0) {
            fprintf(stderr, "Error creating reader %d: %s\n", i, strerror(err));
            cleanup_storage(&s);
            return;
        }
    }
for (int i = 0; i < 3; i++) {
    err = pthread_create(&modifiers[i], NULL, check_swap, &s);
    if (err != 0) {
        fprintf(stderr, "Error creating modifier %d: %s\n", i, strerror(err));
        atomic_store(&running, 0);
        for (int j = 0; j < i; j++) {
            pthread_join(modifiers[j], NULL);
        }
        for (int j = 0; j < 3; j++) {
            pthread_join(readers[j], NULL);
        }
        cleanup_storage(&s);
        return;
    }
}
    sleep(test_duration_sec);
    atomic_store(&running, 0);
    for (int i = 0; i < 3; i++) {
        pthread_join(readers[i], NULL);
        pthread_join(modifiers[i], NULL);
    }

    printf("Возрастающих пар:   %d (итераций: %d)\n", (int)pairs_inc, (int)iter_inc);
    printf("Убывающих пар:     %d (итераций: %d)\n", (int)pairs_dec, (int)iter_dec);
    printf("Равных пар:        %d (итераций: %d)\n", (int)pairs_eq,  (int)iter_eq);
    printf("Перестановок:      %d\n", (int)swap_count);

    cleanup_storage(&s);
    atomic_store(&running, 1);
}

int main() {
    
    srand(time(NULL));
    
    int test_sizes[] = {100, 1000, 10000, 100000};
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (int i = 0; i < num_tests; i++) {
        int size = test_sizes[i];
        int test_duration = (size <= 1000) ? 5 : 3;
        test(size, test_duration);

        if (i < num_tests - 1) {
            sleep(1);
        }
    }

        return SUCCESS;
}
