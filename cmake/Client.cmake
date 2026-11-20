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
# The client's window is never shown (scene_app_hidden.c), so the GPU device makes no swapchain.
set(RAGE_SCENE_HARNESS tests/scenes/scene_clock.c tests/scenes/scene_net.c tests/scenes/scene_thread.c
    tests/scenes/scene_settings.c tests/scenes/scene_rand.c tests/scenes/scene_driver.c tests/scenes/scene_app_hidden.c)
set(RAGE_SCENE_REPLACED "/src/(platform/(net|clock|thread|settings)_posix|core/(drive_state|app_sdl))[.]c$")
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
if(APPLE)
    # ragexe.app; put it in the game folder, next to DATA.INI and the GRF files.
    set_target_properties(ragexe PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_GUI_IDENTIFIER local.ragexe.client
        MACOSX_BUNDLE_BUNDLE_NAME ragexe)
endif()

if(BUILD_TESTING AND NOT ANDROID)
    # The GPU backend on this machine's GPU (tests/CMakeLists.txt declares it for Windows).
    add_executable(test_gfx_gpu tests/test_gfx_gpu.c src/render/gfx_gpu.c src/render/dirty_rects.c
        src/render/frame_ring.c src/render/render_thread.c src/render/quads.c src/render/texel_convert.c
        src/render/draw_modes.c src/render/shadow_fit.c
        src/render/shadow_fit_gpu.c src/render/static_world.c src/render/world_layout.c src/render/skin_layout.c
        src/platform/clock_posix.c src/platform/thread_posix.c)
    target_include_directories(test_gfx_gpu PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(test_gfx_gpu PRIVATE _DEFAULT_SOURCE)
    target_link_libraries(test_gfx_gpu PRIVATE SDL3::SDL3 Threads::Threads m)
    rage_enable_assertions(test_gfx_gpu)
    add_test(NAME gfx_gpu COMMAND test_gfx_gpu 1)
    add_test(NAME gfx_gpu_msaa COMMAND test_gfx_gpu 4)
    add_test(NAME gfx_gpu_present COMMAND test_gfx_gpu 1)
    # The client's quads as instances against the CPU's (tests/CMakeLists.txt says more).
    add_executable(test_quad_instances tests/test_quad_instances.c tests/fixtures/quad_reference.c
        tests/picture_budget.c src/render/gfx_gpu.c src/render/dirty_rects.c src/render/frame_ring.c
        src/render/render_thread.c src/render/quads.c
        src/render/texel_convert.c src/render/draw_modes.c src/render/shadow_fit.c src/render/shadow_fit_gpu.c
        src/render/static_world.c src/render/world_layout.c src/render/skin_layout.c src/platform/clock_posix.c
        src/platform/thread_posix.c
        src/actor/sprite_quad.c src/effects/particle.c src/effects/sprite_particle.c
        src/effects/rag/effect_prim_draw.c src/world/ground_draw.c src/render/rp_project.c src/render/rp_screen.c
        src/render/math3d.c)
    target_include_directories(test_quad_instances PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)
    target_compile_definitions(test_quad_instances PRIVATE _DEFAULT_SOURCE)
    target_link_libraries(test_quad_instances PRIVATE SDL3::SDL3 Threads::Threads m)
    rage_enable_assertions(test_quad_instances)
    add_test(NAME quad_instances COMMAND test_quad_instances 1 20261112 300000 30)
    add_test(NAME quad_instances_msaa COMMAND test_quad_instances 4 20261113 0 30)
    # The shadow fit's compute passes against render/shadow_fit.c (tests/CMakeLists.txt says more).
    add_executable(test_shadow_fit_gpu tests/test_shadow_fit_gpu.c tests/shadow_fit_scenes.c
        src/render/shadow_fit.c src/render/shadow_fit_gpu.c)
    target_include_directories(test_shadow_fit_gpu PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)
    target_link_libraries(test_shadow_fit_gpu PRIVATE SDL3::SDL3 m)
    rage_enable_assertions(test_shadow_fit_gpu)
    add_test(NAME shadow_fit_gpu COMMAND test_shadow_fit_gpu)
    # The link check of the library table (tools/libraries.txt) on the client's objects, through nm.
    add_test(NAME library_links COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/depmap/check_links.py
        --nm ${CMAKE_BINARY_DIR}/CMakeFiles/ragexe.dir --strict)
    # gfx_gpu needs a GPU device, so it keeps the platform's video driver, in a hidden window, and
    # is skipped without one. gfx_gpu_present runs under the offscreen video driver, which shows its
    # window nowhere, so that the window is claimed and presented into where that driver gives
    # windows a swapchain (Mesa's Vulkan drivers do).
    set_tests_properties(gfx_gpu gfx_gpu_msaa PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60
        ENVIRONMENT SDL_AUDIO_DRIVER=dummy)
    set_tests_properties(gfx_gpu_present PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60
        ENVIRONMENT "SDL_AUDIO_DRIVER=dummy;SDL_VIDEO_DRIVER=offscreen")
    set_tests_properties(quad_instances quad_instances_msaa PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600
        ENVIRONMENT SDL_AUDIO_DRIVER=dummy)
    set_tests_properties(shadow_fit_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT SDL_AUDIO_DRIVER=dummy)
    set_tests_properties(library_links PROPERTIES ENVIRONMENT "${RAGE_TEST_ENVIRONMENT}")

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
        # run.py gives the client SDL's dummy audio driver; the harness picks its own video driver.
        set_tests_properties(scenes PROPERTIES TIMEOUT 1800 LABELS client-files)
        # Granny's actors skinned by the device against the CPU code they replaced, on the game
        # folder's converted models (tests/CMakeLists.txt says more).
        add_executable(test_skin_instances tests/test_skin_instances.c tests/fixtures/granny_reference.c
            tests/picture_budget.c src/render/gfx_gpu.c src/render/dirty_rects.c src/render/frame_ring.c
            src/render/render_thread.c src/render/quads.c src/render/texel_convert.c src/render/draw_modes.c
            src/render/shadow_fit.c src/render/shadow_fit_gpu.c
            src/render/static_world.c src/render/world_layout.c src/render/skin_layout.c src/render/rp_project.c
            src/render/rp_screen.c src/render/math3d.c src/resources/granny_gltf.c src/resources/granny_mesh.c
            src/platform/clock_posix.c src/platform/thread_posix.c)
        target_include_directories(test_skin_instances PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)
        target_compile_definitions(test_skin_instances PRIVATE _DEFAULT_SOURCE)
        target_link_libraries(test_skin_instances PRIVATE SDL3::SDL3 rage_cgltf rage_stb rage_zlib Threads::Threads m)
        rage_enable_assertions(test_skin_instances)
        add_test(NAME skin_instances COMMAND test_skin_instances 1 ${RAGE_SCENE_GAME_DIR}/data/model/gltf 20261113 20 20)
        add_test(NAME skin_instances_msaa COMMAND test_skin_instances 4 ${RAGE_SCENE_GAME_DIR}/data/model/gltf
            20261114 4 20)
        set_tests_properties(skin_instances skin_instances_msaa PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600
            LABELS client-files ENVIRONMENT SDL_AUDIO_DRIVER=dummy)
    endif()
endif()
