#ifndef MOCK_FETCHER_H
#define MOCK_FETCHER_H

#include "data_fetcher.hpp"

struct MockFetcher : public DataFetcher {
    static constexpr int total_size = 100;

    int getTotalSizeLocal() override;
    void fetchFramesLocal(int offset, uint8_t *buf, int length) override;
};

#endif
