cmake_minimum_required(VERSION 3.28)
project(2d-engine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(cmake/EmbedScript.cmake)

# ---------------------------------------------------------------------------
# Bootstrap script selection
# The engine embeds exactly ONE Lua file: the game bootstrap (scripts/main.lua).
# It calls engine.set_font_path(), scene.register(), and scene.load().
# ---------------------------------------------------------------------------
set(GAME "main" CACHE STRING "Bootstrap script to embed (filename without .lua, relative to scripts/)")

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/scripts/${GAME}.lua")
  message(FATAL_ERROR "Bootstrap script not found: scripts/${GAME}.lua")
endif()
message(STATUS "Embedding bootstrap: scripts/${GAME}.lua")

# -- SDL3 ---------------------------------------------------------------------
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.14
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# -- SDL3_ttf -----------------------------------------------------------------
set(SDLTTF_VENDORED TRUE CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG        release-3.2.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3_ttf)

# -- SDL3_image ---------------------------------------------------------------
set(SDLIMAGE_VENDORED  TRUE  CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF      OFF   CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL       OFF   CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF       OFF   CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP      OFF   CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-3.2.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3_image)

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

# -- stb_vorbis (header-only, OGG decode) ------------------------------------
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

set_source_files_properties(src/audio/stb_vorbis_impl.c PROPERTIES
    COMPILE_OPTIONS "-w"
)

# -- DejaVu Sans font ---------------------------------------------------------
set(FONT_DIR  "${CMAKE_SOURCE_DIR}/assets/fonts")
set(FONT_FILE "${FONT_DIR}/DejaVuSans.ttf")

if(NOT EXISTS "${FONT_FILE}")
  FetchContent_Declare(
      dejavu_fonts
      URL      https://github.com/dejavu-fonts/dejavu-fonts/releases/download/version_2_37/dejavu-fonts-ttf-2.37.tar.bz2
      URL_HASH SHA256=fa9ca4d13871dd122f61258a80d01751d603b4d3ee14095d65453b4e846e17d7
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(dejavu_fonts)

  file(MAKE_DIRECTORY "${FONT_DIR}")
  file(COPY "${dejavu_fonts_SOURCE_DIR}/ttf/DejaVuSans.ttf"
       DESTINATION "${FONT_DIR}")
  message(STATUS "DejaVuSans.ttf installed to ${FONT_FILE}")
endif()

# -- Embed bootstrap script ---------------------------------------------------
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
    src/core/SceneManager.cpp
    src/core/HotReload.cpp
    src/audio/AudioManager.cpp
    src/audio/stb_vorbis_impl.c
    src/ecs/World.cpp
    src/ecs/systems/AnimationSystem.cpp
    src/ecs/systems/CollisionSystem.cpp
    src/ecs/systems/PhysicsSystem.cpp
    src/ecs/systems/RenderSystem.cpp
    src/ecs/systems/TextSystem.cpp
    src/input/InputManager.cpp
    src/map/MapLoader.cpp
    src/map/MapSystem.cpp
    src/rendering/FontManager.cpp
    src/rendering/TextureManager.cpp
    src/scripting/ScriptingEngine.cpp
    src/scripting/bindings/KeycodeFromString.cpp
    src/scripting/bindings/BindWorld.cpp
    src/scripting/bindings/BindEngine.cpp
    src/scripting/bindings/BindTextures.cpp
    src/scripting/bindings/BindAudio.cpp
    src/scripting/bindings/BindMapValidation.cpp
)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES})
add_dependencies(${PROJECT_NAME} embed_${GAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    "${GENERATED_DIR}"
    "${LUA_SHIM_DIR}"
    "${lua_SOURCE_DIR}"
    "${stb_SOURCE_DIR}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:HOTRELOAD_ENABLED>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
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
