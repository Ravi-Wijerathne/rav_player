#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace rav {

class StreamBuffer {
public:
    explicit StreamBuffer(size_t capacity = 5 * 1024 * 1024);

    bool write(const uint8_t* data, size_t size);
    size_t read(uint8_t* data, size_t size, int64_t position);

    void clear();
    void flush();

    size_t size() const;
    size_t capacity() const { return capacity_; }
    bool empty() const;
    bool full() const;

    int64_t read_position() const { return read_pos_.load(); }
    int64_t write_position() const { return write_pos_.load(); }

    void set_read_position(int64_t pos);

    double buffered_duration() const;

    size_t available(int64_t position) const;

    void wait_for_data(size_t min_bytes);

private:
    std::vector<uint8_t> buffer_;
    size_t capacity_;
    std::atomic<int64_t> write_pos_{0};
    std::atomic<int64_t> read_pos_{0};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace rav
