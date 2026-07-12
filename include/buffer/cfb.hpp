#ifndef CYCLIC_FRAGMENT_BUFFER_H
#define CYCLIC_FRAGMENT_BUFFER_H

#include "buffer.hpp"
#include "buffer/fill_guard.hpp"
#include "fetcher/data_fetcher.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class CyclicFragmentBuffer : public Buffer {
    std::mutex mutex;
    int head = 0;         // Position of start of valid buffer (locally)
    int cur_start = 0;    // Local head -> global position in file
    int size_present = 0; // Currently amount of usable data present in buffer
    size_t size;          // Size of buffer

    std::thread filler;

    std::condition_variable cv;
    std::atomic_bool filling = false;

    DataFetcher *fetcher;

    int read(uint8_t *buf, int buf_size) override;

    void refill(RefillType, int = 0);
    void advance(int);
    void join_filler();

  public:
    int get_size_present() const;
    int get_head() const;

    CyclicFragmentBuffer(DataFetcher *, unsigned int size);
    ~CyclicFragmentBuffer() override;

#ifdef DEBUG
    void print_stats();
#endif
};

#endif
