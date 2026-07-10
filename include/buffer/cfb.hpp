#ifndef CYCLIC_FRAGMENT_BUFFER_H
#define CYCLIC_FRAGMENT_BUFFER_H

#include "buffer.hpp"
#include "fetcher/data_fetcher.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

struct CyclicFragmentBuffer : public Buffer {
    std::mutex mutex;
    int head = 0;         // Position of start of valid buffer (locally)
    int cur_start = 0;    // Local head -> global position in file
    int size_present = 0; // Currently amount of usable data present in buffer
    size_t size;          // Size of buffer

    std::thread filler;

    std::condition_variable cv;
    std::atomic_bool filling = false;

    DataFetcher *fetcher;

    CyclicFragmentBuffer(DataFetcher *, unsigned int size);
    ~CyclicFragmentBuffer() override;

    int read(uint8_t *buf, int buf_size) override;

    void refill(bool);
    void advance(int);
    void join_filler();

#ifdef DEBUG
    void print_stats();
#endif
};

#endif
