#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace rav {

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : buffer_(capacity + 1), capacity_(capacity + 1) {}

    bool push(const T& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (current_head + 1) % capacity_;
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[current_head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        T item = buffer_[current_tail];
        tail_.store((current_tail + 1) % capacity_, std::memory_order_release);
        return item;
    }

    size_t size() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        if (h >= t) return h - t;
        return capacity_ - t + h;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t next = (head_.load(std::memory_order_acquire) + 1) % capacity_;
        return next == tail_.load(std::memory_order_acquire);
    }

    void clear() {
        tail_.store(head_.load(std::memory_order_relaxed), std::memory_order_release);
    }

private:
    std::vector<T> buffer_;
    size_t capacity_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

} // namespace rav
