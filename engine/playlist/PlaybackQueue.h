#pragma once

#include <memory>
#include <random>
#include <vector>

#include "PlaylistItem.h"

namespace rav {

enum class RepeatMode {
    None,
    One,
    All
};

class PlaybackQueue {
public:
    PlaybackQueue() = default;

    void set_items(const std::vector<PlaylistItem>& items);
    void add_item(const PlaylistItem& item);
    void remove_item(size_t index);
    void clear();

    bool has_next() const;
    bool has_previous() const;

    const PlaylistItem& current() const;
    const PlaylistItem& next();
    const PlaylistItem& previous();

    int current_index() const { return current_; }

    void go_to(size_t index);

    void set_shuffle(bool enabled);
    bool shuffle() const { return shuffle_; }

    void set_repeat(RepeatMode mode);
    RepeatMode repeat() const { return repeat_; }

    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

private:
    std::vector<PlaylistItem> items_;
    std::vector<size_t> play_order_;
    size_t current_{0};
    bool shuffle_{false};
    RepeatMode repeat_{RepeatMode::None};

    void rebuild_play_order();
    size_t get_index(size_t pos) const;
};

} // namespace rav
