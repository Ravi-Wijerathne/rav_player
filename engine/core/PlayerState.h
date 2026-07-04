#pragma once

namespace rav {

enum class PlayerState {
    Idle,
    Loading,
    Playing,
    Paused,
    Seeking,
    Buffering,
    Stopped,
    Error
};

} // namespace rav
