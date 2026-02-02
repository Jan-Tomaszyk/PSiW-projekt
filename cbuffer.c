#include "cbuffer.h"

#ifndef MAX_ITEM_SIZE
#define MAX_ITEM_SIZE (1 << 20)  // 1MB per item
#endif

CBuffer* newbuf(int size)
{
    CBuffer* cb = malloc(sizeof(CBuffer));
    if (!cb) return NULL;
    cb->buffer = malloc(size * sizeof(void*));
    if (!cb->buffer)
    {
        free(cb);
        return NULL;
    }
    cb->capacity = size;
    cb->head = cb->tail = 0;
    cb->item_size = sizeof(void*);
    //size_t item_size=0;
    cb->head = cb->tail = 0;
    return cb;
}

void add(CBuffer* buf, void* el)
{
    pthread_mutex_lock(&buf->buf_mut);
    while ((buf->head + 1) % buf->capacity == buf->tail) 
    {
        //pthread_mutex_unlock(&buf->buf_mut);
        pthread_cond_wait(&buf->not_full, &buf->buf_mut);
    }
    buf->buffer[buf->head] = el; // Stores the pointer directly
    buf->head = (buf->head + 1) % buf->capacity;
    pthread_mutex_unlock(&buf->buf_mut);
    pthread_cond_signal(&buf->not_empty);
}

void* get(CBuffer* buf)
{
    while ((buf->head) == buf->tail) 
    {
        pthread_cond_wait(&buf->not_empty, &buf->buf_mut);
    }
    return buf->buffer[buf->tail];
}

void* pop(CBuffer* buf)
{
    while ((buf->head) == buf->tail) 
    {
        pthread_cond_wait(&buf->not_empty, &buf->buf_mut);
    }
    return buf->buffer[buf->head];
}

int del(CBuffer* buf, void* el)
{
    pthread_mutex_lock(&buf->buf_mut);
    
    if (buf->head == buf->tail) {
        pthread_mutex_unlock(&buf->buf_mut);
        return 0;
    }
    int current = buf->tail;
    while (current != buf->head)
    {
        if (buf->buffer[current] == el)
        {
            int next = (current + 1) % buf->capacity;
            while (next != buf->head)
            {
                buf->buffer[current] = buf->buffer[next];
                current = next;
                next = (next + 1) % buf->capacity;
            }
            buf->head = (buf->head - 1) % buf->capacity;
            pthread_mutex_unlock(&buf->buf_mut);
            pthread_cond_signal(&buf->not_full);
            return 1; // Success
        }
        current = (current + 1) % buf->capacity;
    }
    pthread_mutex_unlock(&buf->buf_mut);
    return 0;
}

/*
int count(CBuffer* buf)
{
    pthread_mutex_lock(&buf->buf_mut);
    
    if (buf->head == buf->tail)
    {
        pthread_mutex_unlock(&buf->buf_mut);
        return 0;
    }
    int current = buf->tail;
    int licz=0;
    while (current != buf->head)
    {
        licz++;
        current = (current + 1) % buf->capacity;
    }
    pthread_mutex_unlock(&buf->buf_mut);
    return licz;
}
*/
int count(CBuffer* buf) {
    pthread_mutex_lock(&buf->buf_mut);
    int count = (buf->head - buf->tail + buf->capacity) % buf->capacity;
    pthread_mutex_unlock(&buf->buf_mut);
    return count;
}

//pthread_mutex_lock(&buf->buf_mut);
//    while ((buf->head + 1) % buf->capacity == buf->tail) 
//    {
//        pthread_cond_wait(&buf->not_full, &buf->buf_mut);
//    }
    

void setsize(CBuffer* buf, int n)
{
    pthread_mutex_lock(&buf->buf_mut);
    
    int old_capacity = buf->capacity;
    buf->capacity = n;

    while(count(buf) > n)
    {
        pthread_cond_wait(&buf->not_full, &buf->buf_mut);
    }
    
    pthread_mutex_unlock(&buf->buf_mut);
    
    if(n > old_capacity) {
        pthread_cond_broadcast(&buf->not_full);
    }
    /*if(buf->capacity>n && count(buf)>n)
    {
        buf->capacity=count(buf);
    }
    while(buf->capacity>n)
    {
        pthread_cond_wait(&buf->not_full, &buf->buf_mut);
    }
    
    buf->capacity=n;
    pthread_mutex_unlock(&buf->buf_mut);
    if(count(buf)<buf->capacity)
    {
        pthread_cond_broadcast(&buf->not_full);
    }*/
}

int append(CBuffer* buf, CBuffer* buf2)
{
    pthread_mutex_lock(&buf->buf_mut);
    pthread_mutex_lock(&buf2->buf_mut);
    int transferred = 0;
    int available = buf->capacity - count(buf);
    int to_transfer = min(available, count(buf2));

    while(transferred < to_transfer)
    {
        void* el = buf2->buffer[buf2->tail];
        buf->buffer[buf->head] = el;
        buf->head = (buf->head + 1) % buf->capacity;
        buf2->tail = (buf2->tail + 1) % buf2->capacity;
        transferred++;
    }
    pthread_mutex_unlock(&buf2->buf_mut);
    pthread_mutex_unlock(&buf->buf_mut);
    
    if(transferred > 0)
    {
        pthread_cond_signal(&buf->not_empty);
        pthread_cond_signal(&buf2->not_full);
    }

    return transferred;
}

void destroy(CBuffer* buf)
{
    if(!buf) return;

    pthread_mutex_lock(&buf->buf_mut);
    pthread_cond_broadcast(&buf->not_full);
    pthread_cond_broadcast(&buf->not_empty);
    pthread_mutex_unlock(&buf->buf_mut);

    pthread_cond_destroy(&buf->not_full);
    pthread_cond_destroy(&buf->not_empty);
    pthread_mutex_destroy(&buf->buf_mut);

    free(buf->buffer); // Safe even if NULL
    free(buf);        
}
