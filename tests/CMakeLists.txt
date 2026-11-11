# The test suite: every test, declared once for every platform (AGENTS.md, "Building and testing";
# docs/portability.md, "Build and test"). On Windows tests\build.bat configures, builds and runs it
# with Ninja and CTest; the oracle tests are a 32-bit configuration of their own (tests/oracle).
#
# Every test runs headless and silent: CTest gives it SDL's dummy video and audio drivers
# (RAGE_TEST_ENVIRONMENT in the top CMakeLists.txt), and on Windows the client libraries below
# answer the client's message boxes on stderr (headless.h). A test that needs a real window is
# labelled `interactive`.

# Assertions stay on in every configuration, in the tests and in the client code they test.
foreach(language C CXX)
    foreach(config RELEASE RELWITHDEBINFO MINSIZEREL)
        string(REGEX REPLACE "[-/]DNDEBUG" "" CMAKE_${language}_FLAGS_${config}
            "${CMAKE_${language}_FLAGS_${config}}")
    endforeach()
endforeach()
if(MSVC)
    # In every configuration as in an optimized one: functions and data in their own sections,
    # identical string literals pooled, and the linker drops what nothing uses (some tests compile
    # whole client files and stub only what they reach).
    add_compile_options(/Gy /Gw /GF)
    add_link_options(/OPT:REF)
    foreach(config DEBUG RELWITHDEBINFO)
        string(REGEX REPLACE "/INCREMENTAL( |$)" "/INCREMENTAL:NO\\1" CMAKE_EXE_LINKER_FLAGS_${config}
            "${CMAKE_EXE_LINKER_FLAGS_${config}}")
    endforeach()
endif()
include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/vendor/lua-5.1.4/src
    ${PROJECT_SOURCE_DIR}/vendor/zlib-1.2.11 ${PROJECT_SOURCE_DIR}/vendor/libpng-1.6.36
    ${PROJECT_SOURCE_DIR}/vendor/yyjson-0.13.0)
if(WIN32)
    # What the Windows tests link beyond CMake's default Windows libraries; an import library adds
    # nothing to a program that does not call into it.
    set(RAGE_TEST_SYSTEM_LIBRARIES winmm ws2_32 imm32 gdiplus dbghelp)
endif()

# ---- The client's files ----
# Client files the tests link are compiled once, into one static library per library of
# tools/libraries.txt (rage_client_base, rage_client_platform, ...). A test names them with CLIENT
# and links the libraries that hold them, highest first; the linker takes from each library only
# the objects the test uses. A test that compiles a client file with its own defines or a fixture
# compiles its own copy (OBJECTS, or its own sources). Each file keeps the warning level the tests
# have held it to: /W3 unless rage_client_warnings says otherwise.
execute_process(COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/depmap/libtable.py"
    --cmake "${CMAKE_CURRENT_BINARY_DIR}/libraries.cmake" RESULT_VARIABLE failed)
if(failed)
    message(FATAL_ERROR "tools/depmap/libtable.py could not assign the files under src to libraries")
endif()
include(${CMAKE_CURRENT_BINARY_DIR}/libraries.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/tools/libraries.txt ${PROJECT_SOURCE_DIR}/tools/depmap/libtable.py)

# rage_client_options(<file> <option>...): MSVC options of one client file, in the libraries only.
function(rage_client_options file)
    set_property(GLOBAL APPEND PROPERTY RAGE_CLIENT_OPTIONS_${file} ${ARGN})
endfunction()

# rage_client_warnings(<level> <file>...): the warning level of client files other than 3.
function(rage_client_warnings level)
    foreach(file IN LISTS ARGN)
        rage_client_options(${file} /W${level})
    endforeach()
endfunction()

# rage_link_client(<target> <file under src>...)
function(rage_link_client target)
    set(used)
    foreach(file IN LISTS ARGN)
        set(library "${RAGE_LIBRARY_OF_${file}}")
        if(NOT library)
            message(FATAL_ERROR "${target}: src/${file} is not a C file of a library of tools/libraries.txt")
        endif()
        string(REPLACE "-" "_" lib "rage_client_${library}")
        if(NOT TARGET ${lib})
            add_library(${lib} STATIC)
            set_target_properties(${lib} PROPERTIES RAGE_CLIENT_LIBRARY ON)
            if(MSVC)
                target_compile_options(${lib} PRIVATE /WX /we4013)
            endif()
            if(WIN32)
                target_link_libraries(${lib} PUBLIC ${RAGE_TEST_SYSTEM_LIBRARIES})
            else()
                target_link_libraries(${lib} PUBLIC m)
            endif()
            if(NOT WIN32 AND library STREQUAL "platform")
                find_package(Threads REQUIRED)
                target_compile_definitions(${lib} PRIVATE _POSIX_C_SOURCE=200809L)
                if(APPLE)
                    target_compile_definitions(${lib} PRIVATE _DARWIN_C_SOURCE)
                endif()
                target_link_libraries(${lib} PUBLIC Threads::Threads)
                find_package(Iconv REQUIRED)
                target_link_libraries(${lib} PUBLIC Iconv::Iconv)
            endif()
        endif()
        get_property(added GLOBAL PROPERTY RAGE_CLIENT_FILES)
        if(NOT file IN_LIST added)
            set_property(GLOBAL APPEND PROPERTY RAGE_CLIENT_FILES ${file})
            set(path ${PROJECT_SOURCE_DIR}/src/${file})
            target_sources(${lib} PRIVATE ${path})
            if(MSVC)
                get_property(options GLOBAL PROPERTY RAGE_CLIENT_OPTIONS_${file})
                if(NOT options MATCHES "/W[0-4]")
                    list(PREPEND options /W3)
                endif()
                # Only in the libraries: a test's own copy of the file keeps the test's options.
                set_property(SOURCE ${path} APPEND PROPERTY COMPILE_OPTIONS
                    "$<$<BOOL:$<TARGET_PROPERTY:RAGE_CLIENT_LIBRARY>>:${options}>")
            endif()
        endif()
        list(APPEND used ${lib})
    endforeach()
    foreach(library IN LISTS RAGE_LIBRARY_ORDER)
        string(REPLACE "-" "_" lib "rage_client_${library}")
        if(lib IN_LIST used)
            target_link_libraries(${target} PRIVATE ${lib})
            set_property(TARGET ${target} APPEND PROPERTY RAGE_TEST_LABELS ${library})
        endif()
    endforeach()
endfunction()

if(WIN32)
    rage_client_options(platform/win_utf8.c /FI${CMAKE_CURRENT_SOURCE_DIR}/headless.h)
    # Only the platform tests link it, which were never held to more than /W1: at /W3 its winsock
    # calls are deprecated (C4996).
    rage_client_warnings(1 platform/net_win32.c)
endif()

# The client files that a test held to /W1 or /W4 (the highest level any test compiled them
# at); before the first test, since a file takes its options when a test first names it.
rage_client_warnings(1 actor/map_type_table.c contents/body_layer_maps.c core/byte_array.c core/thread_text.c
    effects/ez2str_effect_mgr.c game/msg_broadcast.c game/packet_dispatch.c game/packet_dispatch2.c
    game/skill_cast_chat.c game/whisper_commands.c json/json_bson.c json/json_loader.c render/rp_pool.c
    session/character_name.c session/job_sounds.c ui/character/character_std.c ui/chat/edit_history.c
    ui/chat/markup_links.c ui/controls/combo_list.c ui/controls/edit3_lines.c ui/controls/edit_text.c
    ui/controls/listbox_ex.c ui/controls/listbox_id.c ui/controls/text_lines.c
    ui/controls/window_coloured_lines.c ui/controls/window_draw.c ui/controls/window_lines.c
    ui/controls/window_shorten.c ui/items/item_tag_buttons.c ui/items/item_tag_text.c
    ui/items/item_tags_chat.c ui/wndmgr/whisper_names.c ui/wndmgr/wndmgr_std.c web/name_cards.c
    world/mode_draw.c world/mode_lifetime.c)
rage_client_warnings(4 contents/byte_queue.c core/abstract_method.c core/cstr.c core/fatal.c core/hex.c
    core/numeric.c core/once_win32.c core/text_markup.c core/utf8.c effects/durations.c game/broadcast_text.c
    game/scream_text.c login/password_encode.c navi/navi_text_lines.c platform/code_page_win32.c
    render/text_runs.c std/list.c std/tree.c std/vec.c ui/chat/speaker_name.c ui/controls/item_id_tag.c)

# ---- Test programs ----
# Paths are relative to the repository root.
function(rage_test_paths out)
    set(paths)
    foreach(path IN LISTS ARGN)
        if(IS_ABSOLUTE "${path}")
            list(APPEND paths "${path}")
        else()
            list(APPEND paths "${PROJECT_SOURCE_DIR}/${path}")
        endif()
    endforeach()
    set(${out} ${paths} PARENT_SCOPE)
endfunction()

# The compile options shared by rage_test and rage_test_objects, from their parsed arguments:
# W1 or W4 (MSVC: the warning level, /W3 otherwise), NO_WX (MSVC: warnings are not errors), WE4013
# (MSVC: an undeclared function is an error), FORCE_INCLUDE <header>, DEFINES, OPTIONS, INCLUDES
# (directories), GENERATED (rage_test_generate sets whose directory the sources include from).
function(rage_test_compile target)
    if(MSVC)
        if(T_W1)
            target_compile_options(${target} PRIVATE /W1)
        elseif(T_W4)
            target_compile_options(${target} PRIVATE /W4)
        else()
            target_compile_options(${target} PRIVATE /W3)
        endif()
        if(NOT T_NO_WX)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        if(T_WE4013)
            target_compile_options(${target} PRIVATE /we4013)
        endif()
        if(T_FORCE_INCLUDE)
            target_compile_options(${target} PRIVATE "/FI${PROJECT_SOURCE_DIR}/${T_FORCE_INCLUDE}")
        endif()
    elseif(T_FORCE_INCLUDE)
        target_compile_options(${target} PRIVATE "SHELL:-include ${PROJECT_SOURCE_DIR}/${T_FORCE_INCLUDE}")
    endif()
    target_compile_definitions(${target} PRIVATE ${T_DEFINES})
    target_compile_options(${target} PRIVATE ${T_OPTIONS})
    rage_test_paths(includes ${T_INCLUDES})
    target_include_directories(${target} PRIVATE ${includes})
    foreach(set IN LISTS T_GENERATED)
        add_dependencies(${target} rage_generate_${set})
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/${set})
        get_property(files GLOBAL PROPERTY RAGE_GENERATED_FROM_${set})
        foreach(file IN LISTS files)
            if(RAGE_LIBRARY_OF_${file})
                set_property(TARGET ${target} APPEND PROPERTY RAGE_TEST_LABELS ${RAGE_LIBRARY_OF_${file}})
            endif()
        endforeach()
    endforeach()
