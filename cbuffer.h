#ifndef CBUFFER_H
#define CBUFFER_H

// ==============================================
//
//  Version 1.0, 2026-01-08
//
// ==============================================


#include <pthread.h>
#include <stdlib.h>
//#include <stdio.h>



typedef struct
{
    void** buffer;      // Data storage
    size_t capacity;   // Actual buffer size (for modulo operations, unchanged during setsize)
    size_t limit;      // Effective capacity for add() blocking (changed immediately in setsize)
    size_t pending_min_limit;  // Minimum limit among all pending setsize calls
    int setsizes_in_progress; // Flag to indicate how many a setsize operation is in progress
    int head;       // Write position
    int tail;       // Read position
    //size_t item_size; // Always sizeof(void*)
    size_t count;     // Current item count
    pthread_mutex_t buf_mut;
    pthread_cond_t not_full;
    pthread_cond_t only_full;
    pthread_cond_t not_empty;
} CBuffer;

CBuffer* newbuf(int size);

void add(CBuffer* buf, void* el);

void* get(CBuffer* buf);

void* pop(CBuffer* buf);

int del(CBuffer* buf, void* el);

int count(CBuffer* buf);

int _count_nolock(CBuffer* buf);

void setsize(CBuffer* buf, int n);

int append(CBuffer* buf, CBuffer* buf2);

void destroy(CBuffer* buf);

#endif  // CBUFFER_H
