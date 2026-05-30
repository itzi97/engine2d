# 2D Engine (C++20)

A 2D game engine built from scratch in **C++20**.

## Stack

| Layer | Library |
|---|---|
| Windowing / Rendering / Input | SDL3 |
| Math | GLM |
| Scripting | sol2 + Lua 5.4 |
| Map parsing | Tiled JSON + nlohmann/json |
| Build | CMake 3.28+ with FetchContent |

## Architecture

Entity-Component System (ECS) with flat component storages keyed by `std::type_index`. Entities are `uint32_t` IDs. The `World` struct owns all storages via `std::unique_ptr`. All sol2 includes are isolated to `src/scripting/ScriptingEngine.cpp`.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/2d-engine
```

Press **Escape** or close the window to exit.
