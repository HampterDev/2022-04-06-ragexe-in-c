if(TARGET Freetype::Freetype)
    return()
endif()
if(NOT ANDROID)
    find_package(Freetype REQUIRED)
    return()
endif()
# Android has no FreeType for apps: build it, with only what the client reads (no compressed fonts,
# no PNG glyphs, no shaping).
include(FetchContent)
foreach(dependency ZLIB BZIP2 PNG HARFBUZZ BROTLI)
    set(FT_DISABLE_${dependency} ON CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(freetype
    URL https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/VER-2-13-3
    URL_HASH SHA256=bc5c898e4756d373e0d991bab053036c5eb2aa7c0d5c67e8662ddc6da40c4103)
FetchContent_MakeAvailable(freetype)
add_library(Freetype::Freetype ALIAS freetype)
