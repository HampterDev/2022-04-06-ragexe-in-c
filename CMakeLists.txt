cmake_minimum_required(VERSION 3.20)
project(ragexe_port LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
include(CTest)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
include(cmake/SourceEncoding.cmake)
option(RAGE_SANITIZERS "Enable address and undefined-behavior checks" OFF)
option(RAGE_PLATFORM_PROBE "Build the optional SDL3 window, IME and Metal bring-up probe" OFF)
option(RAGE_CLIENT "Build the full client on Linux (GCC)" OFF)
option(RAGE_ORACLE "Build only the oracle tests: 32-bit Windows, against the original executable (tests/oracle)" OFF)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/source-charset:utf-8 /execution-charset:.949)
else()
    # The client was written for MSVC, whose char is signed; ARM Linux and Android make it unsigned.
    add_compile_options(-Wall -Wextra -Werror=implicit-function-declaration -fsigned-char)
    if(RAGE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined,float-cast-overflow)
    endif()
endif()

if(RAGE_ORACLE)
    add_subdirectory(tests/oracle)
    return()
endif()

# Tests run headless and silent: SDL's dummy video and audio drivers, unless a test sets its own
# (tests/CMakeLists.txt, rage_test_run).
set(RAGE_TEST_ENVIRONMENT SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy)

if(RAGE_PLATFORM_PROBE)
    include(cmake/PlatformProbe.cmake)
endif()
if(RAGE_CLIENT)
    enable_language(CXX)
    include(cmake/SDL3.cmake)
elseif(BUILD_TESTING AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    # The password encryption a Windows test links (vendor/crijndael) is C++.
    enable_language(CXX)
endif()

add_subdirectory(vendor)

function(rage_enable_assertions target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -UNDEBUG)
    endif()
endfunction()

add_executable(lua_chunk tools/lua_chunk.c)
target_link_libraries(lua_chunk PRIVATE rage_lua)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(RAGE_CLIENT)
    include(cmake/Client.cmake)
endif()
