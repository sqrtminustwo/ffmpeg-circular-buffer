#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "buffer/default_buffer.hpp"

int DefaultBuffer::read(uint8_t *buf, int buf_size) {
    int bytes_left = total_size - offset;
    buf_size = std::min(buf_size, bytes_left);

    if (buf_size <= 0) return -1;

    memcpy(buf, base + offset, buf_size);

    offset += buf_size;

    return buf_size;
}
