#include "cbuffer.h"
#include <math.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>

#ifndef MAX_ITEM_SIZE
#define MAX_ITEM_SIZE (1 << 20)  // 1MB per item
#endif

CBuffer* newbuf(int size)
{
        //printf("\ntworze newbuff %d\n", size);
    CBuffer* cb = malloc(sizeof(CBuffer));
    if (!cb) return NULL;
    cb->buffer = malloc((size+1) * sizeof(void*));
    if (!cb->buffer)
    {
        //free(cb);
        destroy(cb);
        return NULL;
    }
    cb->capacity = size+1;
    cb->head = cb->tail = 0;
    cb->item_size = sizeof(void*);
    //size_t item_size=0;
    cb->head = cb->tail = 0;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&cb->buf_mut, &attr);
        //n("\nstworzono newbuff %d\n", size);
    return cb;
}

void add(CBuffer* buf, void* el)
{
        //
    pthread_mutex_lock(&buf->buf_mut);
    //printf("\ndodaje - lock\n");
        while ((buf->head + 1) % buf->capacity == buf->tail)
    {
        pthread_mutex_unlock(&buf->buf_mut);//?
        pthread_cond_wait(&buf->not_full, &buf->buf_mut);
        pthread_mutex_lock(&buf->buf_mut);
    }
        //printf("\ndodaje - koniec czek\n");
    buf->buffer[buf->head] = el; // Stores the pointer directly
    buf->head = (buf->head + 1) % buf->capacity;
        pthread_mutex_unlock(&buf->buf_mut);
        //printf("\ndodaje - unlock\n");
        pthread_cond_signal(&buf->not_empty);
        //printf("dodano el");
}

void* get(CBuffer* buf)
{
        //printf("\ngetting\n");
    while ((buf->head) == buf->tail)
    {
        pthread_cond_wait(&buf->not_empty, &buf->buf_mut);
    }
        //printf("\ngot\n");
    return buf->buffer[buf->tail];
}

void* pop(CBuffer* buf)
{
        //printf("\npoping\n");
    while ((buf->head) == buf->tail)
    {
        //pthread_cond_wait(&buf->not_empty, &buf->buf_mut);
    }
        printf("\npoped\n");
    return buf->buffer[(buf->head-1)%buf->capacity];
}


int del(CBuffer* buf, void* el)
{
    pthread_mutex_lock(&buf->buf_mut);

    // Empty buffer case
    if(buf->head == buf->tail) {
        printf("brak elementu");
        pthread_mutex_unlock(&buf->buf_mut);
        return 0;
    }

    // Search for element
    int found = 0;
    int current = buf->tail;

    while(current != buf->head) {
        if(buf->buffer[current] == el) {
            found = 1;
            break;
        }
        current = (current + 1) % buf->capacity;
    }

    // Shift elements if found
    if(found) {
        int next = (current + 1) % buf->capacity;
        while(next != buf->head) {
            buf->buffer[current] = buf->buffer[next];
            current = next;
            next = (next + 1) % buf->capacity;
        }
        buf->head = (buf->head - 1 + buf->capacity) % buf->capacity;
    }

    pthread_mutex_unlock(&buf->buf_mut);

    // Signal if we created space
    if(found)
        {
                pthread_cond_signal(&buf->only_full);
                pthread_cond_signal(&buf->not_full);
        }
        else
        {
                printf("brak elementu");
        }

    return found;
}


int count(CBuffer* buf)
{
         //printf("\ncounting\n");
    pthread_mutex_lock(&buf->buf_mut);
    int count = (buf->head - buf->tail + buf->capacity) % buf->capacity;
    pthread_mutex_unlock(&buf->buf_mut);
         //printf("\ncounted\n");
    return count;
}

//pthread_mutex_lock(&buf->buf_mut);
//    while ((buf->head + 1) % buf->capacity == buf->tail)
//    {
//        pthread_cond_wait(&buf->not_full, &buf->buf_mut);
//    }


// Temporary debug version of setsize
void setsize(CBuffer* buf, int n)
{
        printf("[DEBUG] Entering setsize(%d)\n", n);

        //static __thread int lock_depth = 0;
        //if(lock_depth++==0)
        //{

        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        int d=1;
        timeout.tv_sec += d; // 1 second timeout

        if(pthread_mutex_timedlock(&buf->buf_mut, &timeout) != 0)
        {
                printf("Failed to acquire mutex after %d seconds", d);
                return;
        }
        //lock_owned = 1;

        //}
            printf("[DEBUG] Acquired mutex\n");

        int old_capacity = buf->capacity;
    buf->capacity = n;
        //pthread_mutex_unlock(&buf->buf_mut);
    //printf("[DEBUG] Released mutex\n");
//      pthread_mutex_lock(&buf->buf_mut);
    //printf("[DEBUG] Acquired mutex\n");
        if (n< old_capacity)
        {
                while(count(buf) > n)
                {
                        printf("[DEBUG] Waiting (count=%d, new_cap=%d)\n", count(buf), n);
                        pthread_cond_broadcast(&buf->not_empty);
                        pthread_cond_wait(&buf->only_full, &buf->buf_mut);
                        //pthread_cond_wait(&buf->not_full, &buf->buf_mut);
                }
        }
        if (count(buf)<buf->capacity)
        {
                pthread_cond_broadcast(&buf->not_full);
        }
        //if(--lock_depth==0)
        //{
                pthread_mutex_unlock(&buf->buf_mut);
        //lock_owned = 0;
        //}
        //pthread_mutex_unlock(&buf->buf_mut);
    printf("[DEBUG] Released mutex\n");
}

int append(CBuffer* buf, CBuffer* buf2)
{
        //printf("\nappending\n");
    pthread_mutex_lock(&buf->buf_mut);
    pthread_mutex_lock(&buf2->buf_mut);
    int transferred = 0;
    int available = buf->capacity - count(buf);
    int to_transfer = MIN(available, count(buf2));

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
        //printf("\nappended\n");
    return transferred;
}

void destroy(CBuffer* buf)
{
        printf("\ndestroying\n");
    if(!buf) {printf("nothing to free");return;}
        printf("locking ");
    pthread_mutex_lock(&buf->buf_mut);
    pthread_cond_broadcast(&buf->not_full);
    pthread_cond_broadcast(&buf->not_empty);
    pthread_mutex_unlock(&buf->buf_mut);
        printf("unlocked ");
    pthread_cond_destroy(&buf->not_full);
    pthread_cond_destroy(&buf->not_empty);
    pthread_mutex_destroy(&buf->buf_mut);


        if(buf->buffer)
        {
                for(int i = buf->tail; i != buf->head; i = (i+1) % buf->capacity)
                {
                        free(buf->buffer[i]);
                }
                printf("freeing buffer");
                free(buf->buffer); // Safe even if NULL
                printf("buffer freed");
        }
        printf("freeing buf");
    free(buf);
        printf("buf freed");
                printf("\ndestroyed\n");
}

