#include "Utils.h"
#include <stdlib.h> 
#include <stdlib.h>


Node* create_node(const char* str) {
    Node* node = malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Error: malloc for create Node\n");
        return NULL;
    }
    
    int len = snprintf(node->value, sizeof(node->value), "%s", str);
    if (len < 0 || (size_t)len >= sizeof(node->value)) {
        fprintf(stderr, "Error: str ne vlazit :( '%s'\n", str);
    }

    node->next = NULL;
    //node->ref_count = 1;

    if (pthread_spin_init(&node->lock, PTHREAD_PROCESS_PRIVATE) != 0) {
        fprintf(stderr, "Error: mutex for create Node\n");
        free(node);
        return NULL;
    }

    return node;
}

int add_node(Storage* s, const char* str) {
    Node* node = create_node(str);
    if (!node) return FAILED;

    pthread_spin_lock(&s->head_lock);
    if (!s->first) {
        s->first = node;
    } else {
        Node* cur = s->first;
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = node;
    }
    pthread_spin_unlock(&s->head_lock);
    return SUCCESS;
}

void cleanup_storage(Storage* s) {
    pthread_spin_lock(&s->head_lock);
    Node* current_node = s->first;
    
    s->first = NULL;
    pthread_spin_unlock(&s->head_lock); //!
    while (current_node) {
        Node* next = current_node->next;
        pthread_spin_destroy(&current_node->lock);
        free(current_node);
        current_node = next;
    }
    pthread_spin_destroy(&s->head_lock);
}

char* generate_random_string(int max_len) {
      char* str = malloc(max_len + 1);
    if (!str){
        fprintf(stderr, "Error: malloc for create str\n");
        return NULL;
    }
    int len = (rand() % max_len) + 1;

     for (int i = 0; i < len; i++) {
        str[i] = 'A' + (rand() % 26);
    }
    str[len] = '\0';
    
    return str;
}