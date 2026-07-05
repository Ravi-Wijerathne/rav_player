#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

#include "SubtitleFrame.h"

namespace rav {

class SubtitleQueue {
public:
    SubtitleQueue() = default;

    void push(SubtitleFrame frame) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(frame));
        }
    }

    // Get all subtitles active at the given time, then remove expired ones
    std::vector<SubtitleFrame> subtitles_at_time(double time_seconds) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Remove expired subtitles (those whose end_time has passed)
        while (!queue_.empty() && queue_.front().end_time < time_seconds) {
            queue_.pop_front();
        }

        // Collect active subtitles
        std::vector<SubtitleFrame> active;
        for (const auto& sub : queue_) {
            if (sub.start_time <= time_seconds && sub.end_time >= time_seconds) {
                active.push_back(sub);
            }
        }
        return active;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

private:
    std::deque<SubtitleFrame> queue_;
    mutable std::mutex mutex_;
};

} // namespace rav
