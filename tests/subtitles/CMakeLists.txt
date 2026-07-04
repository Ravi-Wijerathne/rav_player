add_executable(subtitle_tests
    SRTParserTest.cpp
    ASSParserTest.cpp
    SubtitleFrameTest.cpp
)

target_link_libraries(subtitle_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(subtitle_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(subtitle_tests)
