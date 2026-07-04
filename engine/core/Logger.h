#pragma once

#include <chrono>
#include <ctime>
#include <format>
#include <iostream>
#include <mutex>
#include <string>

namespace rav {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    template <typename... Args>
    void log(LogLevel level, const std::string& tag,
             std::string_view fmt, Args&&... args) {
        if (level < min_level_) return;

        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) % 1000;

        std::string msg = std::vformat(fmt, std::make_format_args(args...));

        char time_buf[16];
        std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", std::localtime(&t));

        std::string out = std::format(
            "{}:{:03d} [{:5}] [{}] {}",
            time_buf, ms.count(), level_str(level), tag, msg);

        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << out << std::endl;

        if (level == LogLevel::Fatal) {
            std::cerr << "FATAL: " << msg << std::endl;
            std::abort();
        }
    }

    void set_min_level(LogLevel level) { min_level_ = level; }

private:
    Logger() = default;

    static constexpr const char* level_str(LogLevel l) {
        switch (l) {
            case LogLevel::Trace:   return "TRACE";
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO ";
            case LogLevel::Warning: return "WARN ";
            case LogLevel::Error:   return "ERROR";
            case LogLevel::Fatal:   return "FATAL";
        }
        return "UNKWN";
    }

    LogLevel min_level_{LogLevel::Debug};
    std::mutex mutex_;
};

} // namespace rav

#define LOG_TRACE(tag, ...)  rav::Logger::instance().log(rav::LogLevel::Trace, tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...)  rav::Logger::instance().log(rav::LogLevel::Debug, tag, __VA_ARGS__)
#define LOG_INFO(tag, ...)   rav::Logger::instance().log(rav::LogLevel::Info, tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)   rav::Logger::instance().log(rav::LogLevel::Warning, tag, __VA_ARGS__)
#define LOG_ERROR(tag, ...)  rav::Logger::instance().log(rav::LogLevel::Error, tag, __VA_ARGS__)
#define LOG_FATAL(tag, ...)  rav::Logger::instance().log(rav::LogLevel::Fatal, tag, __VA_ARGS__)
