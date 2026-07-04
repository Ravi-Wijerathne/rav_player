#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rav {

struct AdaptiveStreamConfig {
    int initial_bitrate{500000};
    int min_bitrate{100000};
    int max_bitrate{10000000};
    double upswitch_ratio{1.5};
    double downswitch_ratio{0.7};
    int stability_threshold_ms{5000};
    int max_buffer_ms{30000};
    int min_buffer_ms{3000};
};

enum class AdaptiveStreamQuality {
    Low,
    Medium,
    High,
    Auto
};

class AdaptiveStream {
public:
    using BandwidthCallback = std::function<double()>;

    explicit AdaptiveStream(const AdaptiveStreamConfig& config = {});

    void set_bandwidth_callback(BandwidthCallback cb);

    void add_variant(int bandwidth, int width, int height);

    int select_bitrate();

    void report_download_time(int bytes, std::chrono::milliseconds elapsed);

    void set_quality(AdaptiveStreamQuality quality);

    AdaptiveStreamQuality quality() const;

    int current_bitrate() const { return current_bitrate_; }
    int variant_count() const { return static_cast<int>(variants_.size()); }

    void reset();

private:
    struct Variant {
        int bandwidth;
        int width;
        int height;
    };

    AdaptiveStreamConfig config_;
    std::vector<Variant> variants_;
    std::atomic<int> current_bitrate_{0};
    AdaptiveStreamQuality quality_{AdaptiveStreamQuality::Auto};
    BandwidthCallback bw_callback_;

    double measured_bandwidth_{0.0};
    int selected_variant_{-1};
};

} // namespace rav
