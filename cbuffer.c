#include "cbuffer.h"
#include <math.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>

#ifndef MAX_ITEM_SIZE
#define MAX_ITEM_SIZE (1 << 20)  // 1MB per item
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

CBuffer* newbuf(int size)
{
        //printf("\ntworze newbuff %d\n", size);
    CBuffer* cb = malloc(sizeof(CBuffer));
    if (!cb)
    {   
        return NULL;
    }
    cb->buffer = malloc(size * sizeof(void*));
    if (!cb->buffer)
    {
        free(cb);
        return NULL;
    }
    cb->capacity = size;  // Actual buffer size (unchanged during setsize reorganization)
    cb->limit = size;     // Effective capacity (changed immediately in setsize)
    cb->pending_min_limit = INT_MAX; // No pending setsize calls initially
    cb->head = cb->tail = 0;
    //cb->item_size = sizeof(void*);
    cb->count = 0;
    //size_t item_size=0;
    cb->head = cb->tail = 0;
    cb->setsizes_in_progress = 0;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&cb->buf_mut, &attr);
        pthread_cond_init(&cb->not_full, NULL);
        pthread_cond_init(&cb->not_empty, NULL);
        pthread_cond_init(&cb->only_full, NULL);
        //printf("\nstworzono newbuff %d\n", size);
    return cb;
}

void add(CBuffer* buf, void* el)
{
        //
    pthread_mutex_lock(&buf->buf_mut);
    ////printf("\ndodaje - lock\n");
        // Check against the minimum of (limit, pending_min_limit) to respect ongoing setsize calls
        size_t effective_limit = MIN(buf->limit, buf->pending_min_limit);
        while (buf->count >= effective_limit)
        {
            pthread_cond_wait(&buf->not_full, &buf->buf_mut);
        }
        ////printf("\ndodaje - koniec czek\n");
    printf("[DEBUG] add(): head=%d, capacity=%ld\n", buf->head, buf->capacity);
    buf->buffer[buf->head] = el; // Stores the pointer directly
    buf->head = (buf->head + 1) % buf->capacity;
    buf->count++;
    printf("[DEBUG] add() done: new_head=%d, count=%ld\n", buf->head, buf->count);
        pthread_cond_signal(&buf->not_empty);
        pthread_mutex_unlock(&buf->buf_mut);
        ////printf("\ndodaje - unlock\n");
        ////printf("dodano el");
}

void* get(CBuffer* buf)
{
    pthread_mutex_lock(&buf->buf_mut);
        ////printf("\ngetting\n");
    while (buf->count==0) //(buf->head) == buf->tail)
    {
        pthread_cond_wait(&buf->not_empty, &buf->buf_mut);
    }
    printf("[DEBUG] get(): tail=%d, capacity=%ld\n", 
           buf->tail, buf->capacity);
    void* el = buf->buffer[buf->tail];
    /* Clear the slot to avoid leaving stale pointers in the buffer
       which could later be double-freed by destroy(). */
    buf->buffer[buf->tail] = NULL;
    buf->tail = (buf->tail + 1) % buf->capacity;
    buf->count--;
    printf("[DEBUG] get() done: new_tail=%d, count=%ld\n", buf->tail, buf->count);
    pthread_cond_signal(&buf->only_full);
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->buf_mut);
        ////printf("\ngot\n");
    return el;
}

void* pop(CBuffer* buf)
{
    pthread_mutex_lock(&buf->buf_mut);
        ////printf("\npoping\n");
    while (buf->count==0) //(buf->head) == buf->tail)
    {
        pthread_cond_wait(&buf->not_empty, &buf->buf_mut);
    }
    printf("[DEBUG] pop(): tail=%d, capacity=%ld\n", buf->tail, buf->capacity);
    int idx = (buf->head - 1 + buf->capacity) % buf->capacity;
    void* el = buf->buffer[idx];
    buf->buffer[idx] = NULL;
    buf->head = idx;
    buf->count--;
    printf("[DEBUG] pop() done: new_head=%d, count=%ld\n", buf->head, buf->count);
    pthread_cond_signal(&buf->only_full);
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->buf_mut);
        ////printf("\npoped\n");
    return el;
}


