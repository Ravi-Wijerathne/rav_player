#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "../ffmpeg/FFmpegContext.h"

namespace rav {

class PacketQueue {
public:
    explicit PacketQueue(size_t max_size = 120)
        : max_size_(max_size) {}

    bool push(PacketPtr packet) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.size() >= max_size_) {
                return false;
            }
            queue_.push(std::move(packet));
        }
        cv_.notify_one();
        return true;
    }

    PacketPtr pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || draining_; });
        if (draining_ && queue_.empty()) return nullptr;
        PacketPtr pkt = std::move(queue_.front());
        queue_.pop();
        return pkt;
    }

    bool try_pop(PacketPtr& out, int timeout_ms = 0) {
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

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) queue_.pop();
    }

    void set_max_size(size_t max) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_size_ = max;
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
    std::queue<PacketPtr> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool draining_{false};
};

} // namespace rav
