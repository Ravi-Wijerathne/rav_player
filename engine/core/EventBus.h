#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

namespace rav {

template <typename T>
class EventBus {
public:
    using Listener = std::function<void(const T&)>;

    void publish(const T& event) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(event);
        cv_.notify_one();
    }

    bool poll(T& out) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop();
        return true;
    }

    void subscribe(Listener listener) {
        std::lock_guard<std::mutex> lock(listeners_mutex_);
        listeners_.push_back(std::move(listener));
    }

    void dispatch() {
        T event;
        while (poll(event)) {
            std::lock_guard<std::mutex> lock(listeners_mutex_);
            for (auto& listener : listeners_) {
                listener(event);
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        std::lock_guard<std::mutex> lock_l(listeners_mutex_);
        while (!queue_.empty()) queue_.pop();
        listeners_.clear();
    }

private:
    std::queue<T> queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::vector<Listener> listeners_;
    std::mutex listeners_mutex_;
};

} // namespace rav
