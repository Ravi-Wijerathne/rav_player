add_executable(plugins_tests
    PluginManagerTest.cpp
)

target_link_libraries(plugins_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(plugins_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(plugins_tests)