endfunction()

# The client's .c files that a file under tests includes, itself or through the files under tests
# it includes (rage_test_label_sources); headers under src are not followed.
function(rage_test_included_client out path)
    get_property(scanned GLOBAL PROPERTY RAGE_TEST_SCANNED_${path})
    if(scanned)
        get_property(files GLOBAL PROPERTY RAGE_TEST_INCLUDES_${path})
        set(${out} ${files} PARENT_SCOPE)
        return()
    endif()
    set_property(GLOBAL PROPERTY RAGE_TEST_SCANNED_${path} ON)
    set(files)
    get_filename_component(dir ${path} DIRECTORY)
    file(STRINGS ${path} lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"]+[>\"]")
    foreach(line IN LISTS lines)
        string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"].*$" "\\1" name "${line}")
        get_filename_component(local ${dir}/${name} ABSOLUTE)
        file(RELATIVE_PATH rel ${PROJECT_SOURCE_DIR} ${local})
        if(rel MATCHES "^src/(.*[.]c)$")
            list(APPEND files ${CMAKE_MATCH_1})
        elseif(rel MATCHES "^tests/" AND EXISTS ${local})
            rage_test_included_client(included ${local})
            list(APPEND files ${included})
        elseif(name MATCHES "[.]c$" AND EXISTS ${PROJECT_SOURCE_DIR}/src/${name})
            list(APPEND files ${name})
        endif()
    endforeach()
    set_property(GLOBAL PROPERTY RAGE_TEST_INCLUDES_${path} ${files})
    set(${out} ${files} PARENT_SCOPE)
endfunction()

# Labels for a test: the libraries of tools/libraries.txt whose .c files it compiles, links
# (CLIENT) or includes, so that `ctest -L <library>` runs the tests a change in that library
# reaches. A header reaches the libraries of the files that include it.
function(rage_test_label_sources target)
    set(labels)
    foreach(path IN LISTS ARGN)
        file(RELATIVE_PATH rel ${PROJECT_SOURCE_DIR} ${path})
        set(files)
        if(rel MATCHES "^src/(.*[.]c)$")
            set(files ${CMAKE_MATCH_1})
        elseif(rel MATCHES "^tests/" AND EXISTS ${path})
            rage_test_included_client(files ${path})
        endif()
        foreach(file IN LISTS files)
            list(APPEND labels ${RAGE_LIBRARY_OF_${file}})
        endforeach()
    endforeach()
    if(labels)
        set_property(TARGET ${target} APPEND PROPERTY RAGE_TEST_LABELS ${labels})
    endif()
endfunction()

# rage_test_objects(<name> <source>... [W1|W4] [NO_WX] [WE4013] [FORCE_INCLUDE <header>]
#                   [DEFINES ...] [OPTIONS ...] [INCLUDES ...])
# Sources several tests share, or that one test compiles with options of their own, as an object
# library; a test links it with OBJECTS.
function(rage_test_objects name)
    cmake_parse_arguments(PARSE_ARGV 1 T "W1;W4;NO_WX;WE4013" "FORCE_INCLUDE"
        "DEFINES;OPTIONS;INCLUDES;GENERATED")
    rage_test_paths(sources ${T_UNPARSED_ARGUMENTS})
    add_library(${name} OBJECT ${sources})
    rage_test_compile(${name})
    rage_test_label_sources(${name} ${sources})
endfunction()

# rage_test(<name> [<source>...] [MAIN <file>] [CLIENT <file under src>...] [OBJECTS <name>...]
#           [LIBS <library>...] [ARGS <argument>...] [EXIT <code>] [LABELS ...] [ENVIRONMENT ...]
#           and the options of rage_test_compile)
# Builds test_<name> from MAIN (tests/test_<name>.c by default) and the sources, and runs it as the
# test <name> (rage_test_run).
function(rage_test name)
    cmake_parse_arguments(PARSE_ARGV 1 T "W1;W4;NO_WX;WE4013" "MAIN;EXIT;FORCE_INCLUDE"
        "CLIENT;OBJECTS;GENERATED;DEFINES;OPTIONS;INCLUDES;LIBS;ARGS;LABELS;ENVIRONMENT")
    if(NOT T_MAIN)
        set(T_MAIN tests/test_${name}.c)
    endif()
    set(target test_${name})
    rage_test_paths(sources ${T_MAIN} ${T_UNPARSED_ARGUMENTS})
    add_executable(${target} ${sources})
    rage_test_compile(${target})
    rage_test_label_sources(${target} ${sources})
    foreach(objects IN LISTS T_OBJECTS)
        target_link_libraries(${target} PRIVATE ${objects})
        get_target_property(labels ${objects} RAGE_TEST_LABELS)
        if(labels)
            set_property(TARGET ${target} APPEND PROPERTY RAGE_TEST_LABELS ${labels})
        endif()
    endforeach()
    rage_link_client(${target} ${T_CLIENT})
    target_link_libraries(${target} PRIVATE ${T_LIBS})
    if(WIN32)
        target_link_libraries(${target} PRIVATE ${RAGE_TEST_SYSTEM_LIBRARIES})
    else()
        target_link_libraries(${target} PRIVATE m)
    endif()
    set(run COMMAND ${target} ${T_ARGS} LABELS ${T_LABELS} ENVIRONMENT ${T_ENVIRONMENT})
    if(T_EXIT)
        list(APPEND run EXIT ${T_EXIT})
    endif()
    rage_test_run(${name} ${run})
endfunction()

# rage_test_run(<name> COMMAND <target or program> [<argument>...] [EXIT <code>] [LABELS ...]
#               [ENVIRONMENT <VAR=value>...])
# A test: the command, run in its own directory under work/ with RAGE_TEST_ENVIRONMENT (and
# ENVIRONMENT after it, which may override it). With EXIT the command must end with that code.
function(rage_test_run name)
    cmake_parse_arguments(PARSE_ARGV 1 T "" "EXIT" "COMMAND;LABELS;ENVIRONMENT")
    list(POP_FRONT T_COMMAND program)
    set(labels ${T_LABELS})
    if(TARGET ${program})
        get_target_property(target_labels ${program} RAGE_TEST_LABELS)
        if(target_labels)
            list(APPEND labels ${target_labels})
        endif()
        set(program $<TARGET_FILE:${program}>)
    endif()
    if(T_EXIT)
        add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DEXIT=${T_EXIT}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_exit.cmake ${program} ${T_COMMAND})
    else()
        add_test(NAME ${name} COMMAND ${program} ${T_COMMAND})
    endif()
    set(dir ${CMAKE_CURRENT_BINARY_DIR}/work/${name})
    file(MAKE_DIRECTORY ${dir})
    set(environment ${RAGE_TEST_ENVIRONMENT} ${T_ENVIRONMENT})
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${dir} ENVIRONMENT "${environment}")
    if(labels)
        list(REMOVE_DUPLICATES labels)
        set_tests_properties(${name} PROPERTIES LABELS "${labels}")
    endif()
endfunction()

# rage_test_generate(<name> OUTPUTS <file>... DEPENDS <file under src>... [ARGS <argument>...])
# Production code that tests/extract_<name>.py copies out of src at build time, into
# generated/<name> (a test reads it with GENERATED <name>). The script is called with the
# repository and that directory unless ARGS says otherwise.
function(rage_test_generate name)
    cmake_parse_arguments(PARSE_ARGV 1 G "" "SCRIPT" "OUTPUTS;DEPENDS;ARGS")
    set(dir ${CMAKE_CURRENT_BINARY_DIR}/generated/${name})
    if(NOT G_SCRIPT)
        set(G_SCRIPT tests/extract_${name}.py)
    endif()
    if(NOT G_ARGS)
        set(G_ARGS ${PROJECT_SOURCE_DIR} ${dir})
    endif()
    list(TRANSFORM G_OUTPUTS PREPEND ${dir}/)
    set(depends)
    foreach(file IN LISTS G_DEPENDS)
        if(IS_ABSOLUTE "${file}")
            list(APPEND depends "${file}")
        else()
            list(APPEND depends ${PROJECT_SOURCE_DIR}/src/${file})
            set_property(GLOBAL APPEND PROPERTY RAGE_GENERATED_FROM_${name} ${file})
        endif()
    endforeach()
    file(MAKE_DIRECTORY ${dir})
    add_custom_command(OUTPUT ${G_OUTPUTS}
        COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/${G_SCRIPT} ${G_ARGS}
        DEPENDS ${PROJECT_SOURCE_DIR}/${G_SCRIPT} ${depends}
        COMMENT "Extracting ${name} for the tests" VERBATIM)
    add_custom_target(rage_generate_${name} DEPENDS ${G_OUTPUTS})
endfunction()

# ---- Every platform ----
rage_test_run(source_encoding COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/test_source_encoding.py)
# The include check of the library table (tools/libraries.txt). The link check needs the client's
# objects: cmake/Client.cmake adds it as library_links, and on Windows `build.bat check` makes it.
rage_test_run(library_includes COMMAND Python3::Interpreter
    ${PROJECT_SOURCE_DIR}/tools/depmap/check_includes.py --strict)
rage_test_run(map_name_migration COMMAND Python3::Interpreter
    ${CMAKE_CURRENT_SOURCE_DIR}/test_map_name_migration.py)

rage_legacy_source(literal_source tests/test_literal_bytes.c)
rage_test(literal_bytes MAIN ${literal_source} W1 NO_WX)

# The platform modules of this platform and the text encoding they use (core/utf8.c); a platform
# module is one header with one file per implementation: <module>_win32.c on Windows,
# <module>_posix.c on Linux, macOS and Android.
if(WIN32)
    set(platform_files core/utf8.c platform/win_utf8.c)
    set(platform_kind win32)
