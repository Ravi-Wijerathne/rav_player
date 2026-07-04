add_executable(playlist_tests
    PlaylistTest.cpp
    PlaybackQueueTest.cpp
    PlaylistParserTest.cpp
)

target_link_libraries(playlist_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(playlist_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(playlist_tests)
