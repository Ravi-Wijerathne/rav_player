add_executable(metadata_tests
    MetadataExtractorTest.cpp
    ArtworkExtractorTest.cpp
)

target_link_libraries(metadata_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
)

target_include_directories(metadata_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

include(GoogleTest)
gtest_discover_tests(metadata_tests)
