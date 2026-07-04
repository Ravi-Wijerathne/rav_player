#include <gtest/gtest.h>
#include <core/EventBus.h>
#include <core/PlayerCommand.h>
#include <core/PlayerEvent.h>

using namespace rav;

TEST(EventBusTest, PublishAndPollCommand) {
    EventBus<PlayerCommand> bus;
    bus.publish(PlayCommand{});

    PlayerCommand cmd;
    EXPECT_TRUE(bus.poll(cmd));
    EXPECT_TRUE(std::holds_alternative<PlayCommand>(cmd));
}

TEST(EventBusTest, PollReturnsFalseWhenEmpty) {
    EventBus<PlayerCommand> bus;
    PlayerCommand cmd;
    EXPECT_FALSE(bus.poll(cmd));
}

TEST(EventBusTest, PublishAndPollEvent) {
    EventBus<PlayerEvent> bus;
    bus.publish(PlaybackStartedEvent{});

    PlayerEvent evt;
    EXPECT_TRUE(bus.poll(evt));
    EXPECT_TRUE(std::holds_alternative<PlaybackStartedEvent>(evt));
}

TEST(EventBusTest, SubscribeAndDispatch) {
    EventBus<PlayerEvent> bus;
    int called = 0;

    bus.subscribe([&](const PlayerEvent&) { ++called; });
    bus.publish(PlaybackStartedEvent{});
    bus.dispatch();

    EXPECT_EQ(called, 1);
}

TEST(EventBusTest, MultipleSubscribers) {
    EventBus<PlayerEvent> bus;
    int a = 0, b = 0;

    bus.subscribe([&](const PlayerEvent&) { ++a; });
    bus.subscribe([&](const PlayerEvent&) { ++b; });
    bus.publish(PlaybackStartedEvent{});
    bus.dispatch();

    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
}

TEST(EventBusTest, FIFOOrder) {
    EventBus<int> bus;
    bus.publish(1);
    bus.publish(2);
    bus.publish(3);

    int val;
    EXPECT_TRUE(bus.poll(val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(bus.poll(val));
    EXPECT_EQ(val, 2);
    EXPECT_TRUE(bus.poll(val));
    EXPECT_EQ(val, 3);
    EXPECT_FALSE(bus.poll(val));
}
