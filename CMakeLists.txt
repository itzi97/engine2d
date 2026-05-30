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

# -- Lua 5.4.8 ----------------------------------------------------------------
# Lua has no CMakeLists.txt so we build it manually.
# We fetch 5.4.8 to match the CachyOS/Arch system Lua version, which added
# a third `unsigned seed` argument to lua_newstate().
# CMP0169: suppress FetchContent_Populate deprecation warning.
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

# -- Lua shim -----------------------------------------------------------------
# sol2 does #include <lua.hpp> with angle brackets. The system /usr/include/lua.hpp
# would win that search and pull in a different Lua version. We copy a shim
# lua.hpp plus all fetched Lua headers into build/lua-shim/ and inject that
# directory first on the include path so <lua.hpp> resolves to our 5.4.8 headers.
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

# -- sol2 (develop) -----------------------------------------------------------
# sol2 v3.3.0 does not handle the Lua 5.4.8 lua_newstate(f, ud, seed)
# 3-argument signature. The develop branch contains the fix.
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

# Shim dir must come first so <lua.hpp> resolves to our wrapper, not /usr/include.
target_include_directories(sol2 INTERFACE
    "${LUA_SHIM_DIR}"
    "${lua_SOURCE_DIR}"
)

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

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    "${LUA_SHIM_DIR}"
    "${lua_SOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
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
