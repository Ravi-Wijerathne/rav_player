add_executable(core_tests
    PlayerStateTest.cpp
    PlayerCommandTest.cpp
    PlayerEventTest.cpp
    EventBusTest.cpp
    LoggerTest.cpp
    PlaybackEngineTest.cpp
)

target_link_libraries(core_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(core_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(core_tests)
