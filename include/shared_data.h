#ifndef SHARED_DATA_H
#define SHARED_DATA_H
#include <stddef.h>


typedef struct shared_data {
    char buffer[4096];
    size_t size;
    int stage;
    int finished;
} shared_data;

#endif // SHARED_DATA_H