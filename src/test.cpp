#include <gtest/gtest.h>
#include "cfb.hpp"

int Factorial(int n) {
    if (n <= 1) return 1;
    else return n * Factorial(n - 1);
}

// Test case for Factorial function
TEST(FactorialTest, HandlesZeroInput) { EXPECT_EQ(Factorial(0), 1); }

TEST(FactorialTest, HandlesPositiveInput) {
    EXPECT_EQ(Factorial(1), 1);
    EXPECT_EQ(Factorial(2), 2);
    EXPECT_EQ(Factorial(3), 6);
    EXPECT_EQ(Factorial(4), 24);
    EXPECT_EQ(Factorial(5), 120);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
