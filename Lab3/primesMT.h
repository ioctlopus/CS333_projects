#pragma once

#ifndef _PRIMESMT_H
#define _PRIMESMT_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

typedef struct BitBlock_s {
    uint32_t bits;
    pthread_mutex_t mutex;
} BitBlock_t;

static inline BitBlock_t* alloc_arr(uint64_t upnum, uint16_t bitsperblock);
static inline void free_arr(BitBlock_t* ar, uint64_t size);
static inline void sieve(BitBlock_t* array, uint64_t size, long tid);
void* thread_func(void*);

#endif // _PRIMESMT_H
