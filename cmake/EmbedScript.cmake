# cmake/EmbedScript.cmake
# Usage:
#   include(cmake/EmbedScript.cmake)
#   embed_script(snake scripts/snake.lua)
#
# Generates build/generated/embedded_<NAME>.hpp with:
#   namespace embedded { inline constexpr char <NAME>[] = "..."; }

function(embed_script NAME LUA_PATH)
    set(OUT_DIR  "${CMAKE_BINARY_DIR}/generated")
    set(OUT_FILE "${OUT_DIR}/embedded_${NAME}.hpp")

    file(MAKE_DIRECTORY "${OUT_DIR}")

    add_custom_command(
        OUTPUT  "${OUT_FILE}"
        COMMAND ${CMAKE_COMMAND}
                -DLUA_PATH="${CMAKE_SOURCE_DIR}/${LUA_PATH}"
                -DOUT_FILE="${OUT_FILE}"
                -DVAR_NAME="${NAME}"
                -P "${CMAKE_SOURCE_DIR}/cmake/EmbedScriptRun.cmake"
        DEPENDS "${CMAKE_SOURCE_DIR}/${LUA_PATH}"
        COMMENT "Embedding ${LUA_PATH} -> ${OUT_FILE}"
    )

    add_custom_target(embed_${NAME} DEPENDS "${OUT_FILE}")
endfunction()
