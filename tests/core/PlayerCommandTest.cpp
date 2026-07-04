#include <gtest/gtest.h>
#include <core/PlayerCommand.h>

using namespace rav;

TEST(PlayerCommandTest, PlayCommand) {
    PlayCommand cmd;
    PlayerCommand var = cmd;
    EXPECT_TRUE(std::holds_alternative<PlayCommand>(var));
}

TEST(PlayerCommandTest, SeekCommand) {
    SeekCommand cmd{42.5};
    PlayerCommand var = cmd;
    auto& extracted = std::get<SeekCommand>(var);
    EXPECT_DOUBLE_EQ(extracted.position_seconds, 42.5);
}

TEST(PlayerCommandTest, LoadMediaCommand) {
    LoadMediaCommand cmd{"/path/to/file.mp4"};
    PlayerCommand var = cmd;
    auto& extracted = std::get<LoadMediaCommand>(var);
    EXPECT_EQ(extracted.url, "/path/to/file.mp4");
}

TEST(PlayerCommandTest, AllCommandTypes) {
    EXPECT_EQ(std::variant_size_v<PlayerCommand>, 7);
}
