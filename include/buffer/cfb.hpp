#ifndef CYCLIC_FRAGMENT_BUFFER_H
#define CYCLIC_FRAGMENT_BUFFER_H

#include "buffer.hpp"
#include "buffer/fill_guard.hpp"
#include "fetcher/data_fetcher.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class MutexProtectedAccess : public std::exception {
  public:
    const char *what() const noexcept;
};

class CyclicFragmentBuffer : public Buffer {
    std::mutex mutex;
    int head = 0;         // Position of start of valid buffer (locally)
    int size_present = 0; // Currently amount of usable data present in buffer
    int cur_start = 0;    // Local head -> global position in file
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
    void set_base(uint8_t *) override;
    uint8_t *get_base() override;

    int get_size_present() const;
    int get_head() const;

    CyclicFragmentBuffer(DataFetcher *, unsigned int size);
    ~CyclicFragmentBuffer() override;

#ifdef DEBUG
    void print_stats();
#endif
};

#endif
