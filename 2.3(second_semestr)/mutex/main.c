#include <stdio.h>
#include <time.h>  
#include <stdlib.h> 
#include "Node.h"
#include "Utils.h"
#include "Thread_readers.h"
#include "Thread_swapers.h"

atomic_int iter_inc = 0, pairs_inc = 0;
atomic_int iter_dec = 0, pairs_dec = 0;
atomic_int iter_eq  = 0, pairs_eq  = 0;

atomic_int running = 1;
atomic_int swap_count = 0;



int main() {
    Storage s = {0};

    if (pthread_mutex_init(&s.head_lock, NULL) != 0) {
        fprintf(stderr, "Error: mutex for create head_lock\n");
        return FAILED;
    }
    s.first = NULL;
    srand(time(NULL));
    const char* words[] = {"aqw", "b", "csdsdghcc","b" , "fdfd", "ee", "fffgrggg", "g", "hhhhhhh"};
    int word_count = sizeof(words) / sizeof(words[0]);
    for (int i = 0; i < 100; i++) {
        const char* word = words[rand() % word_count];
        if (add_node(&s, word) != SUCCESS) {
        fprintf(stderr, "Error for add Node #%d\n", i);
        cleanup_storage(&s);
        return FAILED;
        }
    }
    printf("IM CREATE LIST\n");

    ThreadArgs args[3] = {
        { &s, is_increasing, &iter_inc, &pairs_inc },
        { &s, is_decreasing, &iter_dec, &pairs_dec },
        { &s, is_equal_len,  &iter_eq,  &pairs_eq  }
    };

    pthread_t readers[3], modifiers[3];
    int created_threads = 0;
    int err = 0;

     for (int i = 0; i < 3; i++) {
        err = pthread_create(&readers[i], NULL, generic_thread, &args[i]);
        if (err != 0) {
            fprintf(stderr, "Error pthread_create (reader %d): %s\n", i, strerror(err));
            running = 0;
            for (int j = 0; j < i; j++) {
                pthread_join(readers[j], NULL);
            }
            cleanup_storage(&s);
            return FAILED;
        }
        created_threads++;
    }

    for (int i = 0; i < 3; i++) {
    err = pthread_create(&modifiers[i], NULL, check_swap, &s);

    if (err != 0) {
        fprintf(stderr, "Error pthread_create (modifier): %s\n", strerror(err));
        running = 0;
        for (int i = 0; i < created_threads; i++) {
            pthread_join(readers[i], NULL);
        }
        cleanup_storage(&s);
        return FAILED;
    }
    }

    //завершаемся
    sleep(3);
    running = 0; 

    for (int i = 0; i < 3; i++) {
        err = pthread_join(readers[i], NULL);
        if (err != 0) {
            fprintf(stderr, "Error: pthread_join(reader %d) вернул %s\n", i, strerror(err));
        }
    }
    for (int i = 0; i < 3; i++) {
    err = pthread_join(modifiers[i], NULL);
        if (err != 0) {
            fprintf(stderr, "Error: pthread_join(modifier) вернул %s\n", strerror(err));
        }
    }
    printf("Возрастающих пар:   %d (итераций: %d)\n", (int)pairs_inc, (int)iter_inc);
    printf("Убывающих пар:     %d (итераций: %d)\n", (int)pairs_dec, (int)iter_dec);
    printf("Равных пар:        %d (итераций: %d)\n", (int)pairs_eq,  (int)iter_eq);
    printf("Перестановок:      %d\n", (int)swap_count);

    Node* cur = s.first;
    while (cur) {
        printf("[%s] → ", cur->value);
        cur = cur->next;
    }
    printf("NULL\n"); 


    cleanup_storage(&s);
    return SUCCESS;

}