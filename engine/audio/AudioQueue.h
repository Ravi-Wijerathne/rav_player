#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "AudioFrame.h"

namespace rav {

class AudioQueue {
public:
    explicit AudioQueue(size_t max_size = 30)
        : max_size_(max_size) {}

    bool push(AudioFrame frame) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.size() >= max_size_) {
                return false;
            }
            queue_.push(std::move(frame));
        }
        cv_.notify_one();
        return true;
    }

    AudioFrame pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || draining_; });
        if (draining_ && queue_.empty()) return {};
        AudioFrame frame = std::move(queue_.front());
        queue_.pop();
        return frame;
    }

    bool try_pop(AudioFrame& out, int timeout_ms = 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                          [this] { return !queue_.empty() || draining_; })) {
            return false;
        }
        if (draining_ && queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t max_size() const { return max_size_; }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) queue_.pop();
    }

    void drain() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            draining_ = true;
        }
        cv_.notify_all();
    }

    void reset() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            draining_ = false;
            while (!queue_.empty()) queue_.pop();
        }
    }

private:
    size_t max_size_;
    std::queue<AudioFrame> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool draining_{false};
};

} // namespace rav
