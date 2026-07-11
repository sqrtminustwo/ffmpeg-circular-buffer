#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <cstdio>

struct Buffer {
    uint8_t *base;     // Fixed pointer to the start of buffer memory
    int offset = 0;    // Current reading position in total file
    size_t total_size; // Fixed total size of the file

    virtual ~Buffer() = default;

    static int avio_read_packet(void *opaque, uint8_t *buf, int buf_size) {
        auto *b = static_cast<Buffer *>(opaque);
        // printf("read_packet -> offset = %d, buf_size = %d\n", b->offset, buf_size);
        auto res = b->read(buf, buf_size);
        // printf("done\n");
        return res;
    }

  private:
    virtual int read(uint8_t *buf, int buf_size) = 0;

    int normalize_size(int);
    int normalize_size_default();
};

#endif
