#pragma once

#include <string>
#include <variant>

#include "PlayerState.h"

namespace rav {

struct PlaybackStartedEvent {};
struct PlaybackPausedEvent {};
struct PlaybackEndedEvent {};
struct MediaLoadedEvent {
    double duration_seconds{0.0};
};
struct BufferingStartedEvent {};
struct BufferingEndedEvent {};
struct StateChangedEvent {
    PlayerState previous_state{PlayerState::Idle};
    PlayerState new_state{PlayerState::Idle};
};
struct ErrorOccurredEvent {
    std::string message;
};

using PlayerEvent = std::variant<
    PlaybackStartedEvent,
    PlaybackPausedEvent,
    PlaybackEndedEvent,
    MediaLoadedEvent,
    BufferingStartedEvent,
    BufferingEndedEvent,
    StateChangedEvent,
    ErrorOccurredEvent>;

} // namespace rav
