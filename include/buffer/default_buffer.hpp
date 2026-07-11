#ifndef DEFAULT_BUFFER_H
#define DEFAULT_BUFFER_H

#include "buffer.hpp"

class DefaultBuffer : public Buffer {
    int read(uint8_t *buf, int buf_size) override;
};

#endif
