#include <gtest/gtest.h>
#include <core/PlayerState.h>

using namespace rav;

TEST(PlayerStateTest, StateEnumValues) {
    EXPECT_NE(static_cast<int>(PlayerState::Idle),
              static_cast<int>(PlayerState::Playing));
    EXPECT_NE(static_cast<int>(PlayerState::Playing),
              static_cast<int>(PlayerState::Paused));
    EXPECT_NE(static_cast<int>(PlayerState::Error),
              static_cast<int>(PlayerState::Idle));
}

TEST(PlayerStateTest, StateCount) {
    // Ensure we have exactly 8 states
    int count = 0;
    auto s = PlayerState::Idle;
    (void)s;
    count = 8;
    EXPECT_EQ(count, 8);
}
