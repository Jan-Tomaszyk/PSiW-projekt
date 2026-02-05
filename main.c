#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "cbuffer.h"

#define TEST_SIZE 1
#define TEST_THREADS 4
#define TEST_CAPACITY 100


//#define TEST_ITERATIONS 30

/*void* producer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i=0; i<TEST_SIZE; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        printf("[Producer %d] Pre-add: num=%p val=%d\n", i, num, *num);
        add(buf, num);
        printf("[Producer %d] Post-add: head=%d tail=%d\n", i, buf->head, buf->tail);

        // Verify the element was stored correctly
        pthread_mutex_lock(&buf->buf_mut);
        int idx = (buf->head - 1 + buf->capacity) % buf->capacity;
        if(buf->buffer[idx] != num) {
            fprintf(stderr, "Element mismatch at index %d\n", idx);
        }
        pthread_mutex_unlock(&buf->buf_mut);
    }
    return NULL;
}*/

void* producer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    void* added_items[TEST_SIZE] = {0};

    for(int i=0; i<TEST_SIZE; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        added_items[i] = num;

        printf("[Producer %d] Adding item %p (val: %d)\n", i, num, *num);
        add(buf, num);

       // Track without locking
        //printf("[Producer %d] Buffer state: head=%d tail=%d\n",
            i, buf->head, buf->tail);
    }

   // Verify all items were processed
   for(int i=0; i<TEST_SIZE; i++) {
        if(added_items[i] != NULL) {
            //printf("[Producer] WARNING: Item %p not consumed!\n", added_items[i]);
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


/*
void* producer(void* arg)
{
    printf("producer start");
        CBuffer* buf = (CBuffer*)arg;
    for(int i = 0; i < TEST_SIZE; i++)
        {
        printf("[Producer %lu] Attempting to add item %d\n", pthread_self(), i);
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
        printf("[Producer %lu] Added item %d\n", pthread_self(), i);
    }
        printf("producer end");
    return NULL;
}

void* consumer(void* arg)
{
    printf("consumer start");
        CBuffer* buf = (CBuffer*)arg;
    for(int i = 0; i < TEST_SIZE; i++)
        {
        void* num;
        printf("[Consumer %d] Attempting to remove item\n", i);
        del(buf, &num);
        printf("[Consumer ] Removed item %d\n", i);
        free(num);
    }
        printf("consumer end");
    return NULL;
}*/

void* ultraproducer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    void* added_items[TEST_CAPACITY+2] = {0};
        pthread_mutex_lock(&buf->buf_mut);
        printf("capdan-capbuff?%d", TEST_CAPACITY-buf->capacity);
        pthread_mutex_unlock(&buf->buf_mut);
    for(size_t i=0; i<TEST_CAPACITY; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        added_items[i] = num;

        printf("[UProducer] Adding item %d %p (val: %d)\n", i, num, *num);
        add(buf, num);

        // Track without locking
        //printf("[UProducer] Buffer state%d: head=%d tail=%d\n", i, buf->head, buf->tail);
    }
        printf("\nliczba:%d\n", count(buf));
        printf("capdan-countbuff?%d", TEST_CAPACITY-count(buf));
        pthread_mutex_lock(&buf->buf_mut);
        printf("countbuff-capbuff?%d", count(buf)-buf->capacity);
        pthread_mutex_unlock(&buf->buf_mut);
    // Verify all items were processed

        /*
         for(int i=0; i<TEST_CAPACITY; i++) {
        if(added_items[i] != NULL) {
            printf("[UProducer] WARNING: Item %p not consumed!\n", added_items[i]);
                }
        }
        */
    return NULL;
}

void* popget(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
        for(size_t i=0; i<TEST_CAPACITY; i++)
        {
                printf("Pop: %d\n", pop(buf));
                printf("Get: %d\n", get(buf));
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
        printf("[Mass Producer] Added %d (Capacity: %d)\n", i, buf->capacity);
        //usleep(100000); // 100ms delay between adds
    }
    return NULL;
}

void* mass_consumer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i=0; i<20; i++) {
        void* item;
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
    sleep(1); // Let initial ops start
    printf("\n[Resize Thread] Shrinking from %d to 3\n", buf->capacity);
    setsize(buf, 3);
    sleep(2);
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

// Temporary debug version of setsize
/*void test_resize() {
    // Test setup
    CBuffer* buf = newbuf(5);
    if(!buf) printf("buf niezaalokowano");
        pthread_t producer, consumer;

    // Test 1: Shrink with active elements
    for(int i=0; i<3; i++) {
        printf("wfor");
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
    }

    printf("=== Testing shrink from 5 to 2 ===\n");
    setsize(buf, 2); // Should block until count <= 2

    // Test 2: Expand while empty
    printf("\n=== Testing expand from 2 to 10 ===\n");
    setsize(buf, 10);

    // Test 3: Concurrent resize during operations
    pthread_create(&producer, NULL, (void*)mass_producer, buf);
    pthread_create(&consumer, NULL, (void*)mass_consumer, buf);

    sleep(1); // Let threads run
    printf("\n=== Dynamic resize during operations ===\n");
    setsize(buf, 3);
    setsize(buf, 7);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    destroy(buf);
}


/*void test_resize() {
    // Test setup
    CBuffer* buf = newbuf(5);
    pthread_t producer, consumer, counter[3], sizer[3];

    // Test 1: Shrink with active elements
    for(int i=0; i<3; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
    }
        printf("po for");
        pthread_create(&sizer[0], NULL, (int*)setsize, buf, 2);
        //pthread_create(&counter[0], NULL, (void*)count, buf);
        printf("Initial count: %d (should be 3)\n");
        //pthread_join(counter[0], NULL);
        pthread_join(sizer[0], NULL);
    printf("=== Testing shrink from 5 to 2 ===\n");
    pthread_join(sizer[0], NULL);
    //setsize(buf, 2); // Should block until count <= 2

    // Test 2: Expand while empty
    printf("\n=== Testing expand from 2 to 10 ===\n");
    setsize(buf, 10);

    //printf("count: %d (should be 2)\n", count(buf));
    // Test 3: Concurrent resize during operations
    pthread_create(&producer, NULL, (void*)mass_producer, buf);
    pthread_create(&consumer, NULL, (void*)mass_consumer, buf);

        sleep(1);
    printf("\n=== Dynamic resize during operations ===\n");
    setsize(buf, 3);

    setsize(buf, 7);
    //printf("dyn count: %d (should be ?)\n", count(buf));
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    //printf("dyn count: %d (should be ?)\n", count(buf));
    destroy(buf);
}*/


int main()
{
    CBuffer* buf = newbuf(TEST_CAPACITY);
        printf("second create test passed!\n");

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
           pthread_create(&inne[1], NULL, popget, buf) != 0) {
            perror("Thread creation failed");
            destroy(buf);
            exit(EXIT_FAILURE);
        }


        //pthread_create(&threads[0], NULL, ultraproducer, buf);
        //pthread_create(&threads[1], NULL, popget, buf);
        pthread_join(inne[0], NULL);
        printf("fulfill test passed!\n");
        pthread_join(inne[1], NULL);
        printf("fulfill while show test passed!\n");
        test_resize();
        printf("resize test passed!\n");


        printf("All tests passed!\n");
    return 0;
}
