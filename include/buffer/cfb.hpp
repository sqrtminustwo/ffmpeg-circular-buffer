#ifndef CYCLIC_FRAGMENT_BUFFER_H
#define CYCLIC_FRAGMENT_BUFFER_H

#include "buffer.hpp"
#include "fetcher/data_fetcher.hpp"
#include <shared_mutex>
#include <thread>

enum RefillType { PARTIAL, FULL };

class MutexProtectedAccess : public std::exception {
  public:
    const char *what() const noexcept;
};

class CyclicFragmentBuffer : public Buffer {
    std::shared_mutex mutex;
    int head = 0;        // Position of start of valid buffer (locally)
    int size_present{0}; // Currently amount of usable data present in buffer
    int cur_start = 0;   // Local head -> global position in file
    size_t size;         // Size of buffer

    std::thread filler;

    DataFetcher *fetcher;

    int read(uint8_t *buf, int buf_size) override;

    int non_valid_amount_present();
    void refill(RefillType);
    void advance(int);
    void join_filler();

  public:
#ifndef TEST
    void set_base(uint8_t *) override;
    uint8_t *get_base() const override;
#endif

    int get_size_present() const;
    int get_head() const;

    CyclicFragmentBuffer(DataFetcher *, unsigned int size);
    ~CyclicFragmentBuffer() override;

#ifdef DEBUG
    void print_stats();
#endif
};

#endif
