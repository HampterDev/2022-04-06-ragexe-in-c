if(WIN32)
    message(FATAL_ERROR "The Windows client builds with build.bat")
endif()
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "The client build needs GCC: its narrow literals are compiled as CP949 with -fexec-charset")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/SDL3.cmake)
find_package(Freetype REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Threads REQUIRED)

file(GLOB_RECURSE RAGE_CLIENT_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.c)
file(STRINGS ${PROJECT_SOURCE_DIR}/tools/posix_excluded.txt RAGE_CLIENT_EXCLUDED REGEX "^[^#]")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tools/posix_excluded.txt)
list(TRANSFORM RAGE_CLIENT_EXCLUDED PREPEND ${PROJECT_SOURCE_DIR}/)
list(REMOVE_ITEM RAGE_CLIENT_SOURCES ${RAGE_CLIENT_EXCLUDED})

add_executable(ragexe ${RAGE_CLIENT_SOURCES})
target_include_directories(ragexe PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(ragexe PRIVATE -finput-charset=UTF-8 -fexec-charset=CP949 -fno-strict-aliasing -fwrapv
    -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers)
target_compile_definitions(ragexe PRIVATE _DEFAULT_SOURCE _GNU_SOURCE)
target_link_libraries(ragexe PRIVATE SDL3::SDL3 Freetype::Freetype Fontconfig::Fontconfig Threads::Threads
    rage_lua rage_png rage_zlib rage_gif rage_json rage_stb m ${CMAKE_DL_LIBS})
