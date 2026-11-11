# cmake -DPROGRAM=<test> -DORIGINAL=<original executable> -P run.cmake
# Runs one oracle test (CMakeLists.txt) against the original executable that ORACLE_EXE names, or
# else ORIGINAL; without that file the test is reported skipped.
if(NOT "$ENV{ORACLE_EXE}" STREQUAL "")
    set(ORIGINAL "$ENV{ORACLE_EXE}")
endif()
if(NOT EXISTS "${ORIGINAL}")
    message("oracle: skipped, no original executable at ${ORIGINAL} (set ORACLE_EXE)")
    return()
endif()
execute_process(COMMAND "${PROGRAM}" "${ORIGINAL}" RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${PROGRAM}: exit code ${result}")
endif()
