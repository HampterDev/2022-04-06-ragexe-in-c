# data/effects/effects.json is compiled into the client as a byte array, as build.bat does it:
# tools/bin2c.c writes effect_data_embedded.c with g_effectData and g_effectData_size
# (src/effects/effect_data.c reads them). A cross build (Android) cannot run a tool it compiles, so
# there cmake/bin2c.cmake writes the same file.
set(RAGE_EFFECT_DATA_JSON ${PROJECT_SOURCE_DIR}/data/effects/effects.json)
set(RAGE_EFFECT_DATA_C ${CMAKE_BINARY_DIR}/gen/effect_data_embedded.c)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/gen)
if(CMAKE_CROSSCOMPILING)
    add_custom_command(OUTPUT ${RAGE_EFFECT_DATA_C}
        COMMAND ${CMAKE_COMMAND} -DOUT=${RAGE_EFFECT_DATA_C} -DNAME=g_effectData -DIN=${RAGE_EFFECT_DATA_JSON}
            -P ${PROJECT_SOURCE_DIR}/cmake/bin2c.cmake
        DEPENDS ${RAGE_EFFECT_DATA_JSON} ${PROJECT_SOURCE_DIR}/cmake/bin2c.cmake
        COMMENT "Embedding data/effects/effects.json"
        VERBATIM)
else()
    add_executable(rage_bin2c tools/bin2c.c)
    add_custom_command(OUTPUT ${RAGE_EFFECT_DATA_C}
        COMMAND rage_bin2c ${RAGE_EFFECT_DATA_C} g_effectData ${RAGE_EFFECT_DATA_JSON}
        DEPENDS rage_bin2c ${RAGE_EFFECT_DATA_JSON}
        COMMENT "Embedding data/effects/effects.json"
        VERBATIM)
endif()
