#ifndef BUFFER_H
#define BUFFER_H

#include <atomic>
#include <cstdint>
#include <cstdio>

class Buffer {
  protected:
    std::atomic_int offset{0};        // Current reading position in total file
    std::atomic_size_t total_size{0}; // Fixed total size of the file
    uint8_t *base;                    // Fixed pointer to the start of buffer memory

    virtual int read(uint8_t *buf, int buf_size) = 0;

    int normalize_size(int);
    int normalize_size_default();

  public:
    virtual ~Buffer() = default;

    int get_offset() const;
    virtual void set_offset(int);
    size_t get_total_size() const;
    void set_total_size(size_t);
    virtual void set_base(uint8_t *);
    virtual uint8_t *get_base() const;

    static int avio_read_packet(void *opaque, uint8_t *buf, int buf_size) {
        auto *b = static_cast<Buffer *>(opaque);
        // printf("read_packet -> offset = %d, buf_size = %d\n", b->offset, buf_size);
        auto res = b->read(buf, buf_size);
        // printf("done\n");
        return res;
    }
};

#endif
