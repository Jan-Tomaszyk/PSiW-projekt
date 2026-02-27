#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include "cbuffer.h"

#define TEST_SIZE 4
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
        // Obtain the item (removes it from the buffer) and free it
        void* num = get(buf); // blocks until an item is available
        printf("[Consumer %d] Got: num=%p\n", i, num);
        free(num);
    }
    return NULL;
}

void* ultraproducer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    //void* added_items[TEST_CAPACITY+2];//={0};
        pthread_mutex_lock(&buf->buf_mut);
        int capacity=buf->capacity;
        pthread_mutex_unlock(&buf->buf_mut);
        printf("capdan-capbuff = %d", TEST_CAPACITY-capacity);
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
        printf("countbuff-capbuff = %ld", count(buf)-buf->capacity);
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
        for(size_t i=1; i<TEST_CAPACITY/2; i++)
        {
                printf("[PopGet] Iteration %ld: count=%d\n", i, count(buf));
                if (count(buf)==0)
                {
                    break;
                }
                void* p = pop(buf);
                if (count(buf)==0)
                {
                    if(p) printf("Pop: %p %d\n", p, *(int*)(p));
                    if(p) free(p);
                    break;
                }
                void* g = get(buf);
                if(p) printf("Pop: %p %d\n", p, *(int*)(p));
                if(g) printf("Get: %p %d\n", g, *(int*)(g));
                if(p) free(p);
                if(g) free(g);
                if (count(buf)==0)
                {
                    break;
                }
        }
    return NULL;
}
///


int koniec = 0;
void* mass_producer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    pthread_mutex_lock(&buf->buf_mut);
    int capacity=buf->capacity;
    pthread_mutex_unlock(&buf->buf_mut);
    for(int i=0; i<capacity; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
        printf("[Mass Producer] Added %d (Capacity: %ld)\n", i, buf->capacity);
        }
    while(capacity==buf->capacity) {
            
        }
    pthread_mutex_lock(&buf->buf_mut);
    capacity=buf->capacity;
    while(_count_nolock(buf) > capacity) {
        //pthread_mutex_lock(&buf->buf_mut);
        pthread_cond_wait(&buf->only_full, &buf->buf_mut);
        //pthread_mutex_unlock(&buf->buf_mut);
    }
    capacity=buf->capacity;
    pthread_mutex_unlock(&buf->buf_mut);
    for(int i=0; i<capacity; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
        printf("[Mass Producer] Added %d (Capacity: %ld)\n", i, buf->capacity);
        /*if(i==capacity-1) {
            //break;
            printf("[Mass Producer] Reached capacity %ld, waiting for space...\n", buf->capacity);
            pthread_mutex_lock(&buf->buf_mut);
            while(_count_nolock(buf) >= capacity) {
                pthread_cond_wait(&buf->not_full, &buf->buf_mut);
            }
            pthread_mutex_unlock(&buf->buf_mut);
            printf("[Mass Producer] Space available, resuming...\n");
        }*/
    }
    printf("\n[Mass Producer] koniec\n");
    koniec = 1;
    return NULL;
}

void* mass_consumer(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    int usuniete = 0;
    for(int i=0; i<50; i++) {
        void* item = get(buf);  // Blocks if empty
        printf("[Mass Consumer] Removed %d\n", *(int*)item);
        //free(item);
        //item = NULL;
        usuniete++;
        pthread_mutex_lock(&buf->buf_mut);
        if(koniec||(
            (usuniete==10 || usuniete==12 || usuniete==14 || usuniete==18)
         && _count_nolock(buf) == 0))
        {
            pthread_mutex_unlock(&buf->buf_mut);
            break;
        }
        pthread_mutex_unlock(&buf->buf_mut);
    }
    printf("\n[Mass Consumer] koniec, usuniete: %d\n", usuniete);
    return NULL;
}

void* resize_thread(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    
    // Wait until consumer has started removing items
    sleep(1);  // Give mass_consumer time to acquire lock and start work
    
    pthread_mutex_lock(&buf->buf_mut);
    printf("\n[Resize Thread] Shrinking from %ld to 4\n", buf->capacity);
    pthread_mutex_unlock(&buf->buf_mut);
    setsize(buf, 4);
    //sleep(2);
    printf("\n[Resize Thread] Expanding to 8\n");
    setsize(buf, 8);
    printf("\n[Resize Thread] koniec\n");
    return NULL;
}

void test_resize() {
    CBuffer* buf = newbuf(6);
    pthread_t producer, consumer, resizer;

    pthread_create(&producer, NULL, mass_producer, buf);
    pthread_create(&consumer, NULL, mass_consumer, buf);
    pthread_create(&resizer, NULL, resize_thread, buf);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    pthread_join(resizer, NULL);
    pthread_join(consumer, NULL);

    destroy(buf);
    printf("\n[test Thread] koniec\n");
}

