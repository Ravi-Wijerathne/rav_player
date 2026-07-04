#[=======================================================================
# FindFFmpeg.cmake
# Locates FFmpeg libraries (libavcodec, libavformat, libavutil, etc.)
# via Homebrew paths on macOS or system pkg-config.
#=======================================================================]

if(APPLE)
    set(HOMEBREW_PREFIX "/opt/homebrew")
    set(FFMPEG_ROOT "${HOMEBREW_PREFIX}")

    find_path(FFMPEG_INCLUDE_DIR
        NAMES libavcodec/avcodec.h
        PATHS "${FFMPEG_ROOT}/include"
        NO_DEFAULT_PATH
    )

    foreach(component avcodec avformat avutil avfilter swscale swresample avdevice)
        find_library(FFMPEG_${component}_LIBRARY
            NAMES ${component}
            PATHS "${FFMPEG_ROOT}/lib"
            NO_DEFAULT_PATH
        )
        if(FFMPEG_${component}_LIBRARY)
            list(APPEND FFMPEG_LIBRARIES "${FFMPEG_${component}_LIBRARY}")
        endif()
    endforeach()
else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG IMPORTED_TARGET
            libavcodec
            libavformat
            libavutil
            libavfilter
            libswscale
            libswresample
        )
    endif()

    if(NOT FFMPEG_FOUND)
        find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h)
        foreach(component avcodec avformat avutil avfilter swscale swresample avdevice)
            find_library(FFMPEG_${component}_LIBRARY NAMES ${component})
            if(FFMPEG_${component}_LIBRARY)
                list(APPEND FFMPEG_LIBRARIES "${FFMPEG_${component}_LIBRARY}")
            endif()
        endforeach()
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFMPEG_INCLUDE_DIR FFMPEG_LIBRARIES
)

if(FFMPEG_FOUND)
    set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INCLUDE_DIR}")
    if(NOT TARGET FFmpeg::FFmpeg)
        add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
        set_target_properties(FFmpeg::FFmpeg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
        )
    endif()
    mark_as_advanced(FFMPEG_INCLUDE_DIR FFMPEG_LIBRARIES)
endif()
