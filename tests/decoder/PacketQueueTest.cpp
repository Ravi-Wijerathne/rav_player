#include <gtest/gtest.h>
#include "decoder/PacketQueue.h"
#include <thread>
#include <chrono>

using namespace rav;

TEST(PacketQueueTest, InitiallyEmpty) {
    PacketQueue queue(32);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);
}

TEST(PacketQueueTest, PushAndPop) {
    PacketQueue queue(32);
    auto pkt = PacketPtr(av_packet_alloc());
    auto* raw = pkt.get();
    EXPECT_TRUE(queue.push(std::move(pkt)));
    EXPECT_EQ(queue.size(), 1u);
    EXPECT_FALSE(queue.empty());
    auto popped = queue.pop();
    EXPECT_EQ(popped.get(), raw);
}

TEST(PacketQueueTest, MaxSizeEvicts) {
    PacketQueue queue(2);
    EXPECT_TRUE(queue.push(PacketPtr(av_packet_alloc())));
    EXPECT_TRUE(queue.push(PacketPtr(av_packet_alloc())));
    EXPECT_FALSE(queue.push(PacketPtr(av_packet_alloc())));
}

TEST(PacketQueueTest, DrainReturnsNullptr) {
    PacketQueue queue(32);
    queue.drain();
    auto pkt = queue.pop();
    EXPECT_EQ(pkt, nullptr);
}

TEST(PacketQueueTest, TryPopTimeout) {
    PacketQueue queue(32);
    PacketPtr out;
    bool result = queue.try_pop(out, 10);
    EXPECT_FALSE(result);
    EXPECT_EQ(out, nullptr);
}

TEST(PacketQueueTest, Clear) {
    PacketQueue queue(32);
    queue.push(PacketPtr(av_packet_alloc()));
    EXPECT_FALSE(queue.empty());
    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST(PacketQueueTest, ResetRecoversFromDrain) {
    PacketQueue queue(32);
    queue.drain();
    queue.reset();
    EXPECT_TRUE(queue.push(PacketPtr(av_packet_alloc())));
    auto pkt = queue.pop();
    EXPECT_NE(pkt, nullptr);
}

TEST(PacketQueueTest, SetMaxSize) {
    PacketQueue queue(1);
    EXPECT_TRUE(queue.push(PacketPtr(av_packet_alloc())));
    EXPECT_FALSE(queue.push(PacketPtr(av_packet_alloc())));
    queue.set_max_size(2);
    EXPECT_TRUE(queue.push(PacketPtr(av_packet_alloc())));
}
