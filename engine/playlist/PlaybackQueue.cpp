#include "PlaybackQueue.h"

#include <algorithm>
#include <stdexcept>

namespace rav {

void PlaybackQueue::set_items(const std::vector<PlaylistItem>& items) {
    items_ = items;
    current_ = 0;
    rebuild_play_order();
}

void PlaybackQueue::add_item(const PlaylistItem& item) {
    items_.push_back(item);
    if (shuffle_) {
        play_order_.push_back(items_.size() - 1);
    }
}

void PlaybackQueue::remove_item(size_t index) {
    if (index >= items_.size()) return;
    items_.erase(items_.begin() + static_cast<ptrdiff_t>(index));
    if (current_ >= items_.size() && !items_.empty()) {
        current_ = items_.size() - 1;
    }
    rebuild_play_order();
}

void PlaybackQueue::clear() {
    items_.clear();
    play_order_.clear();
    current_ = 0;
}

bool PlaybackQueue::has_next() const {
    if (items_.empty()) return false;
    if (repeat_ == RepeatMode::All || repeat_ == RepeatMode::One) return true;
    return current_ + 1 < items_.size();
}

bool PlaybackQueue::has_previous() const {
    if (items_.empty()) return false;
    if (repeat_ == RepeatMode::All) return true;
    return current_ > 0;
}

const PlaylistItem& PlaybackQueue::current() const {
    if (items_.empty()) {
        throw std::out_of_range("queue is empty");
    }
    return items_[get_index(current_)];
}

const PlaylistItem& PlaybackQueue::next() {
    if (items_.empty()) {
        throw std::out_of_range("queue is empty");
    }
    if (repeat_ == RepeatMode::One) {
        return items_[get_index(current_)];
    }
    if (repeat_ == RepeatMode::All && current_ + 1 >= items_.size()) {
        current_ = 0;
        return items_[get_index(current_)];
    }
    current_ = std::min(current_ + 1, items_.size() - 1);
    return items_[get_index(current_)];
}

const PlaylistItem& PlaybackQueue::previous() {
    if (items_.empty()) {
        throw std::out_of_range("queue is empty");
    }
    if (current_ > 0) {
        current_--;
    } else if (repeat_ == RepeatMode::All) {
        current_ = items_.size() - 1;
    }
    return items_[get_index(current_)];
}

void PlaybackQueue::go_to(size_t index) {
    if (index < items_.size()) {
        current_ = index;
    }
}

void PlaybackQueue::set_shuffle(bool enabled) {
    shuffle_ = enabled;
    rebuild_play_order();
}

void PlaybackQueue::set_repeat(RepeatMode mode) {
    repeat_ = mode;
}

void PlaybackQueue::rebuild_play_order() {
    play_order_.clear();
    play_order_.reserve(items_.size());
    for (size_t i = 0; i < items_.size(); ++i) {
        play_order_.push_back(i);
    }
    if (shuffle_) {
        static std::mt19937 rng(std::random_device{}());
        std::shuffle(play_order_.begin(), play_order_.end(), rng);
    }
}

size_t PlaybackQueue::get_index(size_t pos) const {
    if (play_order_.empty() || pos >= play_order_.size()) return pos;
    return play_order_[pos];
}

} // namespace rav
