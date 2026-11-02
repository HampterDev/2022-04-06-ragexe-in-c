if(WIN32)
    message(FATAL_ERROR "The Windows client builds with build.bat")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/SDL3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Freetype.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/EffectData.cmake)
find_package(Threads REQUIRED)
# Android has neither Fontconfig nor an iconv with the Windows code pages: there the client scans the
# font folders (src/render/font_match_scan.c) and converts with its own tables
# (src/platform/code_page_iconv.c). Elsewhere these options try the same.
option(RAGE_FONT_SCAN "Find fonts by scanning the font folders instead of through Fontconfig" ${ANDROID})
option(RAGE_OWN_ICONV "Convert code pages with the client's tables of the Windows code pages" ${ANDROID})
set(RAGE_CLIENT_LIBRARIES SDL3::SDL3 Freetype::Freetype Threads::Threads)
if(NOT RAGE_FONT_SCAN)
    find_package(Fontconfig REQUIRED)
    list(APPEND RAGE_CLIENT_LIBRARIES Fontconfig::Fontconfig)
endif()
if(NOT RAGE_OWN_ICONV)
    find_package(Iconv REQUIRED)
    list(APPEND RAGE_CLIENT_LIBRARIES Iconv::Iconv)
endif()

file(GLOB_RECURSE RAGE_CLIENT_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.c)
file(STRINGS ${PROJECT_SOURCE_DIR}/tools/posix_excluded.txt RAGE_CLIENT_EXCLUDED REGEX "^[^#]")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tools/posix_excluded.txt)
list(TRANSFORM RAGE_CLIENT_EXCLUDED PREPEND ${PROJECT_SOURCE_DIR}/)
list(REMOVE_ITEM RAGE_CLIENT_SOURCES ${RAGE_CLIENT_EXCLUDED})

# ragexe_scenes is the client with the clock, the sockets, the threads, the settings store and the per-frame
# driver hook replaced, driven by a Lua scene script (tests/scenes/README.md).
set(RAGE_SCENE_HARNESS tests/scenes/scene_clock.c tests/scenes/scene_net.c tests/scenes/scene_thread.c
    tests/scenes/scene_settings.c tests/scenes/scene_rand.c tests/scenes/scene_driver.c)
set(RAGE_SCENE_REPLACED "/src/(platform/(net|clock|thread|settings)_posix|core/drive_state)[.]c$")
if(ANDROID)
    # The client is libmain.so, which SDL's activity loads and calls; the scene harness stays on desktops.
    set(RAGE_CLIENT_TARGETS ragexe)
else()
    set(RAGE_CLIENT_TARGETS ragexe ragexe_scenes)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(RAGE_SCENE_SOURCES ${RAGE_CLIENT_SOURCES})
    list(FILTER RAGE_SCENE_SOURCES EXCLUDE REGEX ${RAGE_SCENE_REPLACED})
    add_executable(ragexe ${RAGE_CLIENT_SOURCES})
    add_executable(ragexe_scenes ${RAGE_SCENE_SOURCES} ${RAGE_SCENE_HARNESS})
    foreach(target IN LISTS RAGE_CLIENT_TARGETS)
        target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/src)
        target_compile_options(${target} PRIVATE -finput-charset=UTF-8 -fexec-charset=CP949)
    endforeach()
else()
    # Other compilers only emit UTF-8 literals: compile a mirror of src/ whose narrow literals are CP949 escapes.
    set(RAGE_CP949_ROOT ${CMAKE_BINARY_DIR}/cp949)
    file(GLOB_RECURSE RAGE_CLIENT_HEADERS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.def)
    file(GLOB_RECURSE RAGE_CLIENT_ALL_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.c)
    set(RAGE_CLIENT_MIRROR ${RAGE_CLIENT_ALL_SOURCES} ${RAGE_CLIENT_HEADERS})
    list(TRANSFORM RAGE_CLIENT_MIRROR REPLACE "^${PROJECT_SOURCE_DIR}/src/" "${RAGE_CP949_ROOT}/src/")
    list(TRANSFORM RAGE_CLIENT_SOURCES REPLACE "^${PROJECT_SOURCE_DIR}/src/" "${RAGE_CP949_ROOT}/src/")
    file(MAKE_DIRECTORY ${RAGE_CP949_ROOT})
    if(NOT EXISTS ${RAGE_CP949_ROOT}/vendor)
        file(CREATE_LINK ${PROJECT_SOURCE_DIR}/vendor ${RAGE_CP949_ROOT}/vendor SYMBOLIC RESULT link_result)
        if(link_result AND CMAKE_HOST_WIN32)
            # Without the right to make symbolic links Windows still makes directory junctions.
            file(TO_NATIVE_PATH ${RAGE_CP949_ROOT}/vendor link_path)
            file(TO_NATIVE_PATH ${PROJECT_SOURCE_DIR}/vendor vendor_path)
            execute_process(COMMAND cmd /c mklink /J ${link_path} ${vendor_path} OUTPUT_QUIET
                RESULT_VARIABLE link_result)
        endif()
        if(link_result)
            message(FATAL_ERROR "Cannot link ${RAGE_CP949_ROOT}/vendor to ${PROJECT_SOURCE_DIR}/vendor")
        endif()
    endif()
    add_custom_command(OUTPUT ${RAGE_CP949_ROOT}/src.stamp
        BYPRODUCTS ${RAGE_CLIENT_MIRROR}
        COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/source_encoding.py --tree
            ${PROJECT_SOURCE_DIR}/src ${RAGE_CP949_ROOT}/src --stamp ${RAGE_CP949_ROOT}/src.stamp
        DEPENDS ${RAGE_CLIENT_ALL_SOURCES} ${RAGE_CLIENT_HEADERS} ${PROJECT_SOURCE_DIR}/tools/source_encoding.py
        COMMENT "Encoding client literals as CP949"
        VERBATIM)
    add_custom_target(rage_client_cp949 DEPENDS ${RAGE_CP949_ROOT}/src.stamp)
    set(RAGE_SCENE_SOURCES ${RAGE_CLIENT_SOURCES})
    list(FILTER RAGE_SCENE_SOURCES EXCLUDE REGEX ${RAGE_SCENE_REPLACED})
    if(ANDROID)
        add_library(ragexe SHARED ${RAGE_CLIENT_SOURCES})
        set_target_properties(ragexe PROPERTIES OUTPUT_NAME main)
    else()
        add_executable(ragexe ${RAGE_CLIENT_SOURCES})
        add_executable(ragexe_scenes ${RAGE_SCENE_SOURCES} ${RAGE_SCENE_HARNESS})
    endif()
    foreach(target IN LISTS RAGE_CLIENT_TARGETS)
        add_dependencies(${target} rage_client_cp949)
        target_include_directories(${target} PRIVATE ${RAGE_CP949_ROOT}/src)
    endforeach()
