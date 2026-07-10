#ifndef DATA_FETCHER_H
#define DATA_FETCHER_H

#include <cstdint>

struct DataFetcher {
    virtual int getTotalSizeLocal() = 0;
    virtual void fetchFramesLocal(int offset, uint8_t *buf, int length) = 0;
};

#endif
