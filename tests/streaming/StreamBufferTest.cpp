#include <gtest/gtest.h>
#include <streaming/StreamBuffer.h>

#include <cstring>
#include <thread>

using namespace rav;

TEST(StreamBufferTest, InitiallyEmpty) {
    StreamBuffer buf(1024);
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0);
}

TEST(StreamBufferTest, WriteAndSize) {
    StreamBuffer buf(1024);
    uint8_t data[] = "hello world";
    EXPECT_TRUE(buf.write(data, 11));
    EXPECT_EQ(buf.size(), 11);
    EXPECT_FALSE(buf.empty());
}

TEST(StreamBufferTest, ReadAfterWrite) {
    StreamBuffer buf(1024);
    uint8_t write_data[] = "hello world";
    EXPECT_TRUE(buf.write(write_data, 11));

    uint8_t read_data[11] = {};
    auto nread = buf.read(read_data, 11, 0);
    EXPECT_EQ(nread, 11);
    EXPECT_EQ(std::memcmp(read_data, "hello world", 11), 0);
}

TEST(StreamBufferTest, ReadFromPosition) {
    StreamBuffer buf(1024);
    uint8_t data[] = "abcdefghij";
    EXPECT_TRUE(buf.write(data, 10));

    uint8_t read_data[5] = {};
    auto nread = buf.read(read_data, 5, 5);
    EXPECT_EQ(nread, 5);
    EXPECT_EQ(std::memcmp(read_data, "fghij", 5), 0);
}

TEST(StreamBufferTest, Clear) {
    StreamBuffer buf(1024);
    uint8_t data[] = "test";
    EXPECT_TRUE(buf.write(data, 4));
    EXPECT_FALSE(buf.empty());
    buf.clear();
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0);
}

TEST(StreamBufferTest, FlushMovesReadPosition) {
    StreamBuffer buf(1024);
    uint8_t data[] = "test data";
    EXPECT_TRUE(buf.write(data, 9));
    EXPECT_EQ(buf.size(), 9);
    buf.flush();
    EXPECT_TRUE(buf.empty());
}

TEST(StreamBufferTest, Capacity) {
    StreamBuffer buf(128);
    EXPECT_EQ(buf.capacity(), 128);
}

TEST(StreamBufferTest, FullBuffer) {
    StreamBuffer buf(10);
    uint8_t data[10] = {};
    EXPECT_TRUE(buf.write(data, 10));
    EXPECT_TRUE(buf.full());
    EXPECT_FALSE(buf.write(data, 1));
}

TEST(StreamBufferTest, ReadPositionAccess) {
    StreamBuffer buf(1024);
    EXPECT_EQ(buf.read_position(), 0);
    buf.set_read_position(100);
    EXPECT_EQ(buf.read_position(), 100);
}

TEST(StreamBufferTest, WritePosition) {
    StreamBuffer buf(1024);
    uint8_t data[] = "test";
    buf.write(data, 4);
    EXPECT_EQ(buf.write_position(), 4);
}

TEST(StreamBufferTest, AvailableAtPosition) {
    StreamBuffer buf(1024);
    uint8_t data[] = "hello world";
    buf.write(data, 11);
    EXPECT_EQ(buf.available(0), 11);
    EXPECT_EQ(buf.available(5), 6);
    EXPECT_EQ(buf.available(11), 0);
}

TEST(StreamBufferTest, WaitForData) {
    StreamBuffer buf(1024);
    uint8_t data[] = "enough data";
    buf.write(data, 11);

    auto start = std::chrono::steady_clock::now();
    buf.wait_for_data(5);
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(elapsed, std::chrono::seconds(1));
}
