add_executable(streaming_tests
    HLSPlaylistTest.cpp
    AdaptiveStreamTest.cpp
    StreamBufferTest.cpp
)

target_link_libraries(streaming_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(streaming_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(streaming_tests)