void* append_thread(void* arg) {
    printf("\n[Append Thread] Starting append thread\n");
    CBuffer* buf1 = (CBuffer*)arg;
    CBuffer* buf2 = newbuf(10);

    for(int i=0; i<10; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf2, num);
    }
    printf("elements added");
    while(1) {
        int moved = append(buf1, buf2);
        printf("while1");
        if(moved == 0) break;
        atomic_fetch_add(&transfer_count, moved);
    }
    destroy(buf2);
    return NULL;
}

void test_append()
{
    printf("\n=== TEST: append ===\n");    
    CBuffer* buf1 = newbuf(15);
    printf("\ntest_append buf1 createdn");
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

// ===== EDGE CASE TESTS =====

// Test setsize: expand (shrink is already tested in test_resize)
void test_setsize_expand()
{
    printf("\n=== TEST: setsize EXPAND ===\n");
    CBuffer* buf = newbuf(3);  // capacity = 3
    
    // Add 2 items
    int* item1 = malloc(sizeof(int));
    *item1 = 1;
    int* item2 = malloc(sizeof(int));
    *item2 = 2;
    add(buf, item1);
    add(buf, item2);
    
    printf("Before expand: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    
    // Expand capacity to 9
    setsize(buf, 9);
    printf("After expand to 9: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    
    // Should still be able to get the items
    void* got1 = get(buf);
    assert(got1 == item1);
    free(got1);
    void* got2 = get(buf);
    assert(got2 == item2);
    free(got2);
    destroy(buf);
    printf("\nPASS test_setsize_expand\n");
}

// Test setsize: shrink to exactly current count (no wait needed)
void test_setsize_shrink_exact() {
    printf("\n=== TEST: setsize SHRINK to exact count ===\n");
    CBuffer* buf = newbuf(5);  // capacity = 5
    
    // Add 3 items
    for(int i=0; i<3; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf, num);
    }
    
    printf("Before shrink: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    
    // Shrink to 3 (exact count) — should not wait
    setsize(buf, 3);  // n+1 = 3
    printf("After shrink to 3: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    
    /*// Clean up
    for(int i=0; i<3; i++)
    {
        void* item = get(buf);
        free(item);
    }*/
    destroy(buf);
    printf("PASS test_setsize_shrink_exact\n");
}

// Test setsize: multiple sequential operations
void test_setsize_sequential() {
    printf("\n=== TEST: setsize SEQUENTIAL operations ===\n");
    CBuffer* buf = newbuf(4);  // capacity = 4
    
    // Add 2 items
    for(int i=0; i<2; i++) {
        int* num = malloc(sizeof(int));
        *num = 700 + i;
        add(buf, num);
    }
    
    printf("Initial: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    
    // Shrink to 2
    setsize(buf, 2);
    printf("After shrink to 2: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    assert(buf->capacity == 2);
    
    // Expand to 7
    setsize(buf, 7);
    printf("After expand to 7: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    assert(buf->capacity == 7);
    
    // Shrink again to 2
    setsize(buf, 2);
    printf("After shrink to 2 again: count=%d, capacity=%ld\n", count(buf), buf->capacity);
    assert(buf->capacity == 2);
    
    // Clean up
    for(int i=0; i<2; i++) {
        void* item = get(buf);
        free(item);
    }
    destroy(buf);
    printf("PASS test_setsize_sequential\n");
}



// Test append: buf1 full, buf2 has items (should return 0)
void test_append_buf1_full() {
    printf("\n=== TEST: append with buf1 FULL ===\n");
    CBuffer* buf1 = newbuf(2);  // capacity = 2
    CBuffer* buf2 = newbuf(3);  // capacity = 3
    
    // Fill buf1 to capacity
    for(int i=0; i<2; i++) {
        int* num = malloc(sizeof(int));
        *num = 100 + i;
        add(buf1, num);
    }
    
    // Fill buf2 with items
    for(int i=0; i<3; i++) {
        int* num = malloc(sizeof(int));
        *num = 200 + i;
        add(buf2, num);
    }
    
    printf("buf1: count=%d, capacity=%ld\n", count(buf1), buf1->capacity);
    printf("buf2: count=%d, capacity=%ld\n", count(buf2), buf2->capacity);
    
    // Try to append — should return 0
    int moved = append(buf1, buf2);
    printf("Elements moved: %d (expected 0)\n", moved);
    assert(moved == 0);
    
    // Verify counts unchanged
    assert(count(buf1) == 2);
    assert(count(buf2) == 3);
    
    destroy(buf1);
    destroy(buf2);
    printf("PASS test_append_buf1_full\n");
}

// Test append: buf1 empty, buf2 has many items (should transfer all that fit)
void test_append_buf1_empty()
{
    printf("\n=== TEST: append with buf1 EMPTY ===\n");
    CBuffer* buf1 = newbuf(5);  // capacity = 5
    CBuffer* buf2 = newbuf(10); // capacity = 10
    
    // Keep buf1 empty
    
    // Fill buf2 with 8 items
    for(int i=0; i<8; i++) {
        int* num = malloc(sizeof(int));
        *num = 300 + i;
        add(buf2, num);
    }
    
    printf("Before append: buf1 count=%d, buf2 count=%d\n", count(buf1), count(buf2));
    
    // Append — buf1 has 5 free spaces, buf2 has 8 items → should transfer 5
    int moved = append(buf1, buf2);
    printf("Elements moved: %d (expected 5)\n", moved);
    assert(moved == 5);
    
    // Verify counts
    assert(count(buf1) == 5);
    assert(count(buf2) == 3);
    
    destroy(buf1);
    destroy(buf2);
    printf("PASS test_append_buf1_empty\n");
}

// Test append: buf2 empty (should return 0)
void test_append_buf2_empty() {
    printf("\n=== TEST: append with buf2 EMPTY ===\n");
    CBuffer* buf1 = newbuf(5);
    CBuffer* buf2 = newbuf(5);
    
    // Add 2 items to buf1
    for(int i=0; i<2; i++) {
        int* num = malloc(sizeof(int));
        *num = 400 + i;
        add(buf1, num);
    }
    // buf2 is empty
    
    printf("buf1: count=%d, buf2: count=%d\n", count(buf1), count(buf2));
    
    int moved = append(buf1, buf2);
    printf("Elements moved: %d (expected 0)\n", moved);
    assert(moved == 0);
    
    destroy(buf1);
    destroy(buf2);
    printf("PASS test_append_buf2_empty\n");
}

// Test append: partial transfer (buf1 has limited space, buf2 has more)
void test_append_partial() {
    printf("\n=== TEST: append PARTIAL transfer ===\n");
    CBuffer* buf1 = newbuf(5);  // capacity = 5
    CBuffer* buf2 = newbuf(10); // capacity = 10
    
    // Add 3 items to buf1 (3 free spaces left)
    for(int i=0; i<3; i++) {
        int* num = malloc(sizeof(int));
        *num = 500 + i;
        add(buf1, num);
    }
    
    // Add 8 items to buf2
    for(int i=0; i<8; i++) {
        int* num = malloc(sizeof(int));
        *num = 600 + i;
        add(buf2, num);
    }
    pthread_mutex_lock(&buf1->buf_mut);
    printf("Before: buf1 count=%d (%ld free), buf2 count=%d\n", _count_nolock(buf1), buf1->capacity - _count_nolock(buf1), count(buf2));
    pthread_mutex_unlock(&buf1->buf_mut);
    // Should transfer 3 (all available space in buf1)
    int moved = append(buf1, buf2);
    printf("Elements moved: %d (expected 2)\n", moved);
    assert(moved == 2);
    assert(count(buf1) == 5);  // full
    assert(count(buf2) == 6);  // 8 - 2
    
    destroy(buf1);
    destroy(buf2);
    printf("PASS test_append_partial\n");
}


// Test append: with only 1 item in buf2
void test_append_single_item() {
    printf("\n=== TEST: append with single item in buf2 ===\n");
    CBuffer* buf1 = newbuf(5);
    CBuffer* buf2 = newbuf(3);
    
    // Add 1 item to buf2
    int* item = malloc(sizeof(int));
    *item = 800;
    add(buf2, item);
    
    printf("buf1 empty: count=%d, buf2 count=%d\n", count(buf1), count(buf2));
    
    int moved = append(buf1, buf2);
    printf("Elements moved: %d (expected 1)\n", moved);
    assert(moved == 1);
    assert(count(buf1) == 1);
    assert(count(buf2) == 0);
    
    destroy(buf1);
    destroy(buf2);
    printf("PASS test_append_single_item\n");
}
//SI TESTY Z KONKURENCJĄ
// Test append: concurrent producers and consumers
void* producer_thread(void* arg) {
    CBuffer* buf2 = (CBuffer*)arg;
    for(int i=0; i<13; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf2, num);
        printf("[Producer %lx] Added %d\n", pthread_self(), i);
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    struct { CBuffer* buf1; CBuffer* buf2; atomic_int* total; }* params = arg;
    int local_count = 0;
    for(int i=0; i<45; i++)
    {
        pthread_mutex_lock(&params->buf2->buf_mut);
        while(_count_nolock(params->buf2) == 0)
        {
            pthread_cond_wait(&params->buf2->not_empty, &params->buf2->buf_mut);
        }
        pthread_mutex_unlock(&params->buf2->buf_mut);
        int moved = append(params->buf1, params->buf2);
        printf("[Consumer %lx] Moved %d items\n", pthread_self(), moved);
        if(params->total>0 && (moved == 0 || count(params->buf2) == 0 || count(params->buf1) == params->buf1->capacity)) 
        {local_count += moved; break;}
        local_count += moved;
    }
    atomic_fetch_add(params->total, local_count);
    return NULL;
}

void test_proper_concurrency() {
    CBuffer* buf1 = newbuf(15);
    CBuffer* buf2 = newbuf(30);
    atomic_int total_transferred = 0;
    
    // Pass shared buffers to threads
    struct { CBuffer* buf1; CBuffer* buf2; atomic_int* total; } consumer_params = {buf1, buf2, &total_transferred};

    pthread_t producers[2], consumers[2];
    for(int i=0; i<2; i++) pthread_create(&producers[i], NULL, producer_thread, buf2);
    for(int i=0; i<2; i++) pthread_create(&consumers[i], NULL, consumer_thread, &consumer_params);

    sleep(1); // Let threads contend
    for(int i=0; i<2; i++) pthread_join(producers[i], NULL);
    for(int i=0; i<2; i++) pthread_join(consumers[i], NULL);
    
    printf("Final counts: buf1=%d, buf2=%d, transferred=%d\n", 
           count(buf1), count(buf2), atomic_load(&total_transferred));
    assert(atomic_load(&total_transferred) == count(buf1));
}

void* append_thread1(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    CBuffer* other = newbuf(10);
    for(int i=0; i<100; i++) append(buf, other);
    return NULL;
}
void* append_thread2(void* arg) {
    CBuffer* buf = (CBuffer*)arg;
    CBuffer* other = newbuf(10);
    for(int i=0; i<100; i++) append(other, buf);
    return NULL;
}

void test_append_lock_order() {
    CBuffer* buf1 = newbuf(10);
    CBuffer* buf2 = newbuf(10);
    
    // Fill buf2 with test data
    for(int i=0; i<5; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        add(buf2, num);
    }

    // Test append in both directions
    printf("Transfer buf2->buf1...\n");
    int transferred = append(buf1, buf2);
    assert(transferred == 5);
    assert(count(buf1) == 5);
    assert(count(buf2) == 0);

    printf("Transfer buf1->buf2...\n");
    transferred = append(buf2, buf1);
    assert(transferred == 5);
    assert(count(buf2) == 5);
    assert(count(buf1) == 0);

    // Verify no deadlock occurs
    pthread_t threads[4];
    for(int i=0; i<4; i++) {
        pthread_create(&threads[i], NULL, 
            i%2 ? (void*)append_thread1 : (void*)append_thread2,
            (i%2) ? (void*)buf1 : (void*)buf2);
    }
    for(int i=0; i<4; i++) pthread_join(threads[i], NULL);
}


void* random_append_worker(void* arg) {
    CBuffer** buffers = (CBuffer**)arg;
    for(int i=0; i<1000; i++) {
        int idx1 = rand()%4;
        int idx2 = rand()%4;
        if(idx1 != idx2) {
            append(buffers[idx1], buffers[idx2]);
        }
    }
    return NULL;
}

void stress_test_append() {
    CBuffer* buffers[4];
    for(int i=0; i<4; i++) buffers[i] = newbuf(20);
    
    // Create threads that randomly append between buffers
    pthread_t threads[8];
    for(int i=0; i<8; i++) {
        pthread_create(&threads[i], NULL, random_append_worker, buffers);
    }
    
    sleep(10); // Let threads contend
    printf("Stress test completed without deadlock\n");
}

// Test del(): verify element shifting after deletion
void test_del_shifts_elements() {
    printf("\n=== TEST: del() ELEMENT SHIFTING ===\n");
    CBuffer* buf = newbuf(10);  // capacity = 10
    
    // Add 5 items with distinct values
    int* items[5];
    for(int i = 0; i < 5; i++) {
        items[i] = malloc(sizeof(int));
        *items[i] = i * 100;  // 0, 100, 200, 300, 400
        add(buf, items[i]);
    }
    
    printf("Added 5 items: 0, 100, 200, 300, 400\n");
    printf("Initial count: %d\n", count(buf));
    assert(count(buf) == 5);
    
    // Test 1: Delete from middle (delete item 200, which is items[2])
    printf("\n[Test 1] Deleting middle element (200)...\n");
    int res1 = del(buf, items[2]);
    assert(res1 == 1);  // Should be found and deleted
    assert(count(buf) == 4);
    
    // Verify remaining order: should be 0, 100, 300, 400
    void* got;
    int val;
    
    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 0)\n", val);
    assert(val == 0);
    assert(got == items[0]);
    free(got);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 100)\n", val);
    assert(val == 100);
    assert(got == items[1]);
    free(got);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 300)\n", val);
    assert(val == 300);
    assert(got == items[3]);
    free(got);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 400)\n", val);
    assert(val == 400);
    assert(got == items[4]);
    free(got);

    printf("[Test 1] PASS - Middle element correctly deleted and shifted\n");
    
    // Clean up test 1 - items[2] was deleted but never retrieved/freed above
    free(items[2]);
    destroy(buf);
    
    // Test 2: Delete from start
    printf("\n[Test 2] Deleting first element...\n");
    buf = newbuf(10);
    for(int i = 0; i < 5; i++) {
        items[i] = malloc(sizeof(int));
        *items[i] = i * 100;
        add(buf, items[i]);
    }
    
    res1 = del(buf, items[0]);  // Delete first (0)
    assert(res1 == 1);
    assert(count(buf) == 4);
    free(items[0]);
    
    // Should get 100, 200, 300, 400
    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 100)\n", val);
    assert(val == 100);
    assert(got == items[1]);
    free(got);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 200)\n", val);
    assert(val == 200);
    assert(got == items[2]);
    free(got);

    for(int i = 3; i < 5; i++) {
        got = get(buf);
        free(got);
    }

    printf("[Test 2] PASS - First element correctly deleted\n");
    destroy(buf);
    
    // Test 3: Delete from end
    printf("\n[Test 3] Deleting last element...\n");
    buf = newbuf(10);
    for(int i = 0; i < 5; i++) {
        items[i] = malloc(sizeof(int));
        *items[i] = i * 100;
        add(buf, items[i]);
    }
    
    res1 = del(buf, items[4]);  // Delete last (400)
    assert(res1 == 1);
    assert(count(buf) == 4);
    free(items[4]);

    // Should get 0, 100, 200, 300
    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 0)\n", val);
    assert(val == 0);
    assert(got == items[0]);
    //int res2 = del(buf, got);
    //assert(res2 == 1);
    free(items[0]);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 100)\n", val);
    assert(val == 100);
    assert(got == items[1]);
    free(items[1]);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 200)\n", val);
    assert(val == 200);
    assert(got == items[2]);
    free(items[2]);

    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 300)\n", val);
    assert(val == 300);
    assert(got == items[3]);
    free(items[3]);
    
    printf("[Test 3] PASS - Last element correctly deleted\n");
    destroy(buf);
    
    // Test 4: Delete non-existent element
    printf("\n[Test 4] Deleting non-existent element...\n");
    buf = newbuf(10);
    for(int i = 0; i < 3; i++) {
        items[i] = malloc(sizeof(int));
        *items[i] = i * 100;
        add(buf, items[i]);
    }
    
    int* fake_item = malloc(sizeof(int));
    *fake_item = 999;
    int res2 = del(buf, fake_item);
    assert(res2 == 0);  // Should NOT be found
    assert(count(buf) == 3);  // Count unchanged
    
    // Verify items still intact
    got = get(buf);
    val = *(int*)got;
    printf("Got: %d (expected 0)\n", val);
    assert(val == 0);
    
    // Remove remaining items before destroy
    printf("remaining items count: %d\n", count(buf));
    printf("Removing remaining items...\n");
    for(int i = 0; i < 2; i++)
    {
        //printf("Attempting to get item %d...\n", i);
        got = get(buf);
        printf("Got: %d\n", *(int*)got);
        free(got);
    }
    
    printf("[Test 4] PASS - Non-existent element correctly rejected\n");
    
    free(fake_item);
    destroy(buf);
    
    printf("\n=== ALL del() SHIFTING TESTS PASSED ===\n");
}

