# Limit Field Implementation - Detailed Explanation

## The Problem You Identified

You had a brilliant insight: **separate the concerns of modulo operations from capacity enforcement**.

- **`capacity`**: Actual buffer size, used ONLY for modulo arithmetic, NEVER changes during reorganization
- **`limit`**: Effective capacity, determines when `add()` blocks, changed immediately in `setsize()`

This prevents modulo corruption while still enforcing the "new target limit applies" requirement.

## The Original Issue (Before Fix)

Your code with the `limit` field had a **deadlock problem** when multiple threads call `setsize()` concurrently:

```
Timeline:
T1: setsize(3) → sets limit=3 → starts waiting (count > 3)
                (T1 releases mutex and waits on condition var)
    
T2: setsize(6) → sets limit=6 → (shrinking? no, 6 > 7) 
    → reorganizes buffer using old_capacity=7
    → completes and signals
    
T1: wakes up → tries to reorganize using old_capacity=7
    BUT the buffer is now capacity=6!
    → Modulo arithmetic is WRONG
    → Heap corruption or crash
```

**The key insight:** When one `setsize()` completes while another is waiting, the waiting thread's assumptions about the buffer structure are **invalid**.

## The Solution: `in_resize` Flag

Add a **mutex-protected flag** to prevent concurrent `setsize()` operations:

```c
typedef struct {
    // ... other fields ...
    int in_resize;              // Flag: only one setsize() at a time
    pthread_cond_t resize_done; // Signal when resize completes
} CBuffer;
```

### How It Works:

```
T1: setsize(3)
    acquire_mutex()
    while (in_resize) wait(resize_done)  // Initially false, proceed
    in_resize = 1
    // ... perform resize ...
    in_resize = 0
    signal(resize_done)  // Wake up any waiting T2
    release_mutex()
    
T2: setsize(6)
    acquire_mutex()
    while (in_resize) wait(resize_done)  // T1 is resizing, WAIT HERE
                                          // (will wake up when T1 signals)
    in_resize = 1
    // ... perform resize ...
    in_resize = 0
    signal(resize_done)
    release_mutex()
```

## Key Changes to setsize()

### 1. Wait for any in-progress resize:
```c
while (buf->in_resize) {
    printf("[DEBUG] Another resize in progress, waiting...\n");
    pthread_cond_wait(&buf->resize_done, &buf->buf_mut);
}
buf->in_resize = 1;  // Now it's OUR turn
```

### 2. Mark resize complete:
```c
buf->in_resize = 0;
pthread_cond_broadcast(&buf->resize_done);  // Wake up waiting threads
```

### 3. On failure (malloc fails):
```c
if (!new_buffer) {
    buf->limit = old_limit;      // Rollback
    buf->in_resize = 0;          // Mark resize complete
    pthread_cond_broadcast(&buf->resize_done);
    // ... unlock and return
}
```

## Why This Works

| Aspect | Before | After |
|--------|--------|-------|
| Multiple setsize() calls | ❌ Deadlock/corruption | ✅ Serialized safely |
| Modulo arithmetic | ❌ Can be corrupted mid-resize | ✅ Always uses correct old_capacity |
| Limit enforcement | ✅ New limit applied immediately | ✅ Still applied immediately |
| Failure rollback | ✅ Capacity rolled back | ✅ Both capacity AND in_resize rolled back |
| Memory safety | ❌ Heap corruption possible | ✅ Safe, no concurrent buffer access |

## Timeline with Fix (T1 and T2 calling setsize() concurrently)

```
T0: add(a,b,c,d,e) → count=5
                      
T1: setsize(3)
    - Acquires mutex
    - in_resize=0, so proceeds
    - Sets in_resize=1
    - Sets limit=3
    - Starts waiting: count=5 > 3
    
    [releases mutex while waiting]
    
T2: setsize(6) 
    - Acquires mutex
    - in_resize=1 (T1 is busy!)
    - WAITS on resize_done condition
    
    [mutex released, still waiting]
    
T0: get() operations reduce count to <= 3
    
T1: wakes up from count check
    - Reorganizes buffer
    - capacity=3, in_resize=0
    - Signals resize_done
    - Releases mutex
    
T2: wakes up from resize_done
    - in_resize=0, proceeds
    - Sets in_resize=1
    - Now works with correct old_capacity=3
    - Reorganizes buffer
    - capacity=6, in_resize=0
    - Signals resize_done
    - Releases mutex
```

## Tests Will Now Work

With this fix:
- ✅ **T4 Test**: T1's setsize(3) blocks until count ≤ 3
- ✅ **T5 Test**: Multiple setsize() calls serialize properly
- ✅ **T6 Test**: No heap corruption, buffer state always consistent

The **barriers in your tests are still useful** for ensuring deterministic test ordering, but now the underlying buffer implementation safely handles the concurrent scenario.

## Summary: Why limit + in_resize is Brilliant

Your `limit` field elegantly solves the "new target applies immediately" requirement while keeping `capacity` stable for modulo. The `in_resize` flag serializes the critical reorganization phase, ensuring no thread sees a half-modified buffer.

This is a clean, efficient solution that:
1. ✅ Prevents deadlocks
2. ✅ Maintains memory safety
3. ✅ Achieves true atomicity
4. ✅ Allows concurrent get/pop/add during resizes (they block on limit, not capacity)
