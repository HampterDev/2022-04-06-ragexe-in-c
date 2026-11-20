# The oracle tests (README.md): 32-bit programs that map the original executable and compare its
# routines with the lifted C. They are a configuration of their own, -DRAGE_ORACLE=ON from a
# 32-bit MSVC environment (tests\build.bat makes it in build\oracle), since everything else is
# 64-bit. When they run they read the original from ORACLE_EXE, or else from RAGE_ORACLE_EXE
# (..\client-files\2022-04-06_Ragexe_1648707856.exe beside the checkout by default), and report
# themselves skipped when it is not there.
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The oracle tests are 32-bit Windows programs: configure them from a 32-bit MSVC environment.")
endif()
get_filename_component(default_original "${PROJECT_SOURCE_DIR}/../client-files/2022-04-06_Ragexe_1648707856.exe"
    ABSOLUTE)
set(RAGE_ORACLE_EXE "${default_original}" CACHE FILEPATH "The original executable, unless ORACLE_EXE names one")

# The C runtime DLL, and the client's files the tests compare, compiled once; each test takes the
# objects it uses from the library.
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
add_compile_options(/UNDEBUG /W3 /WX /we4013)
# The client's actor types name the records of the piece rules (game/rules/view), headers only here.
include_directories(${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/vendor/lua-5.1.4/src
    ${PROJECT_SOURCE_DIR}/vendor/zlib-1.2.11 ${PROJECT_SOURCE_DIR}/vendor/libpng-1.6.36 ${PROJECT_SOURCE_DIR}/vendor/yyjson-0.13.0
    ${PROJECT_SOURCE_DIR}/game/rules)
add_library(oracle_lifted STATIC oracle_image.c
    ${PROJECT_SOURCE_DIR}/src/actors/ground_skill.c ${PROJECT_SOURCE_DIR}/src/effects/durations.c
    ${PROJECT_SOURCE_DIR}/src/game/cell_coordinates.c ${PROJECT_SOURCE_DIR}/src/game/skill_cast.c
    ${PROJECT_SOURCE_DIR}/src/game/skill_cast_look.c ${PROJECT_SOURCE_DIR}/src/game/skill_hit_fx.c
    ${PROJECT_SOURCE_DIR}/src/game/skill_notify.c ${PROJECT_SOURCE_DIR}/src/game/skill_use.c
    ${PROJECT_SOURCE_DIR}/src/game/skill_use_basic.c ${PROJECT_SOURCE_DIR}/src/game/skill_use_companion.c
    ${PROJECT_SOURCE_DIR}/src/game/skill_use_extra.c ${PROJECT_SOURCE_DIR}/src/game/skill_use_late.c
    ${PROJECT_SOURCE_DIR}/src/game/skill_use_second.c ${PROJECT_SOURCE_DIR}/src/render/cdcbitmap.c
    ${PROJECT_SOURCE_DIR}/src/render/math3d.c
    ${PROJECT_SOURCE_DIR}/src/session/session_cast_effect.c ${PROJECT_SOURCE_DIR}/src/session/session_use_effect.c
    ${PROJECT_SOURCE_DIR}/src/std/tree.c)

# The lookups test_job_lookups compares, copied out of the client's files.
set(generated ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${generated})
add_custom_command(OUTPUT ${generated}/job_lookups.inc
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/extract_functions.py ${PROJECT_SOURCE_DIR}
        ${generated}/job_lookups.inc
        "src/session/predicates.c::BYTE session_is_super_novice_job("
        "src/session/session_lookups.c::int session_remap_job("
        "src/session/sprite_names.c::int session_get_mounted_job("
        "src/session/session_party.c::DWORD session_get_party_color_by_seq("
        "src/session/session_misc.c::int session_get_max_job_level("
    DEPENDS extract_functions.py ${PROJECT_SOURCE_DIR}/src/session/predicates.c
        ${PROJECT_SOURCE_DIR}/src/session/session_lookups.c ${PROJECT_SOURCE_DIR}/src/session/sprite_names.c
        ${PROJECT_SOURCE_DIR}/src/session/session_party.c ${PROJECT_SOURCE_DIR}/src/session/session_misc.c
    COMMENT "Extracting the job lookups for the oracle" VERBATIM)
add_custom_target(oracle_job_lookups DEPENDS ${generated}/job_lookups.inc)

foreach(name skill_poses job_lookups ground_skill skill_use skill_hit skill_cast canvas_texels)
    add_executable(test_${name} test_${name}.c)
    target_link_libraries(test_${name} PRIVATE oracle_lifted user32)
    # Linked away from the original's base 0x400000, which the oracle maps the original at.
    target_link_options(test_${name} PRIVATE /MACHINE:X86 /BASE:0x10000000 /FIXED /DYNAMICBASE:NO)
    add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:test_${name}>
        -DORIGINAL=${RAGE_ORACLE_EXE} -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake)
    set_tests_properties(${name} PROPERTIES LABELS oracle SKIP_REGULAR_EXPRESSION "oracle: skipped")
endforeach()
add_dependencies(test_job_lookups oracle_job_lookups)
target_include_directories(test_job_lookups PRIVATE ${generated})
# The canvas test makes managed textures through the system's DirectDraw.
target_link_libraries(test_canvas_texels PRIVATE ddraw dxguid)
