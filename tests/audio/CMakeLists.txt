add_executable(audio_tests
    AudioClockTest.cpp
    AudioQueueTest.cpp
    AudioFrameTest.cpp
    AudioMixerTest.cpp
)

target_link_libraries(audio_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(audio_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(audio_tests)
