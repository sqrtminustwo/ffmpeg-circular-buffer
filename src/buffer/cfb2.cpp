#include <atomic>
#include <cassert>
#include <cstring>
#include <thread>
#include <cstdio>
#include "buffer/cfb2.hpp"

using namespace std;

#define ACQUIRE(var) var.load(memory_order_acquire)
#define RELAXED_L(var) var.load(memory_order_relaxed)
#define RELEASE(var, val) var.store(val, memory_order_release)

CyclicFragmentBuffer2::CyclicFragmentBuffer2(
    DataFetcher *fetcher, size_t size, size_t min_loading_tresh
)
    : fetcher{fetcher}, size{size}, min_loading_tresh{min_loading_tresh} {
    assert(min_loading_tresh < size);
    total_size = fetcher->getTotalSizeLocal();
    base = new uint8_t[size];
    producer_thread = thread{&CyclicFragmentBuffer2::producer, this};
}
CyclicFragmentBuffer2::~CyclicFragmentBuffer2() {
    RELEASE(produce, false);
    wake_up_producer();
    if (producer_thread.joinable()) producer_thread.join();
    delete[] base;
}

void CyclicFragmentBuffer2::wake_up_producer() {
    producer_counter.fetch_add(1, memory_order_acq_rel);
    producer_counter.notify_one();
}

int_type CyclicFragmentBuffer2::bytes_left(int offset) const { return total_size - offset; }

void CyclicFragmentBuffer2::producer() {
    int_type fill_size;
    int_type present_size;
    int_type filling_start;
    int_type counter;

    int_type offset;
    int_type head;
    int_type tail;
    auto release_head_tail = [&] {
        RELEASE(this->head, head);
        RELEASE(this->tail, tail);
    };

    bool ahead;
    bool behind;

    while (produce.load(memory_order_acquire)) {
        counter = ACQUIRE(producer_counter);
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
        int present_valid_size = max(tail - offset, (int_type)0);
        // printf(
        //     "present_valid_size = %d, offset = %ld, head = %ld, tail = %ld\n",
        //     present_valid_size,
        //     offset,
        //     head,
        //     tail
        // );
        if (present_valid_size < min_loading_tresh) {
            fill_size = min((int_type)size - present_valid_size, bytes_left(offset));

            filling_start = tail % size;

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

        this->producer_counter.wait(counter, memory_order_relaxed);
    };
}

int CyclicFragmentBuffer2::read(uint8_t *buf, int buf_size) {
    int offset = ACQUIRE(this->offset);

    buf_size = min((int_type)buf_size, bytes_left(offset));
    assert(buf_size < size);
    if (buf_size <= 0) return 0;

    int head = ACQUIRE(this->head);
    int tail = ACQUIRE(this->tail);

    bool behind = offset < head;
    bool not_enough = offset + buf_size > tail;

    // printf("offset = %d, behind = %d, not_enough = %d\n", offset, behind, not_enough);

    if (behind || not_enough) {
        wake_up_producer();
        // This is where spinner appears
        return 0;
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
