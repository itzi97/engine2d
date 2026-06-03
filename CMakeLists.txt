cmake_minimum_required(VERSION 3.28)
project(2d-engine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(cmake/EmbedScript.cmake)

# ---------------------------------------------------------------------------
# Game selection
# ---------------------------------------------------------------------------
set(GAME "snake" CACHE STRING "Game script to embed (filename without .lua)")
set_property(CACHE GAME PROPERTY STRINGS snake breakout)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/scripts/${GAME}.lua")
  message(FATAL_ERROR "Game script not found: scripts/${GAME}.lua")
endif()
message(STATUS "Building game: ${GAME}")

# -- SDL3 ---------------------------------------------------------------------
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.14
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# -- SDL3_ttf -----------------------------------------------------------------
set(SDLTTF_VENDORED TRUE CACHE BOOL "" FORCE)   # use bundled FreeType/HarfBuzz
FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG        release-3.2.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3_ttf)

# -- GLM ----------------------------------------------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# -- Lua 5.4.8 ----------------------------------------------------------------
cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.8
    GIT_SHALLOW    TRUE
)
FetchContent_Populate(lua)

file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
list(REMOVE_ITEM LUA_SOURCES
    "${lua_SOURCE_DIR}/lua.c"
    "${lua_SOURCE_DIR}/luac.c"
    "${lua_SOURCE_DIR}/onelua.c"
    "${lua_SOURCE_DIR}/ltests.c"
)
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC "${lua_SOURCE_DIR}")
target_compile_options(lua_static PRIVATE -Wno-deprecated-declarations)

# -- Lua shim -----------------------------------------------------------------
set(LUA_SHIM_DIR "${CMAKE_BINARY_DIR}/lua-shim")
file(MAKE_DIRECTORY "${LUA_SHIM_DIR}")

configure_file("${CMAKE_SOURCE_DIR}/cmake/lua_shim/lua.hpp"
               "${LUA_SHIM_DIR}/lua.hpp" COPYONLY)
foreach(_hdr lua.h luaconf.h lualib.h lauxlib.h)
    if(EXISTS "${lua_SOURCE_DIR}/${_hdr}")
        configure_file("${lua_SOURCE_DIR}/${_hdr}"
                       "${LUA_SHIM_DIR}/${_hdr}" COPYONLY)
    endif()
endforeach()

# -- sol2 ---------------------------------------------------------------------
set(SOL2_ENABLE_INSTALL FALSE CACHE BOOL   "" FORCE)
set(SOL2_LUA_VERSION   "5.4"  CACHE STRING "" FORCE)
set(SOL2_BUILD_LUA     FALSE  CACHE BOOL   "" FORCE)
set(SOL2_TESTS         FALSE  CACHE BOOL   "" FORCE)
set(SOL2_EXAMPLES      FALSE  CACHE BOOL   "" FORCE)
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        develop
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(sol2)
target_include_directories(sol2 INTERFACE "${LUA_SHIM_DIR}" "${lua_SOURCE_DIR}")

# -- nlohmann/json ------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# -- DejaVu Sans font (MIT-compatible, no build dep) -------------------------
set(FONT_DIR "${CMAKE_SOURCE_DIR}/assets/fonts")
set(FONT_FILE "${FONT_DIR}/DejaVuSans.ttf")
if(NOT EXISTS "${FONT_FILE}")
  message(STATUS "Downloading DejaVuSans.ttf...")
  file(MAKE_DIRECTORY "${FONT_DIR}")
  file(DOWNLOAD
    "https://github.com/dejavu-fonts/dejavu-fonts/raw/version_2_37/ttf/DejaVuSans.ttf"
    "${FONT_FILE}"
    EXPECTED_HASH SHA256=7b8028f8a2b6f97b3c0cf3ebdab52ac63ae40e9a38c8e23bab037f09e0c01cf6
    TLS_VERIFY ON
  )
endif()

# -- Embed selected game script -----------------------------------------------
embed_script(${GAME} scripts/${GAME}.lua)

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
execute_process(
    COMMAND ${CMAKE_COMMAND}
            -DGAME_NAME=${GAME}
            -DOUT_DIR=${GENERATED_DIR}
            -P "${CMAKE_SOURCE_DIR}/cmake/GenerateGameShim.cmake"
)

# -- Sources ------------------------------------------------------------------
set(ENGINE_SOURCES
    src/main.cpp
    src/core/Game.cpp
    src/ecs/World.cpp
    src/ecs/components/KinematicComponent.cpp
    src/ecs/components/SpriteComponent.cpp
    src/ecs/systems/CollisionSystem.cpp
    src/ecs/systems/PhysicsSystem.cpp
    src/ecs/systems/RenderSystem.cpp
    src/ecs/systems/TextSystem.cpp
    src/input/InputManager.cpp
    src/rendering/FontManager.cpp
    src/scripting/ScriptingEngine.cpp
)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES})
add_dependencies(${PROJECT_NAME} embed_${GAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    "${GENERATED_DIR}"
    "${LUA_SHIM_DIR}"
    "${lua_SOURCE_DIR}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    FONT_PATH="${FONT_DIR}/DejaVuSans.ttf"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    glm::glm
    nlohmann_json::nlohmann_json
    sol2
    lua_static
)

# -- Compiler Warnings --------------------------------------------------------
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()
