# cmake/EmbedScript.cmake
# Usage (from CMakeLists.txt):
#   include(cmake/EmbedScript.cmake)
#   embed_script(snake scripts/snake.lua)
#
# Generates build/generated/embedded_snake.hpp with:
#   namespace embedded { inline constexpr char snake[] = "..."; }

function(embed_script NAME LUA_PATH)
    set(OUT_DIR  "${CMAKE_BINARY_DIR}/generated")
    set(OUT_FILE "${OUT_DIR}/embedded_${NAME}.hpp")

    file(MAKE_DIRECTORY "${OUT_DIR}")

    # Re-generate whenever the .lua file changes.
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/EmbedScriptTemplate.cmake.in"
        "${OUT_DIR}/_embed_${NAME}.cmake"
        @ONLY
    )

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

    # Expose as a source-level target so the executable depends on it.
    add_custom_target(embed_${NAME} DEPENDS "${OUT_FILE}")
endfunction()
