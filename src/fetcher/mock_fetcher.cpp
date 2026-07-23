#include "fetcher/mock_fetcher.hpp"

FetchFramesCall::FetchFramesCall(int_type offset, int_type length)
    : offset{offset}, length{length} {}

std::ostream &operator<<(std::ostream &os, const FetchFramesCall &obj) {
    os << "FetchFramesCall{offset = " << obj.offset << ", length = " << obj.length << "}";
    return os;
}

int MockFetcher::getTotalSizeLocal() { return total_size; }

void MockFetcher::fetchFramesLocal(int_type offset, uint8_t *buf, int_type length) {
    calls.push_back(FetchFramesCall{offset, length});
    for (int i = 0; i < length; i++) {
        int num = offset + i;
        if (num >= total_size) num = -1;
        buf[i] = num;
    }
}
