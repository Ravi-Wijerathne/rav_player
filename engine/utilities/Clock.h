#pragma once

#include <chrono>

namespace rav {

class Clock {
public:
    using time_point = std::chrono::steady_clock::time_point;

    Clock() = default;

    void start() {
        running_ = true;
        start_time_ = std::chrono::steady_clock::now();
        paused_elapsed_ = std::chrono::nanoseconds::zero();
    }

    void pause() {
        if (!running_) return;
        running_ = false;
        paused_elapsed_ = std::chrono::nanoseconds(
            static_cast<int64_t>(elapsed() * 1'000'000'000.0));
    }

    void resume() {
        if (running_) return;
        running_ = true;
        start_time_ = std::chrono::steady_clock::now() - paused_elapsed_;
    }

    void reset() {
        running_ = false;
        paused_elapsed_ = std::chrono::nanoseconds::zero();
        start_time_ = std::chrono::steady_clock::now();
    }

    void set(double seconds) {
        paused_elapsed_ = std::chrono::nanoseconds(
            static_cast<int64_t>(seconds * 1'000'000'000.0));
        if (running_) {
            start_time_ = std::chrono::steady_clock::now() - paused_elapsed_;
        }
    }

    double elapsed() const {
        if (running_) {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<double>(now - start_time_).count();
        }
        return std::chrono::duration<double>(paused_elapsed_).count();
    }

    bool is_running() const { return running_; }

private:
    bool running_{false};
    time_point start_time_;
    std::chrono::nanoseconds paused_elapsed_{0};
};

} // namespace rav
