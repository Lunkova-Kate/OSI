#ifndef THREAD_READERS_H
#define THREAD_READERS_H

#include "Node.h"

bool is_increasing(int len1, int len2);
bool is_decreasing(int len1, int len2);
bool is_equal_len(int len1, int len2);

int count_pairs(Storage* storage, bool (*predicate)(int, int));
void* generic_thread(void* arg);

#endif