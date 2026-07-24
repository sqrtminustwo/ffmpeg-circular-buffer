#include <cstdint>
#include <gtest/gtest.h>
#include <unistd.h>
#include "buffer/cfb.hpp"
#include "fetcher/mock_fetcher.hpp"

struct TestWrapper {
    static constexpr int bd_size = 10;
    MockFetcher fetcher{};
    CyclicFragmentBuffer *bd = nullptr;
    size_t buf_size;
    uint8_t *buf;

    int rp() {
        assert(bd_size > 0);
        int ret = -1;
        // Mandatory, background thread needs time to wakeup / fill the buffer
        while (!(ret = bd->avio_read_packet(&(*bd), buf, buf_size)))
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return ret;
    }

    void destruct_bd() {
        if (bd) {
            delete bd;
            bd = nullptr;
        }
    }

    TestWrapper() {
        bd = new CyclicFragmentBuffer{&fetcher, bd_size, bd_size / 2};
        bd->set_offset(0);

        buf_size = 3;
        buf = new uint8_t[buf_size];
    }

    ~TestWrapper() {
        destruct_bd();
        delete[] buf;
    }
};

void equal_bufs(std::vector<int> buf_1, uint8_t *buf_2, size_t size) {
    for (int i = 0; i < size; i++) EXPECT_EQ(buf_1[i], buf_2[i]);
}

#ifdef DEBUG
void print_buf(int buf_size, uint8_t *buf) {
    for (int i = 0; i < buf_size; i++) printf("%d ", buf[i]);
    printf("\n");
}
#endif

// TEST(FFmpegCircularBufferTest, ZeroOnNotEnough) {
//     TestWrapper t{};
//
//     t.bd->set_offset(3);
//
//     auto buf_size = 10;
//     auto buf = new uint8_t[buf_size];
//
//     int ret = t.bd->avio_read_packet(&(*t.bd), buf, buf_size);
//     EXPECT_EQ(ret, 0);
//
//     delete[] buf;
// }

TEST(FFmpegCircularBufferTest, BasicCircularLoadingTest) {
    TestWrapper t{};
    int start_elem = 0;
    for (int i = 0; i < 10; i++) {
        t.rp();

        for (int i = 0; i < t.buf_size; i++) EXPECT_EQ(t.buf[i], start_elem + i);

        start_elem += t.buf_size;

#ifdef DEBUG
        print_buf(t.buf_size, t.buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        print_buf(t.bd_size, t.bd->get_base());
#endif
    }
}

TEST(FFmpegCircularBufferTest, NotLoadedOffsetTest) {
    TestWrapper t{};
    auto test_full_buffer = [&](int start_offset) {
        t.bd->set_offset(start_offset);

        for (int i = 0; i < t.bd_size; i++) {
            auto buf_i = i % t.buf_size;
            if (buf_i == 0) t.rp();
            EXPECT_EQ(t.buf[buf_i], start_offset + i);
        }
    };

    // for (auto &i : {0, t.bd_size * 4, t.bd_size * 2}) test_full_buffer(i);
    // for (auto &i : {0, t.bd_size * 4}) test_full_buffer(i);
    for (auto &i : {0, t.bd_size * 4}) test_full_buffer(i);
}

/*
 * Tests paired to bug fixes
 */

TEST(FFmpegCircularBufferTest, DoesNotGoOutOfBounds) {
    TestWrapper t{};
    // We take t.bd_size / 2, because we want to test
    // that it will not try to load t.bd_size bytes even when
    // only t.bd_size / 2 bytes can be loaded as we are at
    // the end of the file

    auto total_size = t.fetcher.total_size;
    auto should_be_loaded = t.bd_size / 2;
    auto offset = total_size - should_be_loaded;
    t.bd->set_offset(offset);

    t.rp();

    // Joins producer thread
    t.destruct_bd();

    // 3 bytes are read by rp
    // thats why we subtract
    EXPECT_EQ(t.fetcher.calls.size(), 1);

#ifdef DEBUG
    for (auto &call : t.fetcher.calls) std::cout << call << "\n";
#endif

    auto call = t.fetcher.calls.front();
    EXPECT_EQ(call.offset, offset);
    EXPECT_EQ(call.length, should_be_loaded);
}

TEST(FFmpegCircularBufferTest, InBoundsNotEnoughOnSecondAsk) {
    TestWrapper t{};

    t.bd->set_offset(0);

    t.rp();
    equal_bufs({0, 1, 2}, t.buf, 3);

#ifdef DEBUG
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    print_buf(t.bd_size, t.bd->get_base());
#endif

    t.bd->set_offset(7);

#ifdef DEBUG
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    print_buf(t.bd_size, t.bd->get_base());
#endif

    t.rp();
    equal_bufs({7, 8, 9}, t.buf, 3);

#ifdef DEBUG
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    print_buf(t.bd_size, t.bd->get_base());
#endif

    t.rp();
    equal_bufs({10, 11, 12}, t.buf, 3);

#ifdef DEBUG
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    print_buf(t.bd_size, t.bd->get_base());
#endif
}

TEST(FFmpegCircularBufferTest, OffsetInBoundsButNotEnoughData) {
    TestWrapper t{};

    t.bd->set_offset(0);
    t.rp();

    t.bd->set_offset(8);
    t.rp();
    equal_bufs({8, 9, 10}, t.buf, 3);
}

TEST(FFmpegCircularBufferTest, DifferentBytesLeftInReadAndProduce) {
    TestWrapper t{};

    // TODO:
}