int del(CBuffer* buf, void* el)
{
    pthread_mutex_lock(&buf->buf_mut);

    // Empty buffer case
    if(buf->count==0) //(buf->head) == buf->tail)
    {
        //printf("brak elementu");
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
        buf->buffer[buf->head] = NULL; // Clear last slot
    //czy powyższe jest w dobrej kolejności? czy najpierw zmniejszać head, a potem czyścić slot?
        buf->count--;
    }

    
    // Signal if we created space
    if(found)
        {
                //printf("\nusunięto element\n");
                pthread_cond_signal(&buf->only_full);
                pthread_cond_signal(&buf->not_full);
        }
        else
        {
                //printf("brak elementu");
        }
        pthread_mutex_unlock(&buf->buf_mut);

    return found;
}

int _count_nolock(CBuffer* buf)
{
    return buf->count;//(buf->head - buf->tail + buf->capacity+1) % (buf->capacity+1);
}

int count(CBuffer* buf)
{
    ////printf("\ncounting\n");
    pthread_mutex_lock(&buf->buf_mut);
        ////printf("\ncount założono mutex\n");
    int count = _count_nolock(buf);//(buf->head - buf->tail + buf->capacity) % buf->capacity;
        ////printf("\ncounted\n");
        pthread_mutex_unlock(&buf->buf_mut);
        ////printf("\ncount zdjęto mutex\n");
    return count;
}


void setsize(CBuffer* buf, int n)
{
        printf("[DEBUG] Entering setsize(%d)\n", n);

        pthread_mutex_lock(&buf->buf_mut);
        
        buf->setsizes_in_progress++;
        // CRITICAL: Capture old values at entry
        int old_capacity = buf->capacity;
        int old_tail = buf->tail;  // CRITICAL: Must capture tail here!
        int old_limit = buf->limit;
        int old_pending_min_limit = buf->pending_min_limit;
        
        printf("[DEBUG] Captured: old_capacity=%d, old_tail=%d, old_limit=%d, old_pending_min_limit=%d, new_limit=%d, count=%ld, head=%d\n", 
               old_capacity, old_tail, old_limit, old_pending_min_limit, n, buf->count, buf->head);

        // Step 1: Update pending_min_limit to track the minimum among all ongoing setsize calls
        //buf->limit = n;  // Update limit immediately to unblock add() if expanding
        if (n < buf->pending_min_limit)
        {
            buf->pending_min_limit = n;
            printf("[DEBUG] Updated pending_min_limit to %d\n", n);
        }

        // Step 2: Wait for buffer to have count <= new_limit (if shrinking)
        if (n < old_limit || n < old_capacity)
        {
            printf("[DEBUG] Shrinking: waiting for count <= %d\n", n);
            while(buf->count > n)
            {
                printf("[DEBUG] Waiting (count=%ld, target=%d)\n", buf->count, n);
                pthread_cond_broadcast(&buf->not_empty);
                pthread_cond_wait(&buf->only_full, &buf->buf_mut);
            }
            printf("[DEBUG] Count is now <= %d\n", n);
        }

        // Step 3: Check if reallocation is needed
        if (n == old_capacity)
        {
            printf("[DEBUG] New size equals old capacity, no reallocation needed\n");
            buf->limit = n;
            printf("[DEBUG] Committed: limit=%d\n", n);
            pthread_mutex_unlock(&buf->buf_mut);
            return;
        }

        // Step 4: Allocate new buffer
        void** new_buffer = malloc(n * sizeof(void*));
        if (!new_buffer) {
            // ROLLBACK: Restore pending_min_limit if malloc fails
            printf("[DEBUG] malloc() failed - rolling back\n");
            buf->pending_min_limit = old_pending_min_limit;
            pthread_cond_broadcast(&buf->not_full);
            pthread_mutex_unlock(&buf->buf_mut);
            return;  
        }
        printf("[DEBUG] Allocated new buffer for size %d\n", n);

        // Step 5: Copy items using captured old_capacity and old_tail (safe - they're local copies!)
        for(int i = 0; i < buf->count; i++) {
            int old_idx = (buf->tail + i) % old_capacity;
            void* item = buf->buffer[old_idx];
            new_buffer[i] = item;
            printf("[DEBUG] Copy item[%d]: old_idx=%d\n", i, old_idx);
        }
        printf("[DEBUG] Copied %ld items from old_buffer (capacity=%d, tail=%d) to new_buffer (capacity=%d)\n", buf->count, old_capacity, buf->tail, n);

        // Step 6: Commit (buf_mut held throughout, so serialized)
        printf("[DEBUG] Before commit: freeing old_buffer\n");
        free(buf->buffer);
        buf->buffer = new_buffer;
        buf->capacity = n;  
        buf->limit = n;     
        buf->tail = 0;
        buf->head = buf->count % buf->capacity;  // Wrap head to valid range
        printf("[DEBUG] Committed: old_buffer freed, capacity=%ld, limit=%ld, tail=0, head=%d, count=%ld\n", 
               buf->capacity, buf->limit, buf->head, buf->count);

        // Step 7: Signal waiters
        pthread_cond_broadcast(&buf->only_full);
        if(buf->count < buf->capacity)
        {
                pthread_cond_broadcast(&buf->not_full);
        }
        
        buf->setsizes_in_progress--;
        if (buf->setsizes_in_progress == 0)
        {
            buf->pending_min_limit = INT_MAX; // Reset when no setsize in progress
            printf("[DEBUG] All setsize operations complete, reset pending_min_limit\n");
        }
        else
        {
            buf->pending_min_limit = MIN(buf->pending_min_limit, n); // Update to the next minimum if there are still setsize operations
            printf("[DEBUG] setsize operations still in progress: %d\n", buf->setsizes_in_progress);
        }
        pthread_mutex_unlock(&buf->buf_mut);
        printf("[DEBUG] Resize complete\n");
}

