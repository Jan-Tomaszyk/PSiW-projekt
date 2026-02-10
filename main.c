#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include "cbuffer.h"

#define TEST_SIZE 1
#define TEST_THREADS 4
#define TEST_CAPACITY 100

atomic_int transfer_count;

void* producer(void* arg)
{
    CBuffer* buf = (CBuffer*)arg;
    void* added_items[TEST_SIZE] = {0};

    for(int i=0; i<TEST_SIZE; i++)
        {
        int* num = malloc(sizeof(int));
        *num = i;
        added_items[i] = num;

        printf("[Producer %d] Adding item %p (val: %d)\n", i, num, *num);
        add(buf, num);

       // Track without locking
       //printf("[Producer %d] Buffer state: head=%d tail=%d\n", i, buf->head, buf->tail);
    }

   // Verify all items were processed
   for(int i=0; i<TEST_SIZE; i++)
{
        if(added_items[i] != NULL)
        {
         printf("[Producer] WARNING: Item %p not consumed!\n", added_items[i]);
        }
    }
    return NULL;
}

void* consumer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i=0; i<TEST_SIZE; i++) {
        void* num = (void*)0xDEADBEEF; // Sentinel value
        printf("[Consumer %d] Pre-del: num=%p\n", i, num);
        int res = del(buf, &num);
        printf("[Consumer %d] Post-del: res=%d num=%p\n", i, res, num);
        if(res == 1) {
            assert(num != (void*)0xDEADBEEF); // Verify pointer changed
            free(num);
        }
    }
    return NULL;
}

void* ultraproducer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    //void* added_items[TEST_CAPACITY+2];//={0};
        pthread_mutex_lock(&buf->buf_mut);
        printf("capdan-capbuff = %ld", TEST_CAPACITY-buf->capacity);
        pthread_mutex_unlock(&buf->buf_mut);
    for(size_t i=0; i<TEST_CAPACITY; i++)
        {
        int* num = malloc(sizeof(int));
        *num = i;
       // added_items[i] = num;

        printf("[UProducer] Adding item %ld %p (val: %d)\n", i, num, *num);
        add(buf, num);

        // Track without locking
        //printf("[UProducer] Buffer state%d: head=%d tail=%d\n", i, buf->head, buf->tail);
    }
        printf("\nliczba:%d\n", count(buf));
        printf("capdan-countbuff = %d", TEST_CAPACITY-count(buf));
        pthread_mutex_lock(&buf->buf_mut);
        printf("countbuff-capbuff = %ld", count(buf)-buf->capacity);
        pthread_mutex_unlock(&buf->buf_mut);
    //verify all items were processed
   /*
        for (int i=0; i<TEST_CAPACITY; i++) {
        if(added_items[i] != NULL) {
            printf("[UProducer] WARNING: Item %p not consumed!\n", added_items[i]);
                }
        }
        */
    return NULL;
}

void* popget(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
        for(size_t i=0; i<TEST_CAPACITY/2; i++)
        {
                printf("Pop: %p %d\n", pop(buf), *(int*)(pop(buf)));
                printf("Get: %p %d\n", get(buf), *(int*)(get(buf)));
        }
    return NULL;
}
///



void* mass_producer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i=0; i<20; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
        printf("[Mass Producer] Added %d (Capacity: %ld)\n", i, buf->capacity);
        }
    return NULL;
}

void* mass_consumer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i=0; i<20; i++) {
        void* item=pop(buf);
        if(del(buf, &item)) {
            printf("[Mass Consumer] Removed %d\n", *(int*)item);
            free(item);
        }
        //usleep(150000); // 150ms delay between removes
    }
    return NULL;
}

void* resize_thread(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    //sleep(1);// Let initial ops start
    pthread_mutex_lock(&buf->buf_mut);
    printf("\n[Resize Thread] Shrinking from %ld to 3\n", buf->capacity);
    pthread_mutex_unlock(&buf->buf_mut);
    setsize(buf, 3);
    //sleep(2);
    printf("\n[Resize Thread] Expanding to 8\n");
    setsize(buf, 8);
    return NULL;
}

