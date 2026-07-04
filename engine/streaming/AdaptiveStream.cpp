#include "AdaptiveStream.h"

#include <algorithm>
#include <cmath>

namespace rav {

AdaptiveStream::AdaptiveStream(const AdaptiveStreamConfig& config)
    : config_(config)
{
    current_bitrate_ = config.initial_bitrate;
}

void AdaptiveStream::set_bandwidth_callback(BandwidthCallback cb) {
    bw_callback_ = std::move(cb);
}

void AdaptiveStream::add_variant(int bandwidth, int width, int height) {
    variants_.push_back({bandwidth, width, height});
    if (selected_variant_ < 0) {
        select_bitrate();
    }
}

void AdaptiveStream::set_quality(AdaptiveStreamQuality quality) {
    quality_ = quality;
}

AdaptiveStreamQuality AdaptiveStream::quality() const {
    return quality_;
}

void AdaptiveStream::report_download_time(int bytes, std::chrono::milliseconds elapsed) {
    if (elapsed.count() == 0) return;
    double instant_bw = (bytes * 8.0) / (elapsed.count() / 1000.0);
    if (measured_bandwidth_ == 0.0) {
        measured_bandwidth_ = instant_bw;
    } else {
        measured_bandwidth_ = 0.75 * measured_bandwidth_ + 0.25 * instant_bw;
    }
}

int AdaptiveStream::select_bitrate() {
    if (variants_.empty()) {
        current_bitrate_ = config_.initial_bitrate;
        return current_bitrate_;
    }

    if (quality_ == AdaptiveStreamQuality::Low) {
        auto it = std::min_element(variants_.begin(), variants_.end(),
            [](const Variant& a, const Variant& b) { return a.bandwidth < b.bandwidth; });
        selected_variant_ = static_cast<int>(std::distance(variants_.begin(), it));
        current_bitrate_ = it->bandwidth;
        return current_bitrate_;
    }

    if (quality_ == AdaptiveStreamQuality::High) {
        auto it = std::max_element(variants_.begin(), variants_.end(),
            [](const Variant& a, const Variant& b) { return a.bandwidth < b.bandwidth; });
        selected_variant_ = static_cast<int>(std::distance(variants_.begin(), it));
        current_bitrate_ = it->bandwidth;
        return current_bitrate_;
    }

    double bw = measured_bandwidth_;
    if (bw_callback_) {
        bw = bw_callback_();
    }

    if (bw <= 0.0) {
        if (selected_variant_ < 0) {
            selected_variant_ = 0;
        }
        current_bitrate_ = variants_[selected_variant_].bandwidth;
        return current_bitrate_;
    }

    int target_idx = 0;
    for (size_t i = 0; i < variants_.size(); ++i) {
        if (variants_[i].bandwidth <= bw * config_.upswitch_ratio) {
            target_idx = static_cast<int>(i);
        }
    }

    if (selected_variant_ >= 0) {
        if (target_idx > selected_variant_ &&
            variants_[target_idx].bandwidth > bw * config_.upswitch_ratio) {
            target_idx = selected_variant_;
        }
        if (target_idx < selected_variant_ &&
            variants_[selected_variant_].bandwidth < bw * config_.downswitch_ratio) {
        } else if (target_idx < selected_variant_) {
            target_idx = selected_variant_;
        }
    }

    selected_variant_ = std::clamp(target_idx, 0, static_cast<int>(variants_.size()) - 1);
    current_bitrate_ = variants_[selected_variant_].bandwidth;
    return current_bitrate_;
}

void AdaptiveStream::reset() {
    measured_bandwidth_ = 0.0;
    selected_variant_ = -1;
    current_bitrate_ = config_.initial_bitrate;
}

} // namespace rav
