add_executable(ffmpeg_tests
    ContainerReaderTest.cpp
    CodecDiscoveryTest.cpp
)

target_link_libraries(ffmpeg_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(ffmpeg_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(ffmpeg_tests)
