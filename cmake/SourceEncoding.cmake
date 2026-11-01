function(rage_legacy_source output source)
    if(MSVC)
        set(${output} "${CMAKE_CURRENT_SOURCE_DIR}/${source}" PARENT_SCOPE)
        return()
    endif()
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/cp949/${source}")
    add_custom_command(OUTPUT "${generated}"
        COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/source_encoding.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/${source}" "${generated}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}" "${PROJECT_SOURCE_DIR}/tools/source_encoding.py"
        VERBATIM)
    set(${output} "${generated}" PARENT_SCOPE)
endfunction()
