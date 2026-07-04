#pragma once

#include <string>
#include <variant>

namespace rav {

struct PlayCommand {};
struct PauseCommand {};
struct StopCommand {};
struct NextCommand {};
struct PreviousCommand {};

struct SeekCommand {
    double position_seconds{0.0};
};

struct LoadMediaCommand {
    std::string url;
};

using PlayerCommand = std::variant<
    PlayCommand,
    PauseCommand,
    StopCommand,
    NextCommand,
    PreviousCommand,
    SeekCommand,
    LoadMediaCommand>;

} // namespace rav
