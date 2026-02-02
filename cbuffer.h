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


typedef struct {
    size_t size;
    void* data;
} TaggedElement;

typedef struct
{
    void** buffer;      // Data storage
    size_t capacity;   // Max elements
    int head;       // Write position
    int tail;       // Read position
    size_t item_size; // Always sizeof(void*)
    pthread_mutex_t buf_mut;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} CBuffer;

CBuffer* newbuf(int size);

void add(CBuffer* buf, void* el);

void* get(CBuffer* buf);

void* pop(CBuffer* buf);

int del(CBuffer* buf, void* el);

int count(CBuffer* buf);

void setsize(CBuffer* buf, int n);

int append(CBuffer* buf, CBuffer* buf2);

void destroy(CBuffer* buf);

#endif	// CBUFFER_H