else()
    set(platform_files core/utf8.c)
    set(platform_kind posix)
endif()
foreach(module clock code_page filesystem net settings sha256 thread)
    list(APPEND platform_files platform/${module}_${platform_kind}.c)
endforeach()
foreach(module clock filesystem thread net code_page sha256)
    rage_test(platform_${module} W1 NO_WX CLIENT ${platform_files})
endforeach()
rage_test(code_page_iconv W1 NO_WX src/platform/code_page_iconv.c src/platform/code_page_tables.c
    DEFINES RAGE_OWN_ICONV)
rage_test(numeric W4 CLIENT core/numeric.c)
rage_test(cstr W4 CLIENT core/cstr.c)
rage_test(math3d CLIENT render/math3d.c)
rage_test(frame_timer W1 NO_WX CLIENT contents/frame_timer.c)
rage_test(string W4)
rage_test(touch CLIENT core/touch.c)
rage_test(containers W4 CLIENT std/list.c std/tree.c std/vec.c)
rage_test(xml CLIENT core/memory.c core/xml.c std/string.c)
rage_test(lua_port W1 NO_WX LIBS rage_lua ARGS ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/lua51_win32.lub)
rage_test(jpeg_codec CLIENT resources/jpeg_codec.c resources/jpeg_storage.c LIBS rage_stb
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/jpeg_quadrants.jpg)
rage_test_generate(chat_room_title OUTPUTS chat_room_title.inc DEPENDS actor/actor_sendmsg.c)
rage_test(chat_room_title WE4013 GENERATED chat_room_title)
rage_test_generate(latest_achievements OUTPUTS latest_achievements.inc DEPENDS session/session_achievements.c)
rage_test(latest_achievements WE4013 CLIENT std/tree.c GENERATED latest_achievements LIBS rage_lua)

# ---- Windows, 32- and 64-bit ----
if(MSVC)
    rage_test_objects(test_crash_dump_crash_dump W4 src/platform/crash_dump.c FORCE_INCLUDE tests/crash_dump_fixture.h
        OPTIONS /Oy-)
    rage_test(crash_dump W4 OPTIONS /Oy- OBJECTS test_crash_dump_crash_dump)
endif()