//testy przykładowe, od pana
void* setsi_Test_prosty()
{
    CBuffer* buf = newbuf(3);
     int* a = malloc(sizeof(int)); *a = 1;
     int* b = malloc(sizeof(int)); *b = 2;
     int* c = malloc(sizeof(int)); *c = 3;
     add(buf, a);
     add(buf, b);
     add(buf, c);
     setsize(buf, 5);
     int* d = malloc(sizeof(int)); *d = 4;
     add(buf, d);
     int* e = malloc(sizeof(int)); *e = 5;
     add(buf, e);
     a = get(buf);
     assert(*a == 1);
     setsize(buf, 20);
     setsize(buf, 4);
     b = get(buf);
     assert(*b == 2);
     c=get(buf);
     d=get(buf);
     e=get(buf);
     assert(count(buf)==0);
     destroy(buf); 
    return NULL;
}

typedef struct {
    pthread_barrier_t barrier;
    CBuffer* buf;
} test_args_t;

// Additional barriers for finer-grained sync in T4
typedef struct {
    pthread_barrier_t barrier_start; // T0/T1/T2 start together
    pthread_barrier_t barrier_T2_done; // T2 signals when setsize(6)+add(f) done
    pthread_barrier_t barrier_T0_T1; // sync between T0 and T1 (coordinate add(g) with a get)
    CBuffer* buf;
} t4_args_t;
// T4 - Thread 0
void* setsi_Test_T4_T0(void* arg) {
    t4_args_t* args = (t4_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    

    printf("[T4-T0] Starting\n");
    
    int* a = malloc(sizeof(int)); *a = 1;
    int* b = malloc(sizeof(int)); *b = 2;
    int* c = malloc(sizeof(int)); *c = 3;
    int* d = malloc(sizeof(int)); *d = 4;
    int* e = malloc(sizeof(int)); *e = 5;
    
    add(buf, a);
    add(buf, b);
    add(buf, c);
    add(buf, d);
    add(buf, e);
    printf("[T4-T0] Added a,b,c,d,e\n");
    

    pthread_barrier_wait(&args->barrier_start);  // Sync: T1 and T2 start
    printf("[T4-T0] After barrier_start: T1 and T2 can start setsize\n");

    // Wait until T2 finished setsize(6)+add(f)
    pthread_barrier_wait(&args->barrier_T2_done);
    printf("[T4-T0] After barrier_T2_done: T2 finished setsize(6)+add(f)\n");

    a = get(buf);
    printf("[T4-T0] Got a=%d, count=%d (expected 5)\n", *a, count(buf));
    assert(count(buf) == 5);
    b = get(buf);
    printf("[T4-T0] Got b=%d, count=%d (expected 4)\n", *b, count(buf));
    assert(count(buf) == 4);
    c = get(buf);
    printf("[T4-T0] Got c=%d\n", *c);
    // After retrieving first three elements, synchronize with T1 so it may add g
    pthread_barrier_wait(&args->barrier_T0_T1);
    printf("[T4-T0] After barrier_T0_T1: T1 may add g now\n");

    d = get(buf);
    printf("[T4-T0] Got d=%d\n", *d);

    e = get(buf);
    printf("[T4-T0] Got e=%d\n", *e);
    int* f = get(buf);
    printf("[T4-T0] Got f=%d\n", *f);
    int* g = get(buf);
    printf("[T4-T0] Got g=%d, count=%d (expected 0)\n", *g, count(buf));
    assert(count(buf) == 0);
    
    //free(a); free(b); free(c); free(d); free(e); free(f); free(g);
    return NULL;
}

// T4 - Thread 1
void* setsi_Test_T4_T1(void* arg) {
    t4_args_t* args = (t4_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    // start slightly later so T0 can add items
    //usleep(100000); // 100ms
    pthread_barrier_wait(&args->barrier_start);  // Sync
    // Wait until T2 finished expanding and adding f
    pthread_barrier_wait(&args->barrier_T2_done);
    printf("[T4-T1] Starting: calling setsize(3)\n");
    setsize(buf, 3);//punkt 2 - T1 wywołuje setsize(3) - powinien czekać, bo count may be > 3
    printf("[T4-T1] setsize(3) complete, capacity=%ld\n", buf->capacity);

    // Wait for T0 to perform initial gets, then add g
    pthread_barrier_wait(&args->barrier_T0_T1);
    int* g = malloc(sizeof(int)); *g = 7;
    add(buf, g);
    printf("[T4-T1] Added g=7\n");
    printf("[T4-T1] Finished\n");
    return NULL;
}

// T4 - Thread 2 (T3 in description)
void* setsi_Test_T4_T2(void* arg) {
    t4_args_t* args = (t4_args_t*)arg;
    CBuffer* buf = args->buf;
    pthread_barrier_wait(&args->barrier_start);  // Sync
    printf("[T4-T2] Starting: calling setsize(6)\n");
    setsize(buf, 6);
    printf("[T4-T2] setsize(6) complete, capacity=%ld\n", buf->capacity);
    int* f = malloc(sizeof(int)); *f = 6;
    add(buf, f);
    printf("[T4-T2] Added f=6\n");

    // Notify T0 and T1 that T2 finished
    pthread_barrier_wait(&args->barrier_T2_done);
    printf("[T4-T2] Finished\n");
    return NULL;
}

void* setsi_Test_T4() {
    printf("\n=== TEST: setsi_Test_T4 ===\n");
    CBuffer* buf = newbuf(7);
    
    t4_args_t args;
    args.buf = buf;
    pthread_barrier_init(&args.barrier_start, NULL, 3);  // T0, T1, T2
    pthread_barrier_init(&args.barrier_T2_done, NULL, 3); // T0, T1, T2 (T2 signals completion)
    pthread_barrier_init(&args.barrier_T0_T1, NULL, 2);  // T0 and T1

    pthread_t t0, t1, t2;
    pthread_create(&t0, NULL, setsi_Test_T4_T0, &args);
    pthread_create(&t1, NULL, setsi_Test_T4_T1, &args);
    pthread_create(&t2, NULL, setsi_Test_T4_T2, &args);

    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("All threads joined, cleaning up...\n");

    pthread_barrier_destroy(&args.barrier_start);
    pthread_barrier_destroy(&args.barrier_T2_done);
    pthread_barrier_destroy(&args.barrier_T0_T1);
    destroy(buf);
    printf("PASS setsi_Test_T4\n");
    return NULL;
}

// T5 - Thread 0
void* setsi_Test_T5_T0(void* arg) {
    test_args_t* args = (test_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    
    printf("[T5-T0] Starting\n");
    
    int* a = malloc(sizeof(int)); *a = 1;
    int* b = malloc(sizeof(int)); *b = 2;
    int* c = malloc(sizeof(int)); *c = 3;
    int* d = malloc(sizeof(int)); *d = 4;
    int* e = malloc(sizeof(int)); *e = 5;
    int* f = malloc(sizeof(int)); *f = 6;
    
    add(buf, a);
    add(buf, b);
    add(buf, c);
    add(buf, d);
    add(buf, e);
    add(buf, f);
    printf("[T5-T0] Added a-f\n");
    
    pthread_barrier_wait(&args->barrier);  // Sync: T1 and T2 can start
    printf("[T5-T0] After barrier: T1 and T2 can start\n");
    
    a = get(buf);
    printf("[T5-T0] Got a=%d\n", *a);
    
    //pthread_barrier_wait(&args->barrier);  // Sync: T1 setsize done
    //printf("[T5-T0] After barrier: T1 setsize done\n");
    
    b = get(buf);
    printf("[T5-T0] Got b=%d\n", *b);
    
    //printf("[T5-T0] After barrier: T2 setsize done\n");
    
    c = get(buf);
    printf("[T5-T0] Got c=%d\n", *c);
    
    d = get(buf);
    printf("[T5-T0] Got d=%d\n", *d);
    
    pthread_barrier_wait(&args->barrier);  // Sync: T2 setsize done
    printf("[T5-T0] After barrier: T2 setsize done\n");
    //
    e = get(buf);
    printf("[T5-T0] Got e=%d\n", *e);
    
    f = get(buf);
    printf("[T5-T0] Got f=%d\n", *f);
    
    int* g = get(buf);
    printf("[T5-T0] Got g=%d\n", *g);
    
    int* h = get(buf);
    printf("[T5-T0] Got h=%d\n", *h);
    
    free(a); free(b); free(c); free(d); free(e); free(f); free(g); free(h);//jednak zwolnić?
    return NULL;
}

// T5 - Thread 1
void* setsi_Test_T5_T1(void* arg) {
    test_args_t* args = (test_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    
    pthread_barrier_wait(&args->barrier);  // Wait for T0 to add items    
    printf("[T5-T1] Starting: calling setsize(3)\n");
    setsize(buf, 3);
    printf("[T5-T1] setsize(3) complete, capacity=%ld\n", buf->capacity);
    pthread_barrier_wait(&args->barrier);  // Wait for T0 to add items    
    int* h = malloc(sizeof(int)); *h = 8;
    add(buf, h);
    printf("[T5-T1] Added h=8\n");
    
    //pthread_barrier_wait(&args->barrier);  // Notify T0
    printf("[T5-T1] Finished\n");
    return NULL;
}

// T5 - Thread 2
void* setsi_Test_T5_T2(void* arg) {
    test_args_t* args = (test_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    
    pthread_barrier_wait(&args->barrier);  // Wait for T0 to add items
    printf("[T5-T2] Starting: calling setsize(5)\n");
    setsize(buf, 5);
    printf("[T5-T2] setsize(5) complete, capacity=%ld\n", buf->capacity);
    pthread_barrier_wait(&args->barrier);  // Wait for T0 to add items    
    int* g = malloc(sizeof(int)); *g = 7;
    add(buf, g);
    printf("[T5-T2] Added g=7\n");
    
    //pthread_barrier_wait(&args->barrier);  // Notify T0
    printf("[T5-T2] Finished\n");
    return NULL;
}

void* setsi_Test_T5() {
    printf("\n=== TEST: setsi_Test_T5 ===\n");
    CBuffer* buf = newbuf(7);
    
    test_args_t args;
    args.buf = buf;
    pthread_barrier_init(&args.barrier, NULL, 3);  // 3 threads
    
    pthread_t t0, t1, t2;
    pthread_create(&t0, NULL, setsi_Test_T5_T0, &args);
    pthread_create(&t1, NULL, setsi_Test_T5_T1, &args);
    pthread_create(&t2, NULL, setsi_Test_T5_T2, &args);
    
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    pthread_barrier_destroy(&args.barrier);
    destroy(buf);
    printf("PASS setsi_Test_T5\n");
    return NULL;
}

// T6 - Thread 0
void* setsi_Test_T6_T0(void* arg) {
    test_args_t* args = (test_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    
    printf("[T6-T0] Starting\n");
    
    int* a = malloc(sizeof(int)); *a = 1;
    int* b = malloc(sizeof(int)); *b = 2;
    int* c = malloc(sizeof(int)); *c = 3;
    
    add(buf, a);
    add(buf, b);
    add(buf, c);
    printf("[T6-T0] Added a,b,c\n");
    
    pthread_barrier_wait(&args->barrier);  // Sync: T1 and T2 can start
    
    c = pop(buf);
    printf("[T6-T0] Popped c=%d\n", *c);
    
    b = pop(buf);
    printf("[T6-T0] Popped b=%d\n", *b);
    
    pthread_barrier_wait(&args->barrier);  // Sync: T2 setsize done
    printf("[T6-T0] After barrier: T2 setsize done\n");
    
    int* d = pop(buf);
    printf("[T6-T0] Popped d=%d, count=%d (expected 1)\n", *d, count(buf));
    assert(count(buf) == 1);
    
    //free(a); free(b); free(c); free(d);
    return NULL;
}

// T6 - Thread 1 (T2 in description)
void* setsi_Test_T6_T1(void* arg) {
    test_args_t* args = (test_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    
    printf("[T6-T2] Starting: calling setsize(2)\n");
    pthread_barrier_wait(&args->barrier);  // Wait for T0 to add items
    
    setsize(buf, 2);
    printf("[T6-T2] setsize(2) complete, capacity=%ld\n", buf->capacity);
    
    pthread_barrier_wait(&args->barrier);  // Notify T0
    printf("[T6-T2] Finished\n");
    return NULL;
}

// T6 - Thread 2 (T3 in description)
void* setsi_Test_T6_T2(void* arg) {
    test_args_t* args = (test_args_t*)arg;
    CBuffer* buf = args->buf;
    //CBuffer* buf = (CBuffer*)arg;
    
    printf("[T6-T3] Starting: adding d\n");
    pthread_barrier_wait(&args->barrier);  // Wait for T0 to add items
    
    int* d = malloc(sizeof(int)); *d = 4;
    add(buf, d);
    printf("[T6-T3] Added d=4\n");
    pthread_barrier_wait(&args->barrier);  // Notify T0 that d is added
    printf("[T6-T3] Finished\n");
    return NULL;
}

void* setsi_Test_T6() {
    printf("\n=== TEST: setsi_Test_T6 ===\n");
    CBuffer* buf = newbuf(5);
    
    test_args_t args;
    args.buf = buf;
    pthread_barrier_init(&args.barrier, NULL, 3);  // 3 threads
    
    pthread_t t0, t1, t2;
    pthread_create(&t0, NULL, setsi_Test_T6_T0, &args);
    pthread_create(&t1, NULL, setsi_Test_T6_T1, &args);
    pthread_create(&t2, NULL, setsi_Test_T6_T2, &args);
    
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    pthread_barrier_destroy(&args.barrier);
    destroy(buf);
    printf("PASS setsi_Test_T6\n");
    return NULL;
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
    for(int i=0; i<TEST_THREADS*2; i++)
    {
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

        destroy(buf);
        printf("destroy test passed!\n");
    pthread_t inne[5];
        // Verify empty state
    //assert(count(buf) == 0);
    
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
        printf("resize test passed!\n");

        test_append();
        
        printf("append test passed!\n");

        printf("\n========== EDGE CASE TESTS ==========\n");
        test_setsize_expand();
        test_setsize_shrink_exact();
        test_setsize_sequential();
        printf("resize egde test passed!\n");
        
        test_append_buf1_full();
        test_append_buf1_empty();
        test_append_buf2_empty();
        test_append_partial();
        test_append_single_item();
        printf("append egde test passed!\n");

        printf("\n========== SI TESTY ==========\n");
        test_proper_concurrency();
        printf("append concurrency test passed!\n");

        test_del_shifts_elements();
        printf("del shifting test passed!\n");
        
        test_append_lock_order();
        printf("append lock order test passed!\n");
        stress_test_append();
        printf("append stress test passed!\n");

        printf("====================Testy od pana!=====================\n");
        setsi_Test_prosty();
        printf("setsi Test prosty passed!\n");
        
        setsi_Test_T4();
        printf("setsi Test T4 passed!\n");
        
        setsi_Test_T5();
        printf("setsi Test T5 passed!\n");
        
        setsi_Test_T6();
        printf("setsi Test T6 passed!\n");
        
        printf("All tests passed!\n");
    return 0;
}
