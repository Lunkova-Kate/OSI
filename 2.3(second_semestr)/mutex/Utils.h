#ifndef UTILS_H
#define UTILS_H

#include "Node.h"

Node* create_node(const char* str);
int add_node(Storage* s, const char* str);
void cleanup_storage(Storage* s);

#endif