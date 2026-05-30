cmake_minimum_required(VERSION 3.28)
project(2d-engine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# -- SDL3 ---------------------------------------------------------------------
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.14
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# -- GLM ----------------------------------------------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# -- Lua 5.4 ------------------------------------------------------------------
# Lua has no CMakeLists.txt so we build it manually.
# CMP0169: suppress FetchContent_Populate deprecation warning.
cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.7
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
# Place the archive next to other targets so the linker can find it.
set_target_properties(lua_static PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# -- sol2 ---------------------------------------------------------------------
# Disable sol2's own install() rules — they fail because lua_static has none,
# causing two CMake Generate-step errors about missing export sets.
set(SOL2_ENABLE_INSTALL FALSE CACHE BOOL   "" FORCE)
# Disable sol2's own Lua search so it never picks up the system Lua headers.
set(SOL2_LUA_VERSION   "5.4"  CACHE STRING "" FORCE)
set(SOL2_BUILD_LUA     FALSE  CACHE BOOL   "" FORCE)
set(SOL2_TESTS         FALSE  CACHE BOOL   "" FORCE)
set(SOL2_EXAMPLES      FALSE  CACHE BOOL   "" FORCE)
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(sol2)

# Force sol2 (header-only interface target) to see our Lua before the system one.
target_include_directories(sol2 INTERFACE
    "$<BUILD_INTERFACE:${lua_SOURCE_DIR}>"
)
target_link_libraries(sol2 INTERFACE lua_static)

# -- nlohmann/json ------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# -- Sources ------------------------------------------------------------------
set(ENGINE_SOURCES
    src/main.cpp
    src/core/Game.cpp
    src/ecs/World.cpp
    src/scripting/ScriptingEngine.cpp
)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    glm::glm
    sol2              # pulls in lua_static transitively
    nlohmann_json::nlohmann_json
)

# -- Compiler Warnings --------------------------------------------------------
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()