int append(CBuffer* buf, CBuffer* buf2)
{
        printf("\nappending\n");
    pthread_mutex_lock(&buf->buf_mut);
    pthread_mutex_lock(&buf2->buf_mut);
    int transferred = 0;
    int available = buf->capacity - _count_nolock(buf);
    int to_transfer = MIN(available, _count_nolock(buf2));
    printf("[DEBUG] Available space in buf1: %d, items in buf2: %d, will transfer: %d\n",available, _count_nolock(buf2), to_transfer);
    while(transferred < to_transfer)
    {
        void* el = buf2->buffer[buf2->tail];
        buf->buffer[buf->head] = el;
        buf->head = (buf->head + 1) % buf->capacity;
        buf2->tail = (buf2->tail + 1) % buf2->capacity;
        transferred++;
        buf->count++;
        buf2->count--;
    }
    pthread_mutex_unlock(&buf2->buf_mut);
    pthread_mutex_unlock(&buf->buf_mut);

    if(transferred > 0)
    {
        pthread_cond_signal(&buf->not_empty);
        pthread_cond_signal(&buf->only_full);
        pthread_cond_signal(&buf2->only_full);
        pthread_cond_signal(&buf2->not_full);
    }
        printf("\nappended\n");
    return transferred;
}

void destroy(CBuffer* buf)
{
    printf("\nDestroying buffer\n");
    if(!buf)
    {
        return;
    }
    pthread_mutex_lock(&buf->buf_mut);
    pthread_cond_signal(&buf->only_full);
    pthread_cond_broadcast(&buf->not_full);
    pthread_cond_broadcast(&buf->not_empty);
    pthread_mutex_unlock(&buf->buf_mut);
    
    pthread_cond_destroy(&buf->not_full);
    pthread_cond_destroy(&buf->not_empty);
    pthread_cond_destroy(&buf->only_full);

    pthread_mutex_destroy(&buf->buf_mut);

        // NOTE: Do NOT free buffer items here.
        // Items are pointers to user-allocated memory.
        // get/pop return pointers without removing them,
        // so destroying the buffer would cause double-free.
        // Caller is responsible for freeing items via del().
        if(buf->buffer)
        {
                for(int i = buf->tail; i != ((buf->tail + buf->count) % buf->capacity); i = (i+1) % buf->capacity)
                {
                    printf("trying to free element at index %d\n", i);
                    if (buf->buffer[i] != NULL)
                    {
                        printf("freeing %d element\n", i);
                        free(buf->buffer[i]);
                        printf("freed %d element\n", i);
                    }
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

