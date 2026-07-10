#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include "buffer/cfb.hpp"

#include <cstdio>

#ifdef DEBUG
#include <iostream>
void CyclicFragmentBuffer::print_stats() {
    printf(
        "head = %d, size_present = %d, offset = %d, cur_start = %d, size = %zu, "
        "total_size = %zu\n",
        head,
        size_present,
        offset,
        cur_start,
        size,
        total_size
    );
}

void print_buf(int buf_size, uint8_t *buf) {
    for (int i = 0; i < buf_size; i++) printf("%d ", buf[i]);
    printf("\n");
}
#endif

CyclicFragmentBuffer::CyclicFragmentBuffer(DataFetcher *fetcher, unsigned int size)
    : fetcher{fetcher}, size{size} {
    total_size = fetcher->getTotalSizeLocal();
    base = new uint8_t[size];
}
CyclicFragmentBuffer::~CyclicFragmentBuffer() {
    join_filler();
    delete[] base;
}

void CyclicFragmentBuffer::refill(bool full = false) {
    // std::lock_guard<std::mutex> lock(mutex);

    if (full) {
        head = 0;
        cur_start = offset;
        size_present = 0;
    }

    auto fill_start = (head + size_present) % size;
    auto offset_start = cur_start + size_present;

    int fill_size = size - size_present;
    int left_size = total_size - offset;
    fill_size = std::min(fill_size, left_size);

    if (fill_size <= 0) return;

    // TODO: abstract trough inheritance
    if (fill_start <= head) fetcher->fetchFramesLocal(offset_start, base + fill_start, fill_size);
    else {
        auto part1 = size - fill_start;
        fetcher->fetchFramesLocal(offset_start, base + fill_start, size - fill_start);
        fetcher->fetchFramesLocal(offset_start + part1, base, head);
    }

    size_present += fill_size;

    // filling = false;
    //
    // #ifdef DEBUG
    //     std::cout << "[Background Thread] Done\n";
    // #endif
    //
    //     cv.notify_all();
}

void CyclicFragmentBuffer::advance(int buf_size) {
    auto consumed_size = (offset - cur_start) + buf_size;

    offset += buf_size;
    cur_start += consumed_size;
    size_present -= consumed_size;
    head = (head + consumed_size) % size;
}

void CyclicFragmentBuffer::join_filler() {
    if (filler.joinable()) filler.join();
}

int CyclicFragmentBuffer::read(uint8_t *buf, int buf_size) {
    // If filling thread has the lock the first line will stop threading errors
    // Ifspurious wakeup filling thread does not yet have lock second line will
    // While loop fixes spurious wakeup
    // std::unique_lock<std::mutex> lock(mutex);
    // while (filling) cv.wait(lock);

    assert(size >= buf_size);

    int bytes_left = total_size - offset;
    buf_size = std::min(buf_size, bytes_left);

    // Needed because function pointer is passed to ffmpeg function
    if (buf_size <= 0) return -1; // End of file

    auto not_enough = size_present <= buf_size;
    auto ahead = offset < cur_start;
    auto behind = cur_start + size_present < offset;

#ifdef DEBUG
    printf("not_enough = %d, ahead = %d, behind = %d\n", not_enough, ahead, behind);
#endif
    if (not_enough || ahead || behind) {
        // TODO: reuse valid bytes
        // lock.unlock();
        refill(true);
        // lock.lock();
    }

    // Assumes necessary data is present

    auto start_in_buffer = (offset - cur_start) + head;
    start_in_buffer %= size;

    auto end_in_buffer = start_in_buffer + buf_size;
    end_in_buffer %= size;

    if (end_in_buffer > start_in_buffer) memcpy(buf, base + start_in_buffer, buf_size);
    else {
        auto part1 = size - start_in_buffer;
        auto part2 = buf_size - part1;

        memcpy(buf, base + start_in_buffer, part1);
        memcpy(buf + part1, base, part2);
    }

#ifdef DEBUG
    // printf("before advance:\n");
    print_stats();
    print_buf(size, base);
#endif

    advance(buf_size);

#ifdef DEBUG
    // printf("after advance:\n");
    print_stats();
    print_buf(size, base);
#endif
    // if (size_present <= size / 2) {
    //     join_filler();
    //     filling = true;
    //     filler = std::thread(&CyclicFragmentBuffer::refill, this, false);
    // }

#ifdef DEBUG
    print_buf(buf_size, buf);
    printf("-------------------------------\n");
#endif

    return buf_size;
}
