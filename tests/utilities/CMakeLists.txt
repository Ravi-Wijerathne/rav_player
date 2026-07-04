add_executable(utilities_tests
    RingBufferTest.cpp
    ThreadPoolTest.cpp
    ClockTest.cpp
)

target_link_libraries(utilities_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(utilities_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(utilities_tests)
