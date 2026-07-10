#include "fetcher/mock_fetcher.hpp"

int MockFetcher::getTotalSizeLocal() { return total_size; }

void MockFetcher::fetchFramesLocal(int offset, uint8_t *buf, int length) {
    for (int i = 0; i < length; i++) {
        int num = offset + i;
        if (num >= total_size) num = -1;
        buf[i] = num;
    }
}
