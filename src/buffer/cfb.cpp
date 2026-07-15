#include <algorithm>
#include <cassert>
#include <cstring>
#include <mutex>
#include <thread>
#include <cstdio>
#include "buffer/cfb.hpp"

using namespace std;

using write_lock = unique_lock<shared_mutex>;
using read_lock = shared_lock<shared_mutex>;

#ifdef DEBUG
#include <iostream>

void CyclicFragmentBuffer::print_stats() {
    printf(
        "head = %d, size_present = %d, offset = %d, cur_start = %d, size = %zu, "
        "total_size = %zu\n",
        head,
        size_present,
        get_offset(),
        cur_start,
        size,
        get_total_size()
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

const char *MutexProtectedAccess::what() const noexcept {
    return "Variable is protected by mutex, you can't access it.";
}

#ifndef TEST
void CyclicFragmentBuffer::set_base(uint8_t *) { throw MutexProtectedAccess(); }
uint8_t *CyclicFragmentBuffer::get_base() { throw MutexProtectedAccess(); }
#endif

int CyclicFragmentBuffer::get_size_present() const { return size_present; }
int CyclicFragmentBuffer::get_head() const { return head; }

void CyclicFragmentBuffer::refill(RefillType refill_type) {
    int fill_start, offset_start, fill_size;

    {
        write_lock lock(mutex);

        if (refill_type == FULL) {
            head = 0;
            cur_start = offset;
            size_present = 0;
        }

        fill_start = (head + size_present) % size;
        offset_start = cur_start + size_present;

        if (refill_type == FULL) fill_size = size;
        else if (refill_type == PARTIAL) fill_size = size - size_present;
        else fill_size = 0;

        int left_size = total_size - offset_start;
        fill_size = min(fill_size, left_size);

        if (fill_size <= 0) return;
    }

    if (fill_start + fill_size <= size) {
        fetcher->fetchFramesLocal(offset_start, base + fill_start, fill_size);
    } else {
        auto part1 = size - fill_start;
        fetcher->fetchFramesLocal(offset_start, base + fill_start, part1);
        fetcher->fetchFramesLocal(offset_start + part1, base, fill_size - part1);
    }

    {
        write_lock lock(mutex);
        size_present += fill_size;
    }

#ifdef DEBUG
    cout << "Filling done\n";
#endif
}

int CyclicFragmentBuffer::non_valid_amount_present() { return offset - cur_start; }

void CyclicFragmentBuffer::advance(int buf_size) {
    write_lock lock{mutex};

    auto consumed_size = non_valid_amount_present() + buf_size;

    offset += buf_size;
    cur_start += consumed_size;
    size_present -= consumed_size;
    head = (head + consumed_size) % size;
}

void CyclicFragmentBuffer::join_filler() {
    if (filler.joinable()) filler.join();
}

int CyclicFragmentBuffer::read(uint8_t *buf, int buf_size) {
    {
        // If filling thread has the lock the first line will stop threading errors
        // Ifspurious wakeup filling thread does not yet have lock second line will
        // While loop fixes spurious wakeup
        read_lock lock{mutex};

        assert(size >= buf_size);

        int bytes_left = total_size - offset;
        buf_size = min(buf_size, bytes_left);

        // Needed because function pointer is passed to ffmpeg function
        if (buf_size <= 0) return -1; // End of file

        bool not_enough = false, ahead = false, behind = false;

        auto out_of_order = [&]() {
            not_enough = (size_present - non_valid_amount_present()) <= buf_size;
            ahead = offset < cur_start;
            behind = cur_start + size_present < offset;

            return not_enough || ahead || behind;
        };

        // There might be a filler thread that will get the needed bytes already in progress
        if (out_of_order()) {
            lock.unlock();
            join_filler();
            lock.lock();
        }

        if (out_of_order()) {
            lock.unlock();
            refill(FULL);
            lock.lock();
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
        print_stats();
        // print_buf(size, base);
#endif
    }

    advance(buf_size);

    {
        read_lock lock(mutex);

#ifdef DEBUG
        print_stats();
        printf("size_present <= size / 2 = %d\n", size_present <= (int)size / 2);
#endif

        if (size_present <= size / 2) {
            // For now only 1 filler thread can be run at the same time
            lock.unlock();
            join_filler();
            lock.lock();
            filler = thread(&CyclicFragmentBuffer::refill, this, PARTIAL);
        }

#ifdef DEBUG
        printf("-------------------------------\n");
#endif
    }

    return buf_size;
}
