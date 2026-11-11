# cmake -DEXIT=<code> -P expect_exit.cmake <program> [<argument>...]
# Runs a test program that must end with one exit code other than 0 (rage_test_run in
# tests/CMakeLists.txt, EXIT): passes when it does, fails with any other result.
set(command)
set(after_script FALSE)
math(EXPR last "${CMAKE_ARGC} - 1")
foreach(i RANGE ${last})
    if(after_script)
        list(APPEND command "${CMAKE_ARGV${i}}")
    elseif(CMAKE_ARGV${i} STREQUAL CMAKE_CURRENT_LIST_FILE OR CMAKE_ARGV${i} MATCHES "expect_exit[.]cmake$")
        set(after_script TRUE)
    endif()
endforeach()
execute_process(COMMAND ${command} RESULT_VARIABLE result)
if(NOT result STREQUAL EXIT)
    message(FATAL_ERROR "${command}: exit code ${result}, expected ${EXIT}")
endif()
