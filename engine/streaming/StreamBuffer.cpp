#include "StreamBuffer.h"

#include <algorithm>
#include <cstring>

namespace rav {

StreamBuffer::StreamBuffer(size_t capacity)
    : capacity_(capacity)
    , buffer_(capacity)
{
}

bool StreamBuffer::write(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size > capacity_) return false;

    auto wpos = static_cast<size_t>(write_pos_.load());
    auto rpos = static_cast<size_t>(read_pos_.load());

    size_t used = (wpos >= rpos) ? (wpos - rpos) : 0;
    if (used + size > capacity_) return false;

    size_t start = wpos % capacity_;
    size_t first_chunk = std::min(size, capacity_ - start);
    std::memcpy(&buffer_[start], data, first_chunk);
    if (first_chunk < size) {
        std::memcpy(&buffer_[0], data + first_chunk, size - first_chunk);
    }

    write_pos_.store(static_cast<int64_t>(wpos + size));
    cv_.notify_one();
    return true;
}

size_t StreamBuffer::read(uint8_t* data, size_t size, int64_t position) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto rpos = static_cast<size_t>(position);
    auto wpos = static_cast<size_t>(write_pos_.load());

    if (rpos >= wpos) return 0;

    size_t avail = wpos - rpos;
    size_t to_read = std::min(size, avail);

    size_t start = rpos % capacity_;
    size_t first_chunk = std::min(to_read, capacity_ - start);
    std::memcpy(data, &buffer_[start], first_chunk);
    if (first_chunk < to_read) {
        std::memcpy(data + first_chunk, &buffer_[0], to_read - first_chunk);
    }

    return to_read;
}

void StreamBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    write_pos_.store(0);
    read_pos_.store(0);
    std::fill(buffer_.begin(), buffer_.end(), 0);
}

void StreamBuffer::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    read_pos_.store(write_pos_.load());
}

size_t StreamBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto wpos = static_cast<size_t>(write_pos_.load());
    auto rpos = static_cast<size_t>(read_pos_.load());
    if (wpos >= rpos) return wpos - rpos;
    return 0;
}

bool StreamBuffer::empty() const {
    return size() == 0;
}

bool StreamBuffer::full() const {
    return size() >= capacity_;
}

void StreamBuffer::set_read_position(int64_t pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    read_pos_.store(pos);
}

double StreamBuffer::buffered_duration() const {
    (void)this;
    return 0.0;
}

size_t StreamBuffer::available(int64_t position) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto wpos = static_cast<size_t>(write_pos_.load());
    auto rpos = static_cast<size_t>(position);
    if (wpos >= rpos) return wpos - rpos;
    return 0;
}

void StreamBuffer::wait_for_data(size_t min_bytes) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::seconds(5), [&]() {
        auto wpos = static_cast<size_t>(write_pos_.load());
        auto rpos = static_cast<size_t>(read_pos_.load());
        return (wpos - rpos) >= min_bytes;
    });
}

} // namespace rav
