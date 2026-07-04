#include <gtest/gtest.h>
#include <core/Logger.h>
#include <sstream>

using namespace rav;

TEST(LoggerTest, LoggerExists) {
    auto& log = Logger::instance();
    EXPECT_NO_THROW(log.set_min_level(LogLevel::Trace));
}

TEST(LoggerTest, LogDoesNotCrash) {
    EXPECT_NO_THROW(
        LOG_INFO("test", "hello {}", 42)
    );
}
