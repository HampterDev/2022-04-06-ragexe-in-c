if(TARGET SDL3::SDL3)
    return()
endif()
find_package(SDL3 3.4 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    if(ANDROID)
        # SDL's Java activity loads libSDL3.so and then the client's libmain.so.
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    else()
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    endif()
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(rage_sdl3_archive https://codeload.github.com/libsdl-org/SDL/tar.gz/829a65d769d935c4852f8159e964312c0957260a)
    if(WIN32)
        # The Windows tests (tests/CMakeLists.txt) link SDL as vendor\build_vendor.bat builds it for
        # the client: audio, video and the GPU API on Vulkan, no OpenGL, no renderer or input devices
        # beyond keyboard and mouse. That script downloaded the same archive into build\vendor and
        # checked the same hash; when it is there it is unpacked from there, into this build's own
        # copy, which does not depend on build\vendor afterwards.
        foreach(option VIDEO GPU VULKAN)
            set(SDL_${option} ON CACHE BOOL "" FORCE)
        endforeach()
        foreach(option EXAMPLES OPENGL OPENGLES RENDER CAMERA JOYSTICK HAPTIC HIDAPI SENSOR POWER DIALOG TRAY)
            set(SDL_${option} OFF CACHE BOOL "" FORCE)
        endforeach()
        if(EXISTS ${PROJECT_SOURCE_DIR}/build/vendor/sdl3.tar.gz)
            set(rage_sdl3_archive ${PROJECT_SOURCE_DIR}/build/vendor/sdl3.tar.gz)
        endif()
    endif()
    FetchContent_Declare(SDL3
        URL ${rage_sdl3_archive}
        URL_HASH SHA256=48e15cf1a6928453d77afa955d1fdfbbca9169e2bc3d76d111acb9b8bb076d66)
    if(MSVC)
        # SDL compiles with /utf-8, which the client's /source-charset:utf-8 and
        # /execution-charset:.949 (the top CMakeLists.txt) contradict: SDL gets its own options.
        get_property(rage_options DIRECTORY PROPERTY COMPILE_OPTIONS)
        set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
        FetchContent_MakeAvailable(SDL3)
        set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "${rage_options}")
    else()
        FetchContent_MakeAvailable(SDL3)
    endif()
endif()
