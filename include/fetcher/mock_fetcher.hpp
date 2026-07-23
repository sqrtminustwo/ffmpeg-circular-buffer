#ifndef MOCK_FETCHER_H
#define MOCK_FETCHER_H

#include "data_fetcher.hpp"
#include <vector>
#include <iostream>

struct FetchFramesCall {
    int_type offset;
    int_type length;

    FetchFramesCall(int_type offset, int_type length);

    friend std::ostream &operator<<(std::ostream &, const FetchFramesCall &);
};

struct MockFetcher : public DataFetcher {
    static constexpr int total_size = 100;
    std::vector<FetchFramesCall> calls{};

    int getTotalSizeLocal() override;
    void fetchFramesLocal(int_type offset, uint8_t *buf, int_type length) override;
};

#endif
