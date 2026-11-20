# rage_legacy_source(<output variable> <file relative to the repository root>)
function(rage_legacy_source output source)
    if(MSVC)
        set(${output} "${PROJECT_SOURCE_DIR}/${source}" PARENT_SCOPE)
        return()
    endif()
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/cp949/${source}")
    add_custom_command(OUTPUT "${generated}"
        COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/source_encoding.py"
            "${PROJECT_SOURCE_DIR}/${source}" "${generated}"
        DEPENDS "${PROJECT_SOURCE_DIR}/${source}" "${PROJECT_SOURCE_DIR}/tools/source_encoding.py"
        VERBATIM)
    set(${output} "${generated}" PARENT_SCOPE)
endfunction()
