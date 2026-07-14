#include <gtest/gtest.h>
#include "buffer/cfb.hpp"
#include "fetcher/mock_fetcher.hpp"

struct Test {
    static constexpr int bd_size = 10;
    MockFetcher fetcher{};
    CyclicFragmentBuffer bd{&fetcher, bd_size};
    size_t buf_size;
    uint8_t *buf;

    void rp() { bd.avio_read_packet(&bd, buf, buf_size); }

    Test() {
        bd.set_offset(0);

        buf_size = 3;
        buf = new uint8_t[buf_size];
    }

    ~Test() { delete[] buf; }
};

Test t{};

void equal_bufs(std::vector<int> buf_1, uint8_t *buf_2, size_t size) {
    for (int i = 0; i < size; i++) EXPECT_EQ(buf_1[i], buf_2[i]);
}

TEST(FFmpegCircularBufferTest, BasicCircularLoadingTest) {
    int start_elem = 0;
    for (int i = 0; i < 10; i++) {
        t.rp();

        for (int i = 0; i < t.buf_size; i++) EXPECT_EQ(t.buf[i], start_elem + i);

        start_elem += t.buf_size;
    }
}

TEST(FFmpegCircularBufferTest, NotLoadedOffsetTest) {
    auto test_full_buffer = [](int start_offset) {
        t.bd.set_offset(start_offset);

        for (int i = 0; i < t.bd_size; i++) {
            auto buf_i = i % t.buf_size;
            if (buf_i == 0) t.rp();
            EXPECT_EQ(t.buf[buf_i], start_offset + i);
        }
    };

    for (auto &i : {0, t.bd_size * 4, t.bd_size * 2}) test_full_buffer(i);
}

/*
 * Tests paired to buf fixes
 */

TEST(FFmpegCircularBufferTest, DoesNotGoOutOfBounds) {
    // We take t.bd_size / 2, because we want to test
    // that it will not try to load t.bd_size bytes even when
    // only t.bd_size / 2 bytes can be loaded as we are at
    // the end of the file

    auto total_size = t.fetcher.total_size;
    auto should_be_loaded = t.bd_size / 2;
    t.bd.set_offset(total_size - should_be_loaded);

    t.rp();

    // 3 bytes are read by rp
    // thats why we subtract
    EXPECT_EQ(t.bd.get_size_present(), should_be_loaded - 3);
    EXPECT_EQ(t.bd.get_base()[t.bd.get_head() + t.bd.get_size_present() - 1], total_size - 1);
}

TEST(FFmpegCircularBufferTest, InBoundsNotEnoughOnSecondAsk) {
    t.bd.set_offset(0);

    t.rp();
    equal_bufs({0, 1, 2}, t.buf, 3);

    t.bd.set_offset(7);

    t.rp();
    equal_bufs({7, 8, 9}, t.buf, 3);

    t.rp();
    equal_bufs({10, 11, 12}, t.buf, 3);
}

TEST(FFmpegCircularBufferTest, OffsetInBoundsButNotEnoughData) {
    t.bd.set_offset(0);
    t.rp();

    t.bd.set_offset(8);
    t.rp();
    equal_bufs({8, 9, 10}, t.buf, 3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
