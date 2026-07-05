#pragma once

#include <atomic>
#include <chrono>
#include <cmath>

namespace rav {

class SyncController {
public:
    SyncController() = default;

    void start() {
        start_time_ = std::chrono::steady_clock::now();
        last_correction_ = start_time_;
        running_ = true;
        drift_integral_ = 0.0;
    }

    void pause() {
        pause_time_ = std::chrono::steady_clock::now();
        running_ = false;
    }

    void resume() {
        if (!running_ && pause_time_.time_since_epoch().count() > 0) {
            // Adjust start time to account for paused duration
            auto now = std::chrono::steady_clock::now();
            auto paused_duration = now - pause_time_;
            start_time_ += paused_duration;
            running_ = true;
        }
    }

    void reset() {
        running_ = false;
        drift_integral_ = 0.0;
    }

    // Returns the wall clock elapsed time in seconds
    double elapsed_seconds() const {
        if (!running_) return 0.0;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time_).count();
    }

    // Compute drift: audio_pts - wall_time. Positive means audio is ahead.
    double compute_drift(double audio_pts) const {
        if (!running_) return 0.0;
        double wall = elapsed_seconds();
        return audio_pts - wall;
    }

    // Apply PI controller to compute a corrected clock speed factor.
    // Returns a multiplier for the byte rate (1.0 = no correction).
    double correction_factor(double audio_pts, double kp = 0.01, double ki = 0.001) {
        if (!running_) return 1.0;

        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last_correction_).count();
        if (dt < 0.5) return 1.0; // Only correct every 500ms

        last_correction_ = now;

        double drift = compute_drift(audio_pts);
        drift_integral_ += drift * dt;

        // Clamp integral to prevent windup
        const double max_integral = 0.5;
        if (drift_integral_ > max_integral) drift_integral_ = max_integral;
        if (drift_integral_ < -max_integral) drift_integral_ = -max_integral;

        double correction = kp * drift + ki * drift_integral_;
        // Clamp correction to prevent extreme adjustments
        if (correction > 0.05) correction = 0.05;
        if (correction < -0.05) correction = -0.05;

        return 1.0 - correction;
    }

    double drift_integral() const { return drift_integral_; }

private:
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point pause_time_;
    std::chrono::steady_clock::time_point last_correction_;
    bool running_{false};
    double drift_integral_{0.0};
};

} // namespace rav
