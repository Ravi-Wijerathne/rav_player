#include <gtest/gtest.h>
#include <utilities/RingBuffer.h>

using namespace rav;

TEST(RingBufferTest, PushAndPop) {
    RingBuffer<int> buf(4);
    EXPECT_TRUE(buf.push(1));
    EXPECT_TRUE(buf.push(2));

    auto val = buf.pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);

    val = buf.pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 2);

    EXPECT_TRUE(buf.empty());
}

TEST(RingBufferTest, Overflow) {
    RingBuffer<int> buf(3);
    EXPECT_TRUE(buf.push(1));
    EXPECT_TRUE(buf.push(2));
    EXPECT_TRUE(buf.push(3));
    EXPECT_TRUE(buf.full());
    EXPECT_FALSE(buf.push(4));
}

TEST(RingBufferTest, EmptyBuffer) {
    RingBuffer<int> buf(4);
    EXPECT_TRUE(buf.empty());
    auto val = buf.pop();
    EXPECT_FALSE(val.has_value());
}

TEST(RingBufferTest, Clear) {
    RingBuffer<int> buf(4);
    buf.push(1);
    buf.push(2);
    EXPECT_FALSE(buf.empty());
    buf.clear();
    EXPECT_TRUE(buf.empty());
}

TEST(RingBufferTest, Size) {
    RingBuffer<int> buf(4);
    EXPECT_EQ(buf.size(), 0);
    buf.push(10);
    EXPECT_EQ(buf.size(), 1);
    buf.push(20);
    EXPECT_EQ(buf.size(), 2);
    buf.pop();
    EXPECT_EQ(buf.size(), 1);
}

TEST(RingBufferTest, WrapAround) {
    RingBuffer<int> buf(4);
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.pop(); // 1
    buf.pop(); // 2
    buf.push(4);
    buf.push(5);
    // Buffer should now contain 3, 4, 5

    EXPECT_EQ(buf.size(), 3);
    EXPECT_EQ(*buf.pop(), 3);
    EXPECT_EQ(*buf.pop(), 4);
    EXPECT_EQ(*buf.pop(), 5);
    EXPECT_TRUE(buf.empty());
}
