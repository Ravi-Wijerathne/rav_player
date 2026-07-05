find_program(XCRUN xcrun)

add_executable(platform_tests
    MacOSAudioOutputTest.mm
    MetalRendererTest.mm
    VideoToolboxDecoderTest.mm
)

target_link_libraries(platform_tests
    PRIVATE
        rav_engine
        GTest::gmock_main
        "-framework Foundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework Metal"
        "-framework VideoToolbox"
)

target_include_directories(platform_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/engine
)

if(XCRUN)
    set(METAL_SOURCE ${CMAKE_SOURCE_DIR}/engine/rendering/shaders.metal)
    set(METALLIB_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.metallib)

    add_custom_command(
        OUTPUT ${METALLIB_OUTPUT}
        COMMAND ${XCRUN} -sdk macosx metal -c ${METAL_SOURCE}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders.air
        COMMAND ${XCRUN} -sdk macosx metallib
            ${CMAKE_CURRENT_BINARY_DIR}/shaders.air
            -o ${METALLIB_OUTPUT}
        COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/shaders.air
        DEPENDS ${METAL_SOURCE}
        COMMENT "Compiling Metal shaders for platform tests"
    )

    add_custom_target(platform_metallib DEPENDS ${METALLIB_OUTPUT})
    add_dependencies(platform_tests platform_metallib)
endif()

include(GoogleTest)
gtest_discover_tests(platform_tests)
