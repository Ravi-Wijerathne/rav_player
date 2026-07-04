add_executable(media_tests
    LocalFileSourceTest.cpp
    StreamSourceTest.cpp
)

target_link_libraries(media_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(media_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(media_tests)
