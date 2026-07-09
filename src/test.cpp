#include <gtest/gtest.h>
#include "cfb.hpp"

struct Test {
    static constexpr size_t bd_size = 10;
    CyclicFragmentBuffer bd{bd_size};
    size_t buf_size;
    uint8_t *buf;

    void rp() { read_packet(&bd, buf, buf_size); }

    Test() {
        bd.offset = 0;

        buf_size = 3;
        buf = new uint8_t[buf_size];
    }

    ~Test() { delete[] buf; }
};

Test t{};

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
        t.bd.offset = start_offset;

        for (int i = 0; i < t.bd_size; i++) {
            auto buf_i = i % t.buf_size;
            if (buf_i == 0) t.rp();
            EXPECT_EQ(t.buf[buf_i], start_offset + i);
        }
    };

    test_full_buffer(0);
    test_full_buffer(67);
    test_full_buffer(13);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