void test_resize() {
    CBuffer* buf = newbuf(5);
    pthread_t producer, consumer, resizer;

    pthread_create(&producer, NULL, mass_producer, buf);
    pthread_create(&consumer, NULL, mass_consumer, buf);
    pthread_create(&resizer, NULL, resize_thread, buf);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    pthread_join(resizer, NULL);

    destroy(buf);
}

void* append_thread(void* arg) {
    CBuffer* buf1 = (CBuffer*)arg;
    CBuffer* buf2 = newbuf(10);

    for(int i=0; i<10; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf2, num);
    }

    while(1) {
        int moved = append(buf1, buf2);
        printf("while1");
        if(moved == 0) break;
        atomic_fetch_add(&transfer_count, moved);
    }
    destroy(buf2);
    return NULL;
}

void test_append() {
    CBuffer* buf1 = newbuf(15);
    transfer_count = 0;

    pthread_t threads[3];
    for(int i=0; i<3; i++) {
        pthread_create(&threads[i], NULL, append_thread, buf1);
    }


    sleep(1); // Allow transfers to occur
    printf("Total elements transferred: %d\n",
           atomic_load(&transfer_count));

    for(int i=0; i<3; i++) pthread_join(threads[i], NULL);
    destroy(buf1);
}


int main()
{
    CBuffer* buf = newbuf(TEST_CAPACITY);
        printf("create test passed!\n");

    pthread_t threads[TEST_THREADS*2];
    for(int i=0; i<TEST_THREADS; i++) {
        pthread_create(&threads[i], NULL, producer, buf);
        pthread_create(&threads[TEST_THREADS+i], NULL, consumer, buf);
    }
        printf("Create both tests passed!\n");
    for(int i=0; i<TEST_THREADS*2; i++) {
        pthread_join(threads[i], NULL);
    }
        printf("Wait for completion of both tests passed!\n");
    //assert(count(buf) == 0);
    destroy(buf);
        printf("destroy test passed!\n");

        buf = newbuf(TEST_CAPACITY);

    pthread_t prod_threads[5];
    pthread_t cons_threads[5];

    // Create producers
    for(int i = 0; i < 5; i++)
        {
        pthread_create(&prod_threads[i], NULL, producer, buf);
        }
        printf("Create producers tests passed!\n");
    // Create consumers
    for(int i = 0; i < 5; i++)
        {
        pthread_create(&cons_threads[i], NULL, consumer, buf);
        }
        printf("Create consumers tests passed!\n");
    // Wait for completion
    for(int i = 0; i < 5; i++)
        {
        pthread_join(prod_threads[i], NULL);
        }
        printf("Wait for prodcompletion tests passed!\n");
    for(int i = 0; i < 5; i++)
        {
        pthread_join(cons_threads[i], NULL);
        }
        printf("Wait for conscompletion tests passed!\n");


    pthread_t inne[5];
        // Verify empty state
    //assert(count(buf) == 0);
    destroy(buf);
        printf("destroy test passed!\n");
        buf = newbuf(TEST_CAPACITY);
        if(!buf) {
        fprintf(stderr, "Buffer allocation failed\n");
        exit(EXIT_FAILURE);
        }
        if(pthread_create(&inne[0], NULL, ultraproducer, buf) != 0 ||
           pthread_create(&inne[1], NULL, popget, buf) != 0
                || pthread_create(&inne[2], NULL, popget, buf) != 0) {
            perror("Thread creation failed");
            destroy(buf);
            exit(EXIT_FAILURE);
        }


        //pthread_create(&threads[0], NULL, ultraproducer, buf);
        //pthread_create(&threads[1], NULL, popget, buf);
        pthread_join(inne[1], NULL);
        pthread_join(inne[0], NULL);
        pthread_join(inne[2], NULL);
        printf("fulfill while show test passed!\n");
        destroy(buf);
        test_resize();
        buf = newbuf(5);
        //setsize(buf, 7;)
        //setsize(buf, 3);
        //printf("resize test passed!\n");

        test_append();

        printf("All tests passed!\n");
    return 0;
}
