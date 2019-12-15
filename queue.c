#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// This is not representative of a production quality lock-free queue
// but rather a simple replication of a model that was made in TLA+

#define QUEUE_SIZE 128

#ifdef USE_ATOMICS
#define ATOMIC _Atomic
#define LOAD_ACQUIRE(X) atomic_load_explicit(X, memory_order_acquire)
#define STORE_RELEASE(X, Y) atomic_store_explicit(X, Y, memory_order_release)
#else
#define ATOMIC
#define LOAD_ACQUIRE(X) *(X)
#define STORE_RELEASE(X, Y) *(X) = (Y)
#endif

struct spsc_queue {
    int elements[QUEUE_SIZE];
    ATOMIC size_t write_cursor;
    ATOMIC size_t read_cursor;
};

void init_queue(struct spsc_queue *queue) {
    memset(queue, 0, sizeof(*queue));
}

bool write_queue(struct spsc_queue *queue, int val) {
    // The orderings in here are stricter than required
    // when loading the locally modified variables
    size_t reader = LOAD_ACQUIRE(&queue->read_cursor);
    size_t writer = LOAD_ACQUIRE(&queue->write_cursor);

    if (reader - writer >= QUEUE_SIZE) {
        return false;
    }

    size_t write_index = writer % QUEUE_SIZE;

    queue->elements[write_index] = val;

    STORE_RELEASE(&queue->write_cursor, writer+1);
    return true;
}

bool read_queue(struct spsc_queue *queue, int *value) {
    // The orderings in here are stricter than required
    // when loading the locally modified variables
    size_t writer = LOAD_ACQUIRE(&queue->write_cursor);
    size_t reader = LOAD_ACQUIRE(&queue->read_cursor);

    if (reader >= writer) {
        return false;
    }

    size_t read_index = reader % QUEUE_SIZE;

    *value = queue->elements[read_index];

    STORE_RELEASE(&queue->read_cursor, reader + 1);
    return true;
}

int num_iterations=1000*1000*100;

struct spsc_queue queue;

void *reader_loop(void * _ignore) {
    for (int i = 0; i < num_iterations; i++) {
        int expected = i+1;
        int loaded;
        while (!read_queue(&queue, &loaded));
        assert(expected == loaded);
    }
    return NULL;
}

void writer_loop() {
    for (int i = 0; i < num_iterations; i++) {
        int expected = i+1;
        while (!write_queue(&queue, expected));
    }
}

int main() {
    init_queue(&queue);

    pthread_t reader;
    pthread_create(&reader, NULL, reader_loop, NULL);

    writer_loop();
    pthread_join(reader, NULL);

    int dump;
    assert(!read_queue(&queue, &dump));
    return 0;
}
