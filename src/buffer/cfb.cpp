#include <atomic>
#include <cassert>
#include <cstring>
#include <thread>
#include <cstdio>
#include <algorithm>
#include "buffer/cfb.hpp"

using namespace std;

#define ACQUIRE(var) var.load(memory_order_acquire)
#define RELAXED_L(var) var.load(memory_order_relaxed)
#define RELEASE(var, val) var.store(val, memory_order_release)

CyclicFragmentBuffer::CyclicFragmentBuffer(
    DataFetcher *fetcher, size_t size, size_t min_loading_tresh
)
    : fetcher{fetcher}, size{size}, min_loading_tresh{min_loading_tresh} {
    assert(min_loading_tresh < size);
    total_size = fetcher->getTotalSizeLocal();
    base = new uint8_t[size];
    producer_thread = thread{&CyclicFragmentBuffer::producer, this};
}
CyclicFragmentBuffer::~CyclicFragmentBuffer() {
    RELEASE(produce, false);
    wake_up_producer();
    if (producer_thread.joinable()) producer_thread.join();

#ifdef DEBUF
    printf("Producer thread finito\n");
#endif

    delete[] base;
}

void CyclicFragmentBuffer::wake_up(atomic_uint &counter) {
    counter.fetch_add(1, memory_order_acq_rel);
    counter.notify_one();
}
void CyclicFragmentBuffer::wake_up_producer() { wake_up(consumer_counter); }
void CyclicFragmentBuffer::wake_up_consumer() { wake_up(producer_counter); }

void CyclicFragmentBuffer::set_offset(int offset) {
    RELEASE(this->offset, offset);
    wake_up_producer();
}

void CyclicFragmentBuffer::producer() {
    int_type fill_size;
    int_type present_size;
    int_type filling_start;
    int_type consumer_counter;

    int_type offset;
    int_type head;
    int_type tail;
    int_type bytes_left;
    int_type present_valid_size;
    int_type min_loading_tresh;
    auto release_head_tail = [&] {
        RELEASE(this->head, head);
        RELEASE(this->tail, tail);
    };

    bool ahead;
    bool behind;

    while (produce.load(memory_order_acquire)) {
        consumer_counter = ACQUIRE(this->consumer_counter);
        offset = ACQUIRE(this->offset);
        head = RELAXED_L(this->head);
        tail = RELAXED_L(this->tail);

        // Check if we have out of order offset and load if needed
        ahead = offset < head;
        behind = tail < offset;

        // Set buffer to be empty (will be filled below)
        if (ahead || behind) {
            head = offset;
            tail = head;

            release_head_tail();
        }

        // Fill buffer if valid size present is below min tresh
        present_valid_size = max(tail - offset, (int_type)0);
        bytes_left = (int_type)total_size - tail;
        min_loading_tresh = min((int_type)this->min_loading_tresh, bytes_left);

        if (present_valid_size < min_loading_tresh) {
            fill_size = min((int_type)size - present_valid_size, bytes_left);

            filling_start = tail % size;

#ifdef DEBUG
            printf(
                "{\n"
                "\tpresent_valid_size = %ld\n"
                "\tmin_loading_tresh = %ld\n"
                "\tpresent_valid_size < min_loading_tresh = %d\n"
                "\toffset = %ld\n"
                "\thead = %ld\n"
                "\ttail = %ld\n"
                "\ttotal_size = %lu\n"
                "\tbytes_left = %ld\n"
                "\tfill_size = %ld\n"
                "}\n",
                present_valid_size,
                min_loading_tresh,
                present_valid_size < min_loading_tresh,
                offset,
                head,
                tail,
                total_size.load(),
                bytes_left,
                min((int_type)size - present_valid_size, bytes_left)
            );
#endif

            if (filling_start + fill_size <= size) {
                fetcher->fetchFramesLocal(tail, base + filling_start, fill_size);
            } else {
                auto part1 = size - filling_start;
                fetcher->fetchFramesLocal(tail, base + filling_start, part1);
                fetcher->fetchFramesLocal(tail + part1, base, fill_size - part1);
            }

            tail += fill_size;
            if (tail - head > (int_type)size) head = tail - (int_type)size;

            release_head_tail();
        }

        wake_up_consumer();
        this->consumer_counter.wait(consumer_counter, memory_order_relaxed);
    };
}

int CyclicFragmentBuffer::read(uint8_t *buf, int buf_size) {
    int offset = ACQUIRE(this->offset);

    buf_size = min((int_type)buf_size, (int_type)total_size - offset);
    assert(buf_size <= size);
    if (buf_size <= 0) return 0;

    int head, tail;
    bool behind, not_enough;
    auto should_wait_for_producer = [&] {
        head = ACQUIRE(this->head);
        tail = ACQUIRE(this->tail);

        behind = offset < head;
        not_enough = offset + buf_size > tail;

        return behind || not_enough;
    };

    while (should_wait_for_producer()) {
        auto producer_counter = ACQUIRE(this->producer_counter);
        wake_up_producer();
        this->producer_counter.wait(producer_counter, memory_order_relaxed);

        // This is where spinner appears
        // return 0;
    }

    int reading_start = offset % size;
    int reading_end = (offset + buf_size) % size;

    if (reading_start < reading_end) memcpy(buf, base + reading_start, buf_size);
    else {
        auto part1 = size - reading_start;
        auto part2 = buf_size - part1;

        memcpy(buf, base + reading_start, part1);
        memcpy(buf + part1, base, part2);
    }

    this->offset.fetch_add(buf_size, memory_order_acq_rel);

    // We potentially need to refill after read
    wake_up_producer();

    return buf_size;
}
