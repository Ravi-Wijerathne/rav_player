add_executable(rendering_tests
    FrameConverterTest.cpp
)

target_link_libraries(rendering_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(rendering_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(rendering_tests)
