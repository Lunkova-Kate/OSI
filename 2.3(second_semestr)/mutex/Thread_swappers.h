#ifndef THREAD_SWAPPERS_H
#define THREAD_SWAPPERS_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Node.h"

bool swap_first_two(Storage* s);
void* check_swap(void* arg);

#endif