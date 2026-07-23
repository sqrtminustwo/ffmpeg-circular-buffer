#ifndef PRODUCER_H
#define PRODUCER_H

#include "buffer/buffer.hpp"
#include "fetcher/data_fetcher.hpp"
#include <atomic>
#include <thread>

class CyclicFragmentBuffer2 : public Buffer {
    std::atomic<int_type> head = 0; // Start of buffer in file
    std::atomic<int_type> tail = 0; // End of buffer in file
    const size_t size;              // Size of buffer, head and tail should be % this
    const size_t min_loading_tresh; // If buffer size is <= this amount, fetching will be executed

    DataFetcher *fetcher;

    int_type bytes_left(int offset) const;

    /*
     *  Incremented if data has been read from buffer (offset changed)
     *  Or produce was turned off
     */
    void wake_up_producer();
    std::atomic_uint producer_counter = 0;
    std::atomic_bool produce = true;
    std::thread producer_thread;

    int read(uint8_t *buf, int buf_size) override;

  public:
    CyclicFragmentBuffer2() = delete;
    CyclicFragmentBuffer2(DataFetcher *, size_t size, size_t min_loading_tresh);
    ~CyclicFragmentBuffer2() override;

    void producer();
};

#endif