# ---- Windows, 64-bit ----
# The suite tests\build.bat built and ran through one batch file per test until 2026-11-11: the
# same sources, defines and options, now compiled once (docs/custom-review.md, "One test suite in
# CMake"). Files the tests copy out of src at build time come from rage_test_generate.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    # The effect data as the client embeds it (RAGE_EFFECT_DATA_C).
    include(${PROJECT_SOURCE_DIR}/cmake/EffectData.cmake)
    add_custom_target(rage_effect_data DEPENDS ${RAGE_EFFECT_DATA_C})

    # The window manager's and the effects' files, which two of the scripts read all of.
    file(GLOB wndmgr_sources CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/ui/wndmgr/*.c)
    file(GLOB_RECURSE effect_sources CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/effects/*.c)
    rage_test_generate(airplane_window_slot OUTPUTS airplane_window_slot.inc DEPENDS ui/controls/window_base.c
        ui/quests/navigation_cmd.c ui/wndmgr/modal.c)
    rage_test_generate(basic_info_restore OUTPUTS basic_info_restore.inc DEPENDS
        ui/wndmgr/make_window_character.c)
    rage_test_generate(chat_layout_destroy OUTPUTS chat_layout_destroy.inc DEPENDS ui/chat/chat_window.c)
    rage_test_generate(com_lifecycle OUTPUTS com_lifecycle_functions.inc DEPENDS core/winmain.c
        core/app_win32.c ARGS ${PROJECT_SOURCE_DIR}/src/core/winmain.c
        ${PROJECT_SOURCE_DIR}/src/core/app_win32.c
        ${CMAKE_CURRENT_BINARY_DIR}/generated/com_lifecycle/com_lifecycle_functions.inc)
    rage_test_generate(effect_launch_duration OUTPUTS effect_launch_duration.inc DEPENDS
        effects/rag/rag_str_anim.c)
    rage_test_generate(effect_types OUTPUTS effect_type_tables.inc DEPENDS ${effect_sources})
    rage_test_generate(emote_timing OUTPUTS emote_timing.inc DEPENDS game/msg_misc.c)
    rage_test_generate(format_string_msg OUTPUTS format_string_msg.inc DEPENDS game/system_messages.c)
    rage_test_generate(headgear_names OUTPUTS headgear_names.inc DEPENDS session/sprite_names.c)
    rage_test_generate(item_restrictions OUTPUTS item_restrictions_under_test.c DEPENDS iteminfo/item_loader.c
        ARGS ${PROJECT_SOURCE_DIR}/src/iteminfo/item_loader.c
        ${CMAKE_CURRENT_BINARY_DIR}/generated/item_restrictions/item_restrictions_under_test.c)
    rage_test_generate(manager_alloc_failure OUTPUTS manager_alloc_failure.inc DEPENDS core/client_managers.c
        core/map_sky.c effects/ez2str_effect_mgr.c)
    rage_test_generate(map_enter_packet OUTPUTS map_enter_packet.inc DEPENDS login/login_states.c)
    rage_test_generate(mercenary_job OUTPUTS mercenary_job.inc DEPENDS actor/position.c session/predicates.c)
    rage_test_generate(meteor_storm_waves OUTPUTS meteor_storm_waves.inc DEPENDS
        effects/rag/rag_unreferenced.c)
    rage_test_generate(monster_skill_info OUTPUTS monster_skill_info.inc DEPENDS session/talk_tables.c)
    rage_test_generate(name_text OUTPUTS name_text_couple_name.inc name_text_cut_in.inc
        name_text_homun_default.inc name_text_homun_packet.inc name_text_homun_rename.inc
        name_text_map_helper.inc name_text_map_name.inc name_text_mapmove_name.inc name_text_pet_default.inc
        name_text_pet_named.inc name_text_pet_packet.inc name_text_pet_rename.inc name_text_server_list.inc
        name_text_state_line.inc name_text_style_save.inc name_text_zone_name.inc DEPENDS core/text_util.c
        game/actor_names.c game/companion_packets.c game/mode_connection.c game/packets_npc.c
        game/packets_pet.c game/packets_social.c game/packets_world.c game/state_messages_timed.c
        login/login_states.c login/zone.c ui/character/companion_info.c ui/items/item_collection_internal.h
        ui/items/pet_info.c ui/items/styling_shop.c)
    rage_test_generate(perf_overlay OUTPUTS perf_overlay.inc screenshot_case.inc screenshot_keys.inc DEPENDS
        diag/perf.c ui/wndmgr/wndmgr_keys.c)
    rage_test_generate(pet_accessory OUTPUTS pet_accessory.inc DEPENDS actor/game_actor.c game/packets_entry.c
        game/packets_entry_hp.c game/packets_pet.c session/pet.c)
    rage_test_generate(pet_actions OUTPUTS pet_actions.inc DEPENDS game/msg_misc.c game/packets_pet.c
        session/pet.c session/session_init.c)
    rage_test_generate(pet_portrait OUTPUTS pet_portrait.inc DEPENDS session/pet.c)
    rage_test_generate(private_airplane OUTPUTS private_airplane.inc DEPENDS game/private_airplane.c)
    rage_test_generate(screenshot_save OUTPUTS screenshot_save.inc DEPENDS world/mode_mgr.c ARGS
        ${PROJECT_SOURCE_DIR}/src/world/mode_mgr.c
        ${CMAKE_CURRENT_BINARY_DIR}/generated/screenshot_save/screenshot_save.inc)
    rage_test_generate(skill_tree_hints OUTPUTS skill_tree_hints_helpers.inc DEPENDS ui/character/skill_tree.c
        ui/character/skill_tree_points.c)
    rage_test_generate(startup_order OUTPUTS startup_order.inc DEPENDS core/app.c core/startup.c)
    rage_test_generate(status_wnd_info OUTPUTS status_wnd_info.inc DEPENDS ui/character/status_wnd.c)
    rage_test_generate(url_link OUTPUTS url_link_handlers.inc url_link_helper.inc DEPENDS
        ui/controls/text_viewer_links.c ui/controls/url_link.c ui/quests/tag_text_viewer.c)
    rage_test_generate(window_id_list OUTPUTS window_id_list.inc DEPENDS ui/wndmgr/window_slots.c)
    rage_test_generate(window_rows OUTPUTS window_rows.inc DEPENDS ${wndmgr_sources})

    rage_test_objects(rage_test_json_test_support tests/json_test_support.c)
    rage_test_objects(rage_test_quest_value_support tests/quest_value_support.c)
    rage_test_objects(rage_test_weather_state_support tests/weather_state_support.c)
    rage_test_objects(test_bitmap_png_bitmap_png src/resources/bitmap_png.c FORCE_INCLUDE
        tests/png_alloc_fixture.h)
    rage_test_objects(test_bitmap_png_bitmap_storage src/resources/bitmap_storage.c FORCE_INCLUDE
        tests/png_alloc_fixture.h)
    rage_test_objects(test_code_page_conversion_text_encoding W4 src/core/text_encoding.c FORCE_INCLUDE
        tests/encoding_fixture.h)
    rage_test_objects(test_crash_dialog_crash_dialog W4 src/ui/misc/crash_dialog.c FORCE_INCLUDE
        tests/crash_dialog_fixture.h)
    rage_test_objects(test_crash_report_crash_report src/diag/crash_report.c FORCE_INCLUDE
        tests/crash_report_fixture.h)
    rage_test_objects(test_font_cache_fonts src/render/fonts.c FORCE_INCLUDE tests/font_cache_fixture.h)
    rage_test_objects(test_mouse_mouse src/contents/mouse.c FORCE_INCLUDE tests/mouse_fixture.h)
    rage_test_objects(test_mouse_window_win32 src/platform/window_win32.c FORCE_INCLUDE tests/mouse_fixture.h)
    rage_test_objects(test_object_type_actor_object_types W4 src/actor/actor_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_character_object_types W4 src/ui/character/character_object_types.c
        DEFINES TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_chat_object_types W4 src/ui/chat/chat_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_controls_object_types W4 src/ui/controls/controls_object_types.c
        DEFINES TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_hotkey_object_types W4 src/ui/hotkeys/hotkey_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_items_object_types W4 src/ui/items/items_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_mail_object_types W4 src/ui/mail/mail_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_map_object_types W4 src/ui/map/map_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_object_type W4 src/core/object_type.c DEFINES TYPE_QUERY_FIXTURE
        FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_object_type_registry W4 src/core/object_type_registry.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_quests_object_types W4 src/ui/quests/quests_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_render_ui_object_types W4 src/ui/render/render_ui_object_types.c
        DEFINES TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_resource_object_types W4 src/resources/resource_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_shops_object_types W4 src/ui/shops/shops_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_skill_object_types W4 src/effects/skills/skill_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_social_object_types W4 src/ui/social/social_object_types.c DEFINES
        TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_object_type_worldmap_object_types W4 src/ui/worldmap/worldmap_object_types.c
        DEFINES TYPE_QUERY_FIXTURE FORCE_INCLUDE tests/type_query_fixture.h)
    rage_test_objects(test_text_consumers_ellipsize_text src/ui/controls/ellipsize_text.c FORCE_INCLUDE
        tests/encoding_fixture.h)
    rage_test_objects(test_text_consumers_text_encoding W4 src/core/text_encoding.c FORCE_INCLUDE
        tests/encoding_fixture.h)
    rage_test_objects(test_ui_drop_owner_2 WE4013 tests/ui_drop_owner.c DEFINES UI_DROP_OWNER=2)
    rage_test_objects(test_ui_drop_owner_4 WE4013 tests/ui_drop_owner.c DEFINES UI_DROP_OWNER=4)
    rage_test_objects(test_ui_drop_owner_8 WE4013 tests/ui_drop_owner.c DEFINES UI_DROP_OWNER=8)
    rage_test_objects(test_ui_drop_owner_9 WE4013 tests/ui_drop_owner.c DEFINES UI_DROP_OWNER=9)
    rage_test_objects(test_ui_drop_owner_10 WE4013 tests/ui_drop_owner.c DEFINES UI_DROP_OWNER=10)
    rage_test_objects(test_ui_invalidation_uiwindow src/ui/uiwindow.c FORCE_INCLUDE
        tests/ui_invalidation_fixture.h OPTIONS /Oy-)
    rage_test_objects(test_win_utf8_win_utf8 W4 src/platform/win_utf8.c FORCE_INCLUDE
        tests/win_utf8_fixture.h)
    rage_test_objects(rage_test_font_family W4 vendor/font_backend/font_family.cpp)

    rage_test(achievement_owner W1 WE4013 src/contents/achievement.c src/contents/achievement_lua.c
        src/contents/achievement_std.c src/contents/lua_data.c src/core/text_util.c src/std/std_comdats.c
        src/std/string.c src/std/container_memory.c src/std/vec.c src/std/tree.c
        src/platform/code_page_win32.c FORCE_INCLUDE tests/lua_data_manager_boundaries.h INCLUDES src/contents
        LIBS rage_lua)
    rage_test(actor_actions WE4013 CLIENT actor/action.c actor/head_extent.c)
    rage_test(actor_appearance CLIENT actor/appearance.c session/session_effect_state.c)
    rage_test(actor_effect_cells)
    rage_test(actor_helper_owners W1 WE4013 CLIENT actor/map_type_table.c skills/skill_action_registry.c
        std/tree.c std/vec.c)
    rage_test(actor_liveness CLIENT actor/clone_owner.c)
    rage_test(agency_packets W4)
    rage_test(agency_results W1)
    rage_test(ai_context CLIENT core/numeric.c core/utf8.c login/merc_ai.c login/merc_ai_cmds.c
        platform/clock_win32.c platform/win_utf8.c platform/window_win32.c std/tree.c
        ui/hotkeys/session_option_map.c LIBS rage_lua)
    rage_test(airplane_window_slot WE4013 CLIENT std/list.c ui/wndmgr/forget_window.c ui/wndmgr/window_table.c
        ui/wndmgr/wndmgr_handlers.c GENERATED airplane_window_slot window_rows)
    rage_test(animation_drawing W1 CLIENT core/fatal.c core/memory.c core/utf8.c fileio/file.c
        fileio/resource_path.c platform/clock_win32.c platform/code_page_win32.c platform/filesystem_win32.c
        std/vec.c ui/controls/window_draw.c)
    rage_test(animation_owner CLIENT core/utf8.c fileio/file.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c)
    rage_test(attack_records WE4013 tests/attack_records_session.c tests/attack_records_skill_fx.c CLIENT
        actor/actor_msg.c game/packets_act.c game/skill_notify.c platform/clock_win32.c)
    rage_test(attribute_consumers CLIENT game/cell_coordinates.c game/cells.c platform/clock_win32.c
        resources/attr_queries.c)
    rage_test(attribute_grid CLIENT core/utf8.c fileio/file.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c render/math3d.c resources/attr_queries.c)
    rage_test(balloon_text W1 WE4013 CLIENT core/cstr.c core/hex.c core/text_format.c core/text_util.c
        core/utf8.c platform/code_page_win32.c std/container_memory.c std/std_comdats.c std/string.c std/vec.c
        ui/controls/controls_std.c ui/controls/item_id_tag.c ui/controls/text_lines.c ui/std/ui_std_vector.c)
    rage_test(barter_managers WE4013 tests/barter_managers_unreached.c CLIENT contents/barter_item.c
        iteminfo/item_info.c std/container_memory.c std/list.c std/string.c std/tree.c std/vec.c
        web/barter_expanded.c web/barter_market.c)
    rage_test(barter_records WE4013 tests/barter_records_unreached.c CLIENT contents/barter_item.c game/drag.c
        iteminfo/item_info.c std/container_memory.c std/string.c std/vec.c)
    rage_test(basic_info_restore WE4013 GENERATED basic_info_restore)
    rage_test(bitmap_owner CLIENT core/cstr.c std/string.c LIBS rage_png)
    rage_test(bitmap_png OBJECTS test_bitmap_png_bitmap_png test_bitmap_png_bitmap_storage LIBS rage_png)
    rage_test(broadcast_packets W1 WE4013 CLIENT game/msg_broadcast.c session/character_name.c)
    rage_test(broadcast_text W4 CLIENT core/utf8.c game/broadcast_text.c)
    rage_test(button_drawing CLIENT platform/clock_win32.c std/container_memory.c std/string.c std/vec.c)
    rage_test(byte_lines W1 WE4013 CLIENT core/hex.c core/owned_text.c core/text_array.c core/text_format.c
        core/utf8.c platform/code_page_win32.c ui/controls/item_id_tag.c ui/controls/text_lines.c)
    rage_test(byte_queue W4 CLIENT contents/byte_queue.c)
    rage_test(c_text_streams CLIENT contents/guild_emblem_name.c core/text_tokens.c std/container_memory.c
        std/string.c std/string_vec.c std/vec.c)
    rage_test(camera_consumers)
    rage_test(camera_lifecycle CLIENT world/frustum.c world/view_lifecycle.c)
    rage_test(canvas_owner)
    rage_test(canvas_texture)
    rage_test(cell_context CLIENT game/cell_context.c)
    rage_test(character_draw W1 WE4013 CLIENT core/once_win32.c core/utf8.c platform/code_page_win32.c
        std/container_memory.c std/string.c std/tree.c std/vec.c ui/character/character_std.c)
    rage_test(character_model W1 WE4013 CLIENT core/once_win32.c std/container_memory.c std/list.c
        std/string.c std/tree.c std/vec.c ui/std/ui_std_list.c)
    rage_test(character_preview WE4013 CLIENT std/container_memory.c std/string.c std/vec.c
        ui/character/char_preview.c ui/character/select_char_storage.c)
    rage_test(chat_layout_save CLIENT core/owned_text.c session/chat_tabs.c GENERATED chat_layout_destroy)
    rage_test(chat_navigation CLIENT initializers/markup_tags.c iteminfo/item_info.c
        platform/code_page_win32.c std/container_memory.c std/std_comdats.c std/string.c std/string_vec.c
        std/vec.c ui/chat/navi_text.c ui/std/ui_std_vector.c)
    rage_test(chat_tabs)
    rage_test(chat_tabs_layout CLIENT core/owned_text.c platform/filesystem_win32.c session/chat_tabs.c
        session/chat_tabs_layout.c LIBS rage_lua)
    rage_test(chat_tabs_lua LIBS rage_lua)
    rage_test(client_operations MAIN tests/test_named_client_dispatch.c WE4013 CLIENT std/list.c)
    rage_test(code_page_conversion W4 tests/encoding_fixture.c CLIENT platform/code_page_win32.c OBJECTS
        test_code_page_conversion_text_encoding)
    rage_test(com_lifecycle OPTIONS /wd4101 /wd4505 /wd4189 GENERATED com_lifecycle)
    rage_test(container_headers tests/container_headers_peer.c CLIENT std/container_memory.c std/list.c
        std/string.c std/tree.c std/vec.c)
    rage_test(container_memory WE4013 tests/container_memory_mail_probe.c
        tests/container_memory_vector_probe.c tests/container_memory_ui_probe.c
        tests/container_memory_controls_probe.c CLIENT platform/clock_win32.c std/string.c std/vec.c)
    rage_test(content_managers W1 WE4013 src/core/map_sky.c src/contents/client_lua.c src/core/text_util.c
        src/std/std_comdats.c src/std/string.c src/std/container_memory.c src/std/vec.c src/std/string_vec.c
        src/std/tree.c src/platform/code_page_win32.c FORCE_INCLUDE tests/content_manager_allocations.h
        INCLUDES src/contents LIBS rage_lua)
    rage_test(cooldown_clock CLIENT session/skill_delay.c ui/cooldown_lists.c)
    rage_test(cooldown_geometry CLIENT ui/cooldown_timer.c ARGS
        ${PROJECT_SOURCE_DIR}/tests/fixtures/cooldown_geometry.bin)
    rage_test(cooldown_packets CLIENT ui/cooldown_timer.c)
    rage_test(cooldown_sheet CLIENT ui/cooldown_timer.c)
    rage_test(cooldown_storage CLIENT ui/cooldown_timer.c)
    rage_test(crash_dialog W4 OBJECTS test_crash_dialog_crash_dialog)
    rage_test(crash_report OBJECTS test_crash_report_crash_report)
    rage_test(display_devices)
    rage_test(draw_maps W1 WE4013 CLIENT contents/body_layer_maps.c core/once_win32.c std/container_memory.c
        std/string.c std/tree.c std/vec.c)
    rage_test(edit_history W1 CLIENT core/owned_text.c core/text_array.c core/utf8.c
        platform/code_page_win32.c std/string.c ui/chat/edit_history.c ui/controls/edit_text.c
        ui/wndmgr/whisper_names.c)
    rage_test(edit_navigation W1 CLIENT core/owned_text.c core/text_array.c platform/code_page_win32.c
        std/container_memory.c std/std_comdats.c std/string.c std/vec.c ui/std/ui_std_vector.c)
    rage_test(edit_text W1 CLIENT core/fatal.c core/memory.c core/utf8.c platform/code_page_win32.c
        std/string.c ui/controls/edit_text.c)
    rage_test(edit_text_render)
    rage_test(effect_data ${RAGE_EFFECT_DATA_C} CLIENT core/utf8.c fileio/resource_path.c
        platform/code_page_win32.c LIBS rage_json ARGS ${CMAKE_CURRENT_BINARY_DIR}/work/effect_data)
    rage_test(effect_destruction W1)
    rage_test(effect_durations W4 CLIENT effects/durations.c)
    rage_test(effect_factories W1 CLIENT std/container_memory.c std/list.c std/tree.c std/vec.c GENERATED
        effect_types)
    rage_test(effect_launch CLIENT actor/effect_launch.c)
    rage_test(effect_launch_duration WE4013 ${RAGE_EFFECT_DATA_C} CLIENT core/utf8.c effects/durations.c
        effects/effect_data.c fileio/resource_path.c platform/code_page_win32.c GENERATED
        effect_launch_duration LIBS rage_json)
    rage_test(effect_objects CLIENT effects/effect_objects.c)
    rage_test(effect_operations W1 CLIENT std/container_memory.c std/string.c std/string_vec.c std/vec.c)
    rage_test(effect_sound CLIENT effects/effect_sound.c)
    rage_test(effect_targets)
    rage_test(effect_trace W1 WE4013 src/effects/rag/rag_effect_process.c
        src/effects/rag/rag_effect_process_491.c src/effects/rag/rag_effect_process_800.c
        src/effects/rag/rag_skills_000_019.c src/effects/rag/rag_skills_020_039.c
        src/effects/rag/rag_skills_040_079.c src/effects/rag/rag_skills_080_119.c
        src/effects/rag/rag_skills_120_219.c src/effects/rag/rag_skills_220_259.c
        src/effects/rag/rag_skills_300_319.c src/effects/rag/rag_skills_320_339.c
        src/effects/rag/rag_skills_340_373.c src/effects/rag/rag_skills_374_419.c
        src/effects/rag/rag_skills_420_490.c src/effects/rag/rag_skills_491_559.c
        src/effects/rag/rag_skills_560_699.c src/effects/rag/rag_skills_700_799.c
        src/effects/rag/rag_skills_800_839.c src/effects/rag/rag_skills_840_879.c
        src/effects/rag/rag_skills_880_919.c src/effects/rag/rag_skills_920_2373.c src/effects/rag/rag_songs.c
        src/effects/rag/rag_msg_sprites.c src/effects/rag/rag_unreferenced.c
        src/effects/rag/rag_unreferenced_prims.c src/effects/rag/rag_process_moves.c
        src/effects/rag/rag_prim_launch.c src/effects/rag/effect_prim.c src/effects/rag/effect_prim_shared.c
        src/effects/rag/rag_effect.c src/effects/rag/rag_str_anim.c src/effects/rag/rag_str_playback.c
        src/effects/str_path.c src/effects/str_animation.c src/effects/durations.c
        src/effects/effect_objects.c src/effects/effect_sound.c src/actor/effect_launch.c src/actor/position.c
        src/effects/skills/ez2str_effect_files.c src/effects/effect_data.c src/effects/rag/rag_str_bindings.c
        src/fileio/resource_path.c src/platform/code_page_win32.c src/core/utf8.c ${RAGE_EFFECT_DATA_C}
        src/render/math3d.c src/render/rp_project.c src/session/session_effect_state.c src/std/list.c
        src/std/vec.c src/std/string.c src/std/container_memory.c FORCE_INCLUDE tests/effect_trace_rand.h
        OPTIONS /Oy- /GS /Gd LIBS rage_json)
    rage_test(emblem_cache src/session/session_state.c src/platform/filesystem_win32.c
        src/contents/guild_emblem_name.c src/std/string.c src/std/list.c src/std/container_memory.c
        src/std/vec.c src/std/tree.c src/platform/clock_win32.c src/core/client_path.c FORCE_INCLUDE
        tests/emblem_cache_boundaries.h)
    rage_test(emblem_download)
    rage_test(emblem_transport CLIENT core/cstr.c std/string.c LIBS rage_png)
    rage_test(emote_timing GENERATED emote_timing)
    rage_test(ez2str_manager W1 WE4013 CLIENT core/text_util.c effects/ez2str_effect_mgr.c json/json_bson.c
        json/json_loader.c json/json_tree.c platform/code_page_win32.c std/container_memory.c
        std/std_comdats.c std/string.c std/tree.c std/vec.c)
    rage_test(fatal W4 CLIENT core/abstract_method.c core/fatal.c)
    rage_test(file_io CLIENT core/utf8.c fileio/file.c fileio/file_mgr.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c platform/system_memory_win32.c)
    rage_test(fit_plain_text W1 CLIENT core/fatal.c core/memory.c core/utf8.c std/string.c)
    rage_test(font_cache CLIENT core/cstr.c OBJECTS test_font_cache_fonts)
    rage_test(font_cache_system CLIENT core/cstr.c platform/code_page_win32.c render/fonts.c OBJECTS
        rage_test_font_family)
    rage_test(font_family MAIN tests/test_font_family.cpp W4 vendor/font_backend/font_family.cpp DEFINES
        FONT_FAMILY_MOCK)
    rage_test(font_family_system MAIN tests/test_font_family.cpp W4 OBJECTS rage_test_font_family)
    rage_test(forget_window CLIENT ui/wndmgr/forget_window.c ui/wndmgr/window_table.c
        ui/wndmgr/wndmgr_handlers.c GENERATED window_rows)
    rage_test(format_string_msg WE4013 CLIENT core/memory.c core/utf8.c std/string.c GENERATED
        format_string_msg)
    rage_test(frustum CLIENT world/frustum.c)
    rage_test(game_context CLIENT game/context.c world/mode_context.c world/mode_dispatch.c)
    rage_test(game_mode_layout W1 WE4013 CLIENT core/once_win32.c core/utf8.c platform/clock_win32.c
        platform/win_utf8.c platform/window_win32.c std/container_memory.c std/list.c std/string.c std/tree.c
        std/vec.c ui/std/ui_std_list.c ui/wndmgr/wndmgr_std.c web/name_cards.c world/mode_dispatch.c
        world/mode_draw.c world/mode_lifetime.c)
    rage_test(gauge_sources tests/gauge_sources_support.c CLIENT ui/character/bar_graph.c
        ui/character/gauge_value.c ui/character/gauges.c ui/items/homun_bar.c)
    rage_test(gfx_batch CLIENT platform/thread_win32.c render/gfx_batch.c)
    rage_test(gm_name_packets W1 WE4013 CLIENT core/cstr.c platform/clock_win32.c)
    rage_test(granny_models MAIN tests/test_granny_model_storage.c tests/granny_model_geometry.c CLIENT
        core/fatal.c core/memory.c render/math3d.c std/vec.c)
    rage_test(granny_playback)
    rage_test(granny_vertices CLIENT render/math3d.c)
    rage_test(gravity_control tests/effect_base_stubs.c)
    rage_test(grf_archive_owner CLIENT core/cstr.c platform/system_memory_win32.c platform/thread_win32.c
        std/std_sort.c LIBS rage_zlib ARGS ${PROJECT_SOURCE_DIR}/tests/fixtures/grf_records.bin)
    rage_test(ground_item CLIENT core/once_win32.c iteminfo/item_info.c platform/clock_win32.c
        std/container_memory.c std/string.c std/vec.c)
    rage_test(ground_lifecycle CLIENT render/math3d.c resources/attr_queries.c)
    rage_test(ground_queries CLIENT effects/rag/prim_bounce.c render/math3d.c resources/attr_queries.c)
    rage_test(ground_water CLIENT core/utf8.c fileio/file.c fileio/resource_path.c platform/code_page_win32.c
        platform/filesystem_win32.c resources/gnd.c std/container_memory.c std/string.c std/vec.c
        world/ground_lifecycle.c world/ground_water.c OBJECTS rage_test_weather_state_support)
    rage_test(guild_quest_owners W1 WE4013 src/std/vec.c src/contents/guild_mgr.c
        src/contents/guild_emblem_req.c src/contents/guild_emblem_download.c src/contents/renew_quest.c
        src/contents/renew_quest_maps.c src/contents/quest_value.c src/std/std_comdats.c src/std/string.c
        src/std/container_memory.c src/fileio/res_maps.c src/ui/std/ui_std_vector.c src/std/string_vec.c
        src/std/tree.c FORCE_INCLUDE tests/guild_quest_owner_boundaries.h)
    rage_test(guild_state tests/guild_state_support.c CLIENT contents/guild_info.c contents/guild_lists.c
        platform/code_page_win32.c std/container_memory.c std/list.c std/string.c std/tree.c std/vec.c
        ui/social/guild_containers.c ui/social/guild_requests.c)
    rage_test(headgear_names CLIENT std/string.c GENERATED headgear_names LIBS rage_lua)
    rage_test(heal_colours WE4013 tests/effect_base_stubs.c CLIENT std/vec.c)
    rage_test(hex W4 CLIENT core/hex.c)
    rage_test(imf_owner CLIENT actor/layer_order.c core/utf8.c fileio/file.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c)
    rage_test(incoming_whisper CLIENT initializers/markup_tags.c platform/clock_win32.c std/container_memory.c
        std/string.c std/vec.c)
    rage_test(inventory_match CLIENT iteminfo/item_info.c session/inventory_match.c std/container_memory.c
        std/list.c std/string.c std/vec.c)
    rage_test(item_collection_toggle tests/item_collection_toggle_unreached.c CLIENT core/memory.c
        std/container_memory.c std/string.c)
    rage_test(item_definitions tests/item_info_support.c CLIENT core/utf8.c platform/win_utf8.c
        platform/window_win32.c std/vec.c OBJECTS rage_test_weather_state_support LIBS rage_lua)
    rage_test(item_info CLIENT iteminfo/item_info.c iteminfo/item_list.c std/container_memory.c std/list.c
        std/string.c std/vec.c)
    rage_test(item_links CLIENT iteminfo/item_info.c iteminfo/item_link.c std/container_memory.c std/string.c
        std/vec.c)
    rage_test(item_names CLIENT core/hex.c core/owned_text.c core/text_format.c core/utf8.c
        platform/code_page_win32.c std/container_memory.c std/string.c std/vec.c)
    rage_test(item_restrictions
        ${CMAKE_CURRENT_BINARY_DIR}/generated/item_restrictions/item_restrictions_under_test.c CLIENT
        std/vec.c GENERATED item_restrictions)
    rage_test(item_tag_list W4)
    rage_test(item_tags W1 CLIENT core/cstr.c std/container_memory.c std/std_comdats.c std/string.c
        ui/items/item_tag_text.c ui/items/item_tags_chat.c ui/std/ui_std_vector.c)
    rage_test(item_use WE4013 CLIENT game/msg_items.c platform/clock_win32.c session/session_effect_state.c)
    rage_test(job_hit_sounds W1 CLIENT session/job_sounds.c)
    rage_test(jpeg_bitmap)
    rage_test(jpeg_capture)
    rage_test(json_text CLIENT json/json_tree.c LIBS rage_json)
    rage_test(kafra_services LIBS rage_lua)
    rage_test(language_input CLIENT core/utf8.c platform/code_page_win32.c platform/win_utf8.c)
    rage_test(line_layout CLIENT core/text_format.c core/text_markup.c core/utf8.c initializers/markup_tags.c
        iteminfo/item_info.c platform/code_page_win32.c std/container_memory.c std/std_comdats.c std/string.c
        std/vec.c ui/chat/message_tags.c ui/std/ui_std_vector.c)
    rage_test(link_buttons CLIENT core/utf8.c initializers/markup_tags.c iteminfo/item_info.c
        platform/win_utf8.c platform/window_win32.c std/container_memory.c std/std_comdats.c std/string.c
        std/vec.c ui/chat/navi_text.c ui/std/ui_std_vector.c)
    rage_test(list_operations W1 CLIENT std/container_memory.c std/string.c std/tree.c std/vec.c
        ui/controls/combo_list.c ui/controls/controls_std.c ui/controls/listbox_ex.c ui/controls/listbox_id.c)
    rage_test(local_account CLIENT contents/account_settings.c contents/account_settings_json.c
        contents/lua_calls.c contents/settings_json.c contents/user_hotkey_apply.c contents/user_hotkeys.c
        contents/whisper_block.c core/owned_text.c core/text_array.c core/text_encoding.c core/utf8.c
        json/json_tree.c json/service_json.c platform/code_page_win32.c platform/sha256_win32.c
        platform/system_win32.c platform/win_utf8.c platform/window_win32.c session/chat_macro_json.c
        session/session_chat_macros.c LIBS rage_json rage_lua)
    rage_test(local_character CLIENT contents/character_settings.c contents/settings_file.c core/owned_text.c
        json/json_tree.c json/service_json.c platform/filesystem_win32.c platform/sha256_win32.c
        platform/system_win32.c platform/win_utf8.c LIBS rage_json)
    rage_test(local_shop CLIENT contents/mcstore_backup.c contents/mcstore_json.c contents/settings_json.c
        core/owned_text.c core/text_encoding.c iteminfo/item_info.c json/json_tree.c json/service_json.c
        platform/code_page_win32.c platform/sha256_win32.c std/container_memory.c std/string.c std/vec.c LIBS
        rage_json)
    rage_test(login_conn_entry WE4013 CLIENT core/memory.c core/xml.c login/connections.c login/login_std.c
        std/container_memory.c std/string.c std/vec.c)
    rage_test(login_pin_state WE4013 CLIENT core/memory.c login/char_results.c login/login_dispatch.c
        platform/clock_win32.c)
    rage_test(lua_boundaries CLIENT core/utf8.c fileio/file.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c platform/win_utf8.c platform/window_win32.c
        std/container_memory.c std/string.c std/vec.c LIBS rage_lua)
    rage_test(lua_calls CLIENT core/utf8.c platform/win_utf8.c platform/window_win32.c LIBS rage_lua)
    rage_test(lua_calls_disabled MAIN tests/test_lua_calls.c LIBS rage_lua)
    rage_test(lua_data_managers W1 WE4013 src/contents/champion_monster.c src/contents/check_attendance.c
        src/contents/pet_auto_feed.c src/contents/private_airplane_mgr.c src/contents/lua_data.c
        src/session/predicates.c src/fileio/res_maps.c src/core/text_util.c src/std/std_comdats.c
        src/std/string.c src/std/container_memory.c src/std/vec.c src/std/tree.c
        src/platform/code_page_win32.c FORCE_INCLUDE tests/lua_data_manager_boundaries.h INCLUDES src/contents
        LIBS rage_lua)
    rage_test(manager_alloc_failure WE4013 CLIENT std/container_memory.c std/string.c std/tree.c std/vec.c
        GENERATED manager_alloc_failure)
    rage_test(map_enter_packet WE4013 GENERATED map_enter_packet)
    rage_test(map_entry CLIENT game/map_entry.c)
    rage_test(map_info_owner LIBS rage_lua)
    rage_test(map_nodes tests/map_nodes_support.c CLIENT actor/actor_values.c contents/guild_lists.c
        core/once_win32.c platform/code_page_win32.c skills/player_skill_bundle.c skills/skill_info.c
        std/container_memory.c std/list.c std/string.c std/tree.c std/vec.c ui/hotkeys/session_option_map.c
        ui/social/guild_containers.c ui/std/ui_std_list.c world/world_std.c)
    rage_test(markup_helpers W1 CLIENT ui/chat/markup_links.c ui/chat/message_tags.c ui/chat/navi_text.c)
    rage_test(max_job_level WE4013)
    rage_test(mcstore_backup CLIENT contents/mcstore_json.c iteminfo/item_info.c json/json_tree.c
        json/service_json.c std/container_memory.c std/string.c std/vec.c OBJECTS rage_test_json_test_support
        LIBS rage_json)
    rage_test(mcstore_json CLIENT contents/mcstore_json.c core/owned_text.c json/json_tree.c
        json/service_json.c std/container_memory.c std/string.c OBJECTS rage_test_json_test_support LIBS
        rage_json)
    rage_test(memory W4)
    rage_test(mercenary_job WE4013 GENERATED mercenary_job)
    rage_test(message_tables tests/message_tables_support.c CLIENT core/utf8.c iteminfo/item_loader.c
        platform/win_utf8.c platform/window_win32.c std/container_memory.c std/string.c std/vec.c OBJECTS
        rage_test_weather_state_support)
    rage_test(meteor_storm_waves WE4013 GENERATED meteor_storm_waves)
    rage_test(mode_lifecycle CLIENT world/mode_loop.c)
    rage_test(mode_messages CLIENT world/mode_dispatch.c)
    rage_test(mode_tooltip WE4013)
    rage_test(model_keyframes CLIENT render/keyframes.c render/math3d.c ARGS
        ${PROJECT_SOURCE_DIR}/tests/fixtures/model_keyframes.bin)
    rage_test(model_lighting CLIENT render/math3d.c world/model_node_light.c)
    rage_test(model_load CLIENT world/model_load.c)
    rage_test(model_mesh CLIENT render/math3d.c std/std_sort.c)
    rage_test(model_mesh_load tests/model_mesh_load_support.c CLIENT core/numeric.c platform/clock_win32.c
        render/math3d.c resources/model_res_queries.c std/std_sort.c)
    rage_test(model_mesh_normals CLIENT render/anim_mesh.c render/math3d.c ARGS
        ${PROJECT_SOURCE_DIR}/tests/fixtures/model_mesh_normals.bin)
    rage_test(model_node_lifetime tests/model_node_lifetime_support.c CLIENT render/math3d.c)
    rage_test(model_resource_types CLIENT core/owned_text.c resources/model_res_queries.c)
    rage_test(monster_skill_info WE4013 CLIENT core/cstr.c core/memory.c core/xml.c std/string.c GENERATED
        monster_skill_info)
    rage_test(mouse CLIENT core/utf8.c platform/win_utf8.c OBJECTS test_mouse_mouse test_mouse_window_win32)
    rage_test(multiline_draw W1 WE4013 CLIENT core/byte_array.c core/owned_text.c core/text_array.c
        core/utf8.c platform/clock_win32.c platform/win_utf8.c platform/window_win32.c
        ui/controls/edit3_lines.c ui/controls/window_coloured_lines.c ui/items/item_tag_buttons.c)
    rage_test(multiline_lines W1 WE4013 CLIENT core/byte_array.c core/fatal.c core/memory.c core/owned_text.c
        core/text_array.c core/utf8.c std/string.c ui/controls/edit3_lines.c
        ui/controls/window_coloured_lines.c)
    rage_test(multiline_storage W4)
    rage_test(name_cards CLIENT core/once_win32.c std/container_memory.c std/string.c std/tree.c std/vec.c)
    rage_test(name_text WE4013 CLIENT core/hex.c core/memory.c core/owned_text.c core/text_format.c
        core/utf8.c std/string.c GENERATED name_text)
    rage_test(navi_text W1 CLIENT ui/chat/navi_text.c)
    rage_test(navi_text_lines W4 CLIENT core/utf8.c navi/navi_text_lines.c)
    rage_test(navigation_events tests/navigation_event_support.c CLIENT game/navigation.c
        navi/navi_object_queries.c navi/navi_records.c navi/navi_window.c platform/clock_win32.c
        std/container_memory.c std/string.c std/vec.c ui/quests/navigation_view.c)
    rage_test(navigation_objects tests/navigation_object_support.c CLIENT core/utf8.c navi/navi_containers.c
        platform/win_utf8.c platform/window_win32.c std/container_memory.c std/string.c std/vec.c OBJECTS
        rage_test_weather_state_support)
    rage_test(navigation_state tests/navigation_state_support.c CLIENT navi/navi_containers.c
        navi/navi_guide.c navi/navi_instance.c navi/navi_manager.c navi/navi_object_queries.c
        navi/navi_records.c render/texture_image.c std/container_memory.c std/string.c std/vec.c
        world/pathinfo.c OBJECTS rage_test_weather_state_support)
    rage_test(object_operations WE4013 CLIENT actor/object_ops.c core/abstract_method.c
        effects/skills/dummy_effect_launcher.c platform/clock_win32.c std/container_memory.c std/list.c
        std/string.c std/tree.c std/vec.c world/objects.c world/objects_remove.c)
    rage_test(object_type W4 tests/fixtures/type_query_reference.c OBJECTS test_object_type_actor_object_types
        test_object_type_character_object_types test_object_type_chat_object_types
        test_object_type_controls_object_types test_object_type_hotkey_object_types
        test_object_type_items_object_types test_object_type_mail_object_types
        test_object_type_map_object_types test_object_type_object_type test_object_type_object_type_registry
        test_object_type_quests_object_types test_object_type_render_ui_object_types
        test_object_type_resource_object_types test_object_type_shops_object_types
        test_object_type_skill_object_types test_object_type_social_object_types
        test_object_type_worldmap_object_types)
    rage_test(once W4 CLIENT core/once_win32.c)
    rage_test(own_states)
    rage_test(owned_booking CLIENT contents/party_booking_data.c)
    rage_test(packet_dispatch W1 WE4013 tests/packet_dispatch_sinks.c CLIENT game/packet_dispatch.c
        game/packet_dispatch2.c)
    rage_test(packets CLIENT contents/byte_queue.c contents/rag_connection.c core/once_win32.c
        platform/clock_win32.c)
    rage_test(particle_draw CLIENT effects/ez2str_draw.c render/math3d.c)
    rage_test(particle_pools W1 CLIENT platform/clock_win32.c render/math3d.c render/rp_pool.c
        std/container_memory.c std/string.c std/vec.c)
    rage_test(particle_process CLIENT effects/ez2str_playback.c effects/str_animation.c)
    rage_test(particle_view CLIENT effects/particle_view.c)
    rage_test(password_encode W4 CLIENT login/password_encode.c LIBS rage_crijndael)
    rage_test(path_info CLIENT core/numeric.c render/math3d.c world/pathfinder.c)
    rage_test(pathfinder_owner tests/fixtures/pathfinder_reference.c CLIENT core/numeric.c core/utf8.c
        fileio/file.c fileio/resource_path.c platform/code_page_win32.c platform/filesystem_win32.c
        render/math3d.c resources/attr_queries.c std/container_memory.c std/string.c std/tree.c std/vec.c
        OBJECTS rage_test_weather_state_support)
    rage_test(perf_overlay WE4013 GENERATED perf_overlay)
    rage_test(pet_accessory GENERATED pet_accessory LIBS rage_lua)
    rage_test(pet_actions GENERATED pet_actions)
    rage_test(pet_portrait GENERATED pet_portrait)
    rage_test(pick_state tests/pick_state_support.c CLIENT std/container_memory.c std/list.c std/vec.c
        world/pick_node.c)
    rage_test(plain_lines W1 WE4013 CLIENT core/hex.c core/owned_text.c core/text_array.c core/text_format.c
        core/utf8.c platform/code_page_win32.c ui/controls/window_lines.c)
    rage_test(player_names)
    rage_test(prim_creation CLIENT effects/effect_objects.c effects/rag/rag_prim_launch.c)
    rage_test(private_airplane WE4013 CLIENT core/memory.c std/string.c GENERATED private_airplane)
    rage_test(projection tests/fixtures/projection_reference.c CLIENT render/math3d.c render/rp_project.c
        render/rp_screen.c resources/attr_queries.c)
    rage_test(quest_journal CLIENT ui/quests/quest_journal_layout.c)
    rage_test(quest_loading)
    rage_test(quest_packets CLIENT contents/quest_value.c game/quest_hunting.c game/quest_list.c
        platform/clock_win32.c std/container_memory.c std/string.c std/string_vec.c std/vec.c
        ui/std/ui_std_vector.c OBJECTS rage_test_quest_value_support)
    rage_test(quest_time CLIENT platform/clock_win32.c)
    rage_test(quest_value CLIENT contents/quest_value.c std/container_memory.c std/string.c std/string_vec.c
        std/vec.c ui/std/ui_std_vector.c OBJECTS rage_test_quest_value_support)
    rage_test(ranking CLIENT contents/ranking_style.c initializers/markup_tags.c std/string.c)
    rage_test(readable_links CLIENT core/text_markup.c core/utf8.c iteminfo/item_info.c iteminfo/item_link.c
        std/container_memory.c std/list.c std/string.c std/vec.c ui/chat/chat_link_items.c
        ui/chat/item_link_text.c ui/chat/navi_text.c)
    rage_test(render_operations WE4013 src/actor/sprite_overlays.c src/actor/motion_update.c FORCE_INCLUDE
        tests/render_operations_boundaries.h)
    rage_test(renderer_storage CLIENT core/utf8.c fileio/resource_path.c platform/clock_win32.c
        platform/code_page_win32.c platform/filesystem_win32.c platform/system_memory_win32.c
        platform/thread_win32.c platform/win_utf8.c platform/window_win32.c std/std_sort.c)
    rage_test_run(renderer_storage_growth_failure COMMAND test_renderer_storage growth_failure EXIT 3)
    rage_test_run(renderer_storage_queue_failure COMMAND test_renderer_storage queue_failure EXIT 3)
    rage_test(renew_quest_text CLIENT contents/renew_quest_text.c core/memory.c core/utf8.c std/string.c)
    rage_test(resource_cache tests/resource_cache_support.c CLIENT core/cstr.c core/utf8.c
        fileio/grf_mapfile.c fileio/grf_pak.c fileio/res_cache.c fileio/res_get.c
        fileio/resource_cache_store.c fileio/resource_path.c platform/clock_win32.c platform/code_page_win32.c
        platform/filesystem_win32.c platform/thread_win32.c render/cache_lock.c resources/resource_registry.c
        std/container_memory.c std/string.c std/vec.c)
    rage_test(resource_errors CLIENT core/cstr.c fileio/resource_errors.c platform/thread_win32.c)
    rage_test(resource_lifecycle CLIENT std/string.c)
    rage_test(resource_paths tests/resource_paths_support.c CLIENT core/text_util.c platform/code_page_win32.c
        std/container_memory.c std/string.c std/vec.c ui/resource/resource_paths.c)
    rage_test(resource_registry CLIENT resources/resource_registry.c)
    rage_test(resource_spans CLIENT core/utf8.c fileio/file.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c)
    rage_test(rma_owner CLIENT core/utf8.c fileio/file.c fileio/resource_path.c platform/code_page_win32.c
        platform/filesystem_win32.c)
    rage_test(rodex_contents CLIENT initializers/markup_tags.c std/container_memory.c std/list.c std/string.c
        std/vec.c ui/chat/message_tags.c ui/chat/navi_text.c ui/std/ui_std_vector.c)
    rage_test(rodex_send CLIENT core/cstr.c std/container_memory.c std/std_comdats.c std/string.c std/vec.c)
    rage_test(rsx_owner ARGS ${PROJECT_SOURCE_DIR}/tests/fixtures/rsx_records.bin)
    rage_test(scene_bounds CLIENT world/scene_bounds.c)
    rage_test(scene_ground_box)
    rage_test(scene_lifecycle)
    rage_test(scene_storage CLIENT world/ray_picker.c world/scene_geometry.c)
    rage_test(schedule_ids CLIENT contents/schedule_ids.c)
    rage_test(scheduler CLIENT contents/ranking_style.c initializers/markup_tags.c std/string.c)
    rage_test(screenshot_save GENERATED screenshot_save CLIENT core/client_path.c core/memory.c
        platform/filesystem_win32.c std/string.c std/vec.c)
    rage_test(scroll_operations CLIENT std/list.c std/tree.c std/vec.c)
    rage_test(service_json CLIENT core/owned_text.c json/json_tree.c json/service_json.c
        std/container_memory.c std/string.c OBJECTS rage_test_json_test_support LIBS rage_json)
    rage_test(session_companion CLIENT game/companion_packets.c)
    rage_test(session_fields CLIENT game/shortcut_keys.c session/character_slots.c)
    rage_test(session_scalar_packets)
    rage_test(session_text CLIENT core/owned_text.c json/json_tree.c json/service_json.c platform/win_utf8.c
        std/container_memory.c std/string.c OBJECTS rage_test_json_test_support LIBS rage_json)
    rage_test(shortcut_packets W1 WE4013 CLIENT core/cstr.c platform/clock_win32.c)
    rage_test(shorten_text W1 CLIENT core/utf8.c ui/controls/window_shorten.c)
    rage_test(skill_action_registry WE4013 CLIENT skills/skill_action_mgr.c skills/skill_action_registry.c
        skills/skill_actions.c std/tree.c std/vec.c)
    rage_test(skill_cast_chat W1 WE4013 CLIENT game/skill_cast_chat.c std/container_memory.c std/string.c
        std/vec.c)
    rage_test(skill_tree_hints WE4013 CLIENT std/list.c GENERATED skill_tree_hints)
    rage_test(skin_manager CLIENT platform/filesystem_win32.c std/string.c std/string_vec.c std/vec.c
        ui/misc/skin_mgr.c)
    rage_test(snap_state CLIENT ui/misc/snap_mgr.c)
    rage_test(sound_manager CLIENT platform/thread_win32.c)
    rage_test(sprite_atlas)
    rage_test(sprite_owner CLIENT core/utf8.c fileio/file.c fileio/resource_path.c platform/clock_win32.c
        platform/code_page_win32.c platform/filesystem_win32.c render/device_palette.c)
    rage_test(startup_order WE4013 CLIENT core/client_managers.c core/manager_table.c std/string.c GENERATED
        startup_order)
    rage_test(startup_settings CLIENT platform/win_utf8.c)
    rage_test(status_icons CLIENT actor/player_status_icons.c)
    rage_test(status_wnd_info WE4013 CLIENT ui/wndmgr/wnd_infos.c GENERATED status_wnd_info)
    rage_test(str_animation CLIENT effects/ez2str_playback.c effects/rag/rag_str_playback.c
        effects/str_animation.c)
    rage_test(str_paths CLIENT effects/ez2str_particle.c effects/ez2str_playback.c
        effects/rag/rag_str_playback.c effects/str_animation.c effects/str_path.c render/math3d.c
        std/container_memory.c std/string.c std/vec.c)
    rage_test(string_move W4 CLIENT std/string.c)
    rage_test(surface_lifecycle CLIENT core/utf8.c platform/clock_win32.c platform/system_memory_win32.c
        platform/thread_win32.c platform/win_utf8.c platform/window_win32.c)
    rage_test(tag_text_viewer CLIENT initializers/markup_tags.c iteminfo/item_info.c std/container_memory.c
        std/std_comdats.c std/string.c std/vec.c ui/std/ui_std_vector.c)
    rage_test(talk_option_lua LIBS rage_lua)
    rage_test(talk_option_storage)
    rage_test(talk_type CLIENT core/memory.c core/text_tokens.c core/text_util.c platform/code_page_win32.c
        session/session_talk_type.c std/string.c std/string_vec.c std/vec.c)
    rage_test(text_array W4)
    rage_test(text_buttons CLIENT initializers/markup_tags.c iteminfo/item_info.c std/container_memory.c
        std/std_comdats.c std/string.c std/vec.c ui/std/ui_std_vector.c)
    rage_test(text_consumers tests/encoding_fixture.c CLIENT core/utf8.c initializers/markup_tags.c
        platform/code_page_win32.c std/string.c OBJECTS test_text_consumers_ellipsize_text
        test_text_consumers_text_encoding)
    rage_test(text_format W4 CLIENT core/hex.c core/text_markup.c core/utf8.c platform/code_page_win32.c)
    rage_test(text_markup W4 CLIENT core/text_markup.c)
    rage_test(text_parsing W4 CLIENT game/scream_text.c ui/chat/speaker_name.c ui/controls/item_id_tag.c)
    rage_test(text_render CLIENT core/hex.c core/owned_text.c core/text_format.c core/utf8.c
        platform/code_page_win32.c render/text_runs.c)
    rage_test(text_runs W4 CLIENT core/utf8.c render/text_runs.c)
    rage_test(text_tokens CLIENT core/text_tokens.c std/string.c std/string_vec.c std/vec.c
        ui/controls/sprite_paths.c)
    rage_test(text_viewer_links tests/text_viewer_links_unreached.c CLIENT core/memory.c core/text_markup.c
        initializers/markup_tags.c std/container_memory.c std/std_comdats.c std/string.c std/vec.c
        ui/controls/controls_std.c)
    rage_test(texture_cache_owner MAIN tests/test_texture_cache_owner.c CLIENT platform/system_memory_win32.c
        platform/thread_win32.c)
    rage_test(texture_quad CLIENT render/math3d.c render/texture_quad.c)
    rage_test(thread_text W1 WE4013 CLIENT core/thread_text.c)
    rage_test(town_info W1 WE4013 src/contents/town_info.c src/contents/lua_data.c src/core/once_win32.c
        src/std/string.c src/std/container_memory.c src/std/vec.c src/std/tree.c FORCE_INCLUDE
        tests/lua_data_manager_boundaries.h INCLUDES src/contents LIBS rage_lua)
    rage_test(traversal_callbacks tests/model_names_fixture.c tests/traversal_callbacks_support.c CLIENT
        core/numeric.c core/owned_text.c platform/clock_win32.c render/math3d.c resources/model_res_queries.c
        std/list.c std/tree.c std/vec.c world/model_actor.c world/world_near.c world/world_std.c)
    rage_test(ui_drop WE4013 CLIENT core/fatal.c platform/clock_win32.c std/list.c std/string_vec.c std/vec.c
        OBJECTS test_ui_drop_owner_2 test_ui_drop_owner_4 test_ui_drop_owner_8 test_ui_drop_owner_9
        test_ui_drop_owner_10)
    rage_test(ui_extensions W1 CLIENT core/abstract_method.c)
    rage_test(ui_invalidation OPTIONS /Oy- OBJECTS test_ui_invalidation_uiwindow)
    rage_test(url_link WE4013 tests/url_link_unreached.c CLIENT core/owned_text.c core/text_array.c GENERATED
        url_link)
    rage_test(user_hotkeys)
    rage_test(utf8 W4 CLIENT core/utf8.c)
    rage_test(vending_retry CLIENT core/fatal.c core/memory.c game/msg_vending.c iteminfo/item_info.c
        iteminfo/item_list.c platform/clock_win32.c session/session_item_lists.c session/session_items.c
        std/list.c std/string.c std/vec.c)
    rage_test(wave_palette CLIENT core/utf8.c fileio/file.c fileio/resource_path.c platform/code_page_win32.c
        platform/filesystem_win32.c render/device_palette.c)
    rage_test(weather_state CLIENT actor/weather.c actor/weather_instance.c std/container_memory.c
        std/string.c std/vec.c OBJECTS rage_test_weather_state_support)
    rage_test(whisper_block W1 tests/whisper_command_support.c tests/json_test_support.c CLIENT
        game/whisper_commands.c json/json_tree.c json/service_json.c std/container_memory.c std/string.c
        std/vec.c LIBS rage_json)
    rage_test(win_utf8 W4 OBJECTS test_win_utf8_win_utf8)
    rage_test(window_access CLIENT ui/window_access.c)
    rage_test(window_canvas_draw)
    rage_test(window_id_list WE4013 tests/window_id_list_unreached.c CLIENT std/list.c std/tree.c
        ui/wndmgr/make_window.c ui/wndmgr/plate_hooks.c ui/wndmgr/window_table.c ui/wndmgr/wndmgr_handlers.c
        GENERATED window_id_list window_rows)
    rage_test(window_lists WE4013 tests/window_lists_stubs.c CLIENT std/list.c std/string.c std/tree.c
        ui/wndmgr/forget_window.c ui/wndmgr/make_window.c ui/wndmgr/make_window_character.c
        ui/wndmgr/make_window_hud.c ui/wndmgr/make_window_login.c ui/wndmgr/make_window_social.c
        ui/wndmgr/make_window_trade.c ui/wndmgr/plate_hooks.c ui/wndmgr/teardown.c ui/wndmgr/window_registry.c
        ui/wndmgr/window_slots.c ui/wndmgr/window_table.c ui/wndmgr/wndmgr_handlers.c)
    rage_test(window_quad CLIENT ui/controls/window_render.c)
    rage_test(world_context CLIENT world/view_context.c)
    rage_test(world_models CLIENT platform/thread_win32.c)
    rage_test(world_player CLIENT world/world_player.c)
    rage_test(world_resource CLIENT core/utf8.c fileio/file.c fileio/resource_path.c
        platform/code_page_win32.c platform/filesystem_win32.c render/math3d.c)
    rage_test(world_view CLIENT ui/quests/world_view_display.c LIBS rage_lua)
    foreach(test effect_data effect_launch_duration effect_trace)
        add_dependencies(test_${test} rage_effect_data)
    endforeach()

    # The Adventurer's Agency handler of a patched Hercules checkout (docs/server/agency-packets.md),
    # when RAGE_HERCULES_DIR names one: agency_server writes the reply the handler builds, and
    # agency_server_reply decodes it with the client's decoder (test_agency_packets).
    set(RAGE_HERCULES_DIR "" CACHE PATH "A Hercules checkout with the Adventurer's Agency patch")
    if(RAGE_HERCULES_DIR)
        rage_test_generate(agency_server OUTPUTS agency_server_handler.inc
            DEPENDS ${RAGE_HERCULES_DIR}/src/map/clif.c
            ARGS ${RAGE_HERCULES_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated/agency_server/agency_server_handler.inc)
        rage_test(agency_server GENERATED agency_server ARGS server-list.bin LABELS hercules)
        rage_test_run(agency_server_reply COMMAND test_agency_packets
            ${CMAKE_CURRENT_BINARY_DIR}/work/agency_server/server-list.bin LABELS hercules)
        set_tests_properties(agency_server PROPERTIES FIXTURES_SETUP agency_server_reply)
        set_tests_properties(agency_server_reply PROPERTIES FIXTURES_REQUIRED agency_server_reply)
    endif()
endif()

# ---- Linux, macOS ----
if(NOT WIN32)
    rage_test(platform_settings CLIENT ${platform_files} DEFINES _POSIX_C_SOURCE=200809L)
    find_package(Freetype)
    find_package(Fontconfig)
    if(FREETYPE_FOUND AND Fontconfig_FOUND)
        rage_test(text_freetype src/render/fonts_ft.c src/render/font_match_fc.c src/render/textdc.c
            src/render/ctextdc.c src/render/text_freetype.c src/render/text_unicode.c
            src/render/text_runs.c src/render/cdc.c src/core/text_format.c src/core/owned_text.c
            src/core/hex.c src/core/client_path.c src/core/cstr.c DEFINES _POSIX_C_SOURCE=200809L
            CLIENT ${platform_files} LIBS Freetype::Freetype Fontconfig::Fontconfig)
    endif()
    if(TARGET SDL3::SDL3)
        # Silent: SDL's dummy audio driver, as every test has.
        rage_test(miles_sdl CLIENT ${platform_files} DEFINES _POSIX_C_SOURCE=200809L
            LIBS rage_dr_mp3 SDL3::SDL3 ARGS ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
        # The clipboard needs a video driver; the offscreen one shows no window.
        rage_test(platform_window src/platform/window_sdl.c CLIENT ${platform_files}
            DEFINES _POSIX_C_SOURCE=200809L LIBS SDL3::SDL3 ENVIRONMENT SDL_VIDEO_DRIVER=offscreen)
    endif()
endif()
