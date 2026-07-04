add_library(rav_engine STATIC
    dummy.cpp
    # core
    core/PlayerState.h
    core/PlayerCommand.h
    core/PlayerEvent.h
    core/EventBus.h
    core/Logger.h
    # utilities
    utilities/RingBuffer.h
    utilities/ThreadPool.h
    utilities/Clock.h
)

target_include_directories(rav_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rav_engine
    PUBLIC
        ${FFMPEG_LIBRARIES}
)

target_include_directories(rav_engine
    PUBLIC
        ${FFMPEG_INCLUDE_DIRS}
)

# ── Module subdirectories (stubs for future) ─────────────────────────────
# Each module will become its own target or be folded into rav_engine.
# For now we add their include paths so code can reference them.
set(ENGINE_MODULES
    media ffmpeg video audio subtitles streaming
    playlist metadata plugins platform rendering decoder
)

foreach(module ${ENGINE_MODULES})
    file(GLOB_RECURSE module_sources ${module}/*.cpp ${module}/*.mm ${module}/*.h)
    if(module_sources)
        target_sources(rav_engine PRIVATE ${module_sources})
    endif()
    target_include_directories(rav_engine
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/${module}
    )
endforeach()

if(APPLE)
    target_link_libraries(rav_engine PUBLIC
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
        "-framework AudioToolbox"
    )

    # Enable Obj-C++ for .mm files
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/rendering/MetalRenderer.mm
        PROPERTIES COMPILE_FLAGS "-ObjC++"
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/MacOSAudioOutput.mm
        PROPERTIES COMPILE_FLAGS "-ObjC++"
    )
endif()
