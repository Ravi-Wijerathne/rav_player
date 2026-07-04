add_executable(video_tests
    FrameQueueTest.cpp
    VideoClockTest.cpp
    VideoFrameTest.cpp
)

target_link_libraries(video_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(video_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(video_tests)
