add_executable(decoder_tests
    VideoDecoderTest.cpp
    AudioDecoderTest.cpp
    PacketQueueTest.cpp
)

target_link_libraries(decoder_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(decoder_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(decoder_tests)
