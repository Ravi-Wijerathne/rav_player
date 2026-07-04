#pragma once

#include <atomic>

#include "../utilities/Clock.h"

namespace rav {

class AudioClock {
public:
    AudioClock() = default;

    void start() {
        clock_.start();
        running_ = true;
    }

    void pause() {
        clock_.pause();
        running_ = false;
    }

    void resume() {
        clock_.resume();
        running_ = true;
    }

    void reset() {
        clock_.reset();
        pts_ = 0.0;
        last_pts_ = 0.0;
        bytes_consumed_ = 0;
        running_ = false;
    }

    double elapsed() const {
        return clock_.elapsed();
    }

    void set_pts(double pts) {
        pts_ = pts;
        clock_.reset();
        clock_.start();
        running_ = true;
    }

    double pts() const {
        return pts_ + clock_.elapsed();
    }

    void set_bytes_per_second(double bps) {
        bytes_per_second_ = bps;
    }

    double bytes_per_second() const { return bytes_per_second_; }

    void add_bytes_consumed(int64_t bytes) {
        bytes_consumed_ += bytes;
    }

    double serial() const { return serial_.load(); }
    void set_serial(double s) { serial_.store(s); }

    bool is_running() const { return running_; }

private:
    Clock clock_;
    std::atomic<double> pts_{0.0};
    double last_pts_{0.0};
    std::atomic<double> serial_{0.0};
    std::atomic<int64_t> bytes_consumed_{0};
    double bytes_per_second_{0.0};
    std::atomic<bool> running_{false};
};

} // namespace rav
