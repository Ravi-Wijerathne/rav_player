#pragma once

#include <atomic>

#include "../utilities/Clock.h"

namespace rav {

class VideoClock {
public:
    VideoClock() = default;

    void start() {
        real_clock_.start();
        last_update_ = real_clock_.elapsed();
    }

    void set_pts(double pts, double serial) {
        pts_ = pts;
        last_pts_ = pts_;
        serial_ = serial;
        last_update_ = real_clock_.elapsed();
    }

    double pts() const {
        if (real_clock_.is_running()) {
            double elapsed_since_update = real_clock_.elapsed() - last_update_;
            return pts_ + elapsed_since_update;
        }
        return pts_;
    }

    double serial() const { return serial_.load(); }

    double last_update() const { return last_update_.load(); }

    void set_sync_ref_clock(Clock* clock) {
        sync_ref_ = clock;
    }

    void update() {
        if (!sync_ref_) return;
        double elapsed_since_update = real_clock_.elapsed() - last_update_;
        double expected_pts = pts_ + elapsed_since_update;
        pts_.store(expected_pts);
    }

    void pause() {
        real_clock_.pause();
    }

    void resume() {
        real_clock_.resume();
    }

    void reset() {
        pts_ = 0.0;
        last_pts_ = 0.0;
        serial_ = 0.0;
        last_update_ = 0.0;
        real_clock_.reset();
    }

    bool needs_frame() const {
        if (!sync_ref_) return true;
        double master = sync_ref_->elapsed();
        return pts_ <= master;
    }

    double duration_to_next_frame() const {
        if (!sync_ref_) return 0.0;
        double master = sync_ref_->elapsed();
        double diff = pts_ - master;
        return diff > 0 ? diff : 0.0;
    }

private:
    std::atomic<double> pts_{0.0};
    double last_pts_{0.0};
    std::atomic<double> serial_{0.0};
    std::atomic<double> last_update_{0.0};
    Clock real_clock_;
    Clock* sync_ref_{nullptr};
};

} // namespace rav