endif()
foreach(target IN LISTS RAGE_CLIENT_TARGETS)
    target_sources(${target} PRIVATE ${RAGE_EFFECT_DATA_C})
    # The client formats with message-table strings, so formats are not literals (an error in the NDK's flags).
    target_compile_options(${target} PRIVATE -fno-strict-aliasing -fwrapv
        -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers -Wno-error=format-security)
    target_compile_definitions(${target} PRIVATE _DEFAULT_SOURCE _GNU_SOURCE)
    target_link_libraries(${target} PRIVATE ${RAGE_CLIENT_LIBRARIES} rage_lua rage_png rage_zlib rage_json
        rage_stb rage_cgltf rage_dr_mp3 rage_crijndael m ${CMAKE_DL_LIBS})
    if(RAGE_FONT_SCAN)
        target_compile_definitions(${target} PRIVATE RAGE_FONT_SCAN)
    endif()
    if(RAGE_OWN_ICONV)
        target_compile_definitions(${target} PRIVATE RAGE_OWN_ICONV)
    endif()
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
endforeach()
if(TARGET ragexe_scenes)
    target_include_directories(ragexe_scenes PRIVATE ${PROJECT_SOURCE_DIR}/tests/scenes ${PROJECT_SOURCE_DIR}/vendor/stb)
endif()
option(RAGE_GLES "Draw through OpenGL ES 3.0 instead of desktop OpenGL (always on for Android)" OFF)
if(RAGE_GLES)
    foreach(target IN LISTS RAGE_CLIENT_TARGETS)
        target_compile_definitions(${target} PRIVATE RAGE_GLES)
    endforeach()
endif()
if(APPLE)
    # ragexe.app; put it in the game folder, next to DATA.INI and the GRF files.
    set_target_properties(ragexe PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_GUI_IDENTIFIER local.ragexe.client
        MACOSX_BUNDLE_BUNDLE_NAME ragexe)
endif()

if(BUILD_TESTING AND NOT ANDROID)
    add_executable(test_gfx_gl tests/test_gfx_gl.c src/render/gfx_gl.c src/render/gfx_batch.c src/render/display_devices.c
        src/platform/thread_posix.c)
    target_include_directories(test_gfx_gl PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(test_gfx_gl PRIVATE _DEFAULT_SOURCE)
    target_link_libraries(test_gfx_gl PRIVATE SDL3::SDL3 m)
    rage_enable_assertions(test_gfx_gl)
    add_test(NAME gfx_gl COMMAND test_gfx_gl)
    # The link check of the library table (tools/libraries.txt) on the client's objects, through nm.
    add_test(NAME library_links COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/depmap/check_links.py
        --nm ${CMAKE_BINARY_DIR}/CMakeFiles/ragexe.dir --strict)
    set_tests_properties(gfx_gl PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)

    # The scene captures need game data; with RAGE_SCENE_GOLDEN they are compared with golden images.
    set(RAGE_SCENE_GAME_DIR "" CACHE PATH "Game directory the scene tests run in (tests/scenes/README.md)")
    set(RAGE_SCENE_GOLDEN "" CACHE PATH "Golden images for the scene tests")
    if(RAGE_SCENE_GAME_DIR)
        set(RAGE_SCENE_COMPARE)
        if(RAGE_SCENE_GOLDEN)
            set(RAGE_SCENE_COMPARE --golden ${RAGE_SCENE_GOLDEN})
        endif()
        add_test(NAME scenes COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tests/scenes/run.py
            --exe $<TARGET_FILE:ragexe_scenes> --game ${RAGE_SCENE_GAME_DIR} --out ${CMAKE_BINARY_DIR}/scenes
            ${RAGE_SCENE_COMPARE})
        set_tests_properties(scenes PROPERTIES TIMEOUT 1800)
    endif()
endif()
