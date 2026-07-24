#include "fetcher/mock_fetcher.hpp"

FetchFramesCall::FetchFramesCall(int_type offset, int_type length)
    : offset{offset}, length{length} {}

bool FetchFramesCall::operator==(FetchFramesCall const &other) const {
    return offset == other.offset && length == other.length;
}
bool FetchFramesCall::operator!=(FetchFramesCall const &other) const {
    return offset != other.offset || length != other.length;
}

std::ostream &operator<<(std::ostream &os, const FetchFramesCall &obj) {
    os << "FetchFramesCall{offset = " << obj.offset << ", length = " << obj.length << "}";
    return os;
}

void MockFetcher::print_all_calls() const {
    for (auto &call : calls) std::cout << call << "\n";
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
