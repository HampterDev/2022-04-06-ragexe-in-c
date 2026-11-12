if(TARGET Freetype::Freetype)
    return()
endif()
if(NOT ANDROID AND NOT WIN32)
    find_package(Freetype REQUIRED)
    return()
endif()
# Android has no FreeType for apps, and the Windows tests (tests/CMakeLists.txt) link the release
# vendor\build_vendor.bat builds for the Windows client: build it, with only what the client reads
# (no compressed fonts, no PNG glyphs, no shaping). On Windows the archive that script downloaded
# into build\vendor, with the same hash, is unpacked from there when it is there.
include(FetchContent)
foreach(dependency ZLIB BZIP2 PNG HARFBUZZ BROTLI)
    set(FT_DISABLE_${dependency} ON CACHE BOOL "" FORCE)
endforeach()
set(rage_freetype_archive https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/VER-2-13-3)
if(WIN32 AND EXISTS ${PROJECT_SOURCE_DIR}/build/vendor/freetype.tar.gz)
    set(rage_freetype_archive ${PROJECT_SOURCE_DIR}/build/vendor/freetype.tar.gz)
endif()
# Its CMake files predate the runtime setting (policy CMP0091), so the policy is switched on for it.
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
FetchContent_Declare(freetype
    URL ${rage_freetype_archive}
    URL_HASH SHA256=bc5c898e4756d373e0d991bab053036c5eb2aa7c0d5c67e8662ddc6da40c4103)
FetchContent_MakeAvailable(freetype)
add_library(Freetype::Freetype ALIAS freetype)
