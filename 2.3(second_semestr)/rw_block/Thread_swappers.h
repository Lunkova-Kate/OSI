#ifndef THREAD_SWAPPERS_H
#define THREAD_SWAPPERS_H

#include "Node.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

bool swap_first_two(Storage* s);
void* check_swap(void* arg);

#endif