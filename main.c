#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "cbuffer.h"

#define TEST_SIZE 1000
#define TEST_THREADS 4
#define TEST_ITERATIONS 1000

void* producer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i = 0; i < TEST_SIZE; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
        usleep(rand() % 100);
    }
    return NULL;
}

void* consumer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    for(int i = 0; i < TEST_SIZE; i++) {
        void* num;
        del(buf, &num);
        free(num);
        usleep(rand() % 100);
    }
    return NULL;
}

int main() {
    CBuffer* buf = newbuf(100);

    pthread_t prod_threads[5];
    pthread_t cons_threads[5];

    // Create producers
    for(int i = 0; i < 5; i++)
        pthread_create(&prod_threads[i], NULL, producer, buf);

    // Create consumers
    for(int i = 0; i < 5; i++)
        pthread_create(&cons_threads[i], NULL, consumer, buf);

    // Wait for completion
    for(int i = 0; i < 5; i++)
        pthread_join(prod_threads[i], NULL);

    for(int i = 0; i < 5; i++)
        pthread_join(cons_threads[i], NULL);

    // Verify empty state
    assert(count(buf) == 0);
    destroy(buf);

    buf = newbuf(100);

    pthread_t threads[TEST_THREADS*2];
    for(int i=0; i<TEST_THREADS; i++) {
        pthread_create(&threads[i], NULL, producer, buf);
        pthread_create(&threads[TEST_THREADS+i], NULL, consumer, buf);
    }

    for(int i=0; i<TEST_THREADS*2; i++) {
        pthread_join(threads[i], NULL);
    }

    assert(count(buf) == 0);
    destroy(buf);

    printf("All tests passed!\n");
    return 0;
}
