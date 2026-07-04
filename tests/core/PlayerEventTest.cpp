#include <gtest/gtest.h>
#include <core/PlayerEvent.h>

using namespace rav;

TEST(PlayerEventTest, PlaybackStartedEvent) {
    PlaybackStartedEvent e;
    PlayerEvent var = e;
    EXPECT_TRUE(std::holds_alternative<PlaybackStartedEvent>(var));
}

TEST(PlayerEventTest, ErrorOccurredEvent) {
    ErrorOccurredEvent e{"something went wrong"};
    PlayerEvent var = e;
    auto& extracted = std::get<ErrorOccurredEvent>(var);
    EXPECT_EQ(extracted.message, "something went wrong");
}

TEST(PlayerEventTest, StateChangedEvent) {
    StateChangedEvent e{PlayerState::Playing, PlayerState::Paused};
    PlayerEvent var = e;
    auto& extracted = std::get<StateChangedEvent>(var);
    EXPECT_EQ(extracted.previous_state, PlayerState::Playing);
    EXPECT_EQ(extracted.new_state, PlayerState::Paused);
}

TEST(PlayerEventTest, AllEventTypes) {
    EXPECT_EQ(std::variant_size_v<PlayerEvent>, 8);
}
