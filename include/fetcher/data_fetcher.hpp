#ifndef DATA_FETCHER_H
#define DATA_FETCHER_H

#include <cstdint>

using int_type = int64_t;

struct DataFetcher {
    virtual int getTotalSizeLocal() = 0;
    virtual void fetchFramesLocal(int_type offset, uint8_t *buf, int_type length) = 0;
};

#endif
