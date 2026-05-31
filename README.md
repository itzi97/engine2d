# 2D Engine (C++20)

A 2D game engine built from scratch in **C++20**, focused on a simple ECS architecture, SDL3 rendering, and Lua scripting.

Currently, the engine drives a simple **Snake** prototype to validate the ECS, rendering, input, and timing model.

Licensed under the **MIT License**. See `LICENSE` for details.

## Stack

| Layer                        | Library             |
|-----------------------------|---------------------|
| Windowing / Rendering / Input | SDL3              |
| Math                        | GLM                 |
| Scripting                   | sol2 + Lua 5.4      |
| Map parsing                 | Tiled JSON + nlohmann/json |
| Build / Dependencies        | CMake 3.28+ with FetchContent |

## Architecture

The engine uses an Entity-Component System (ECS) with flat component storages keyed by `std::type_index`.

- **Entities** are `uint32_t` IDs created via `Entity::Create()`.
- **Components** derive from a common `Component` base and are stored in `ComponentStorage = std::unordered_map<EntityId, std::unique_ptr<Component>>` per component type.
- The **World** owns all component storages and is responsible for updating and rendering them each frame.
- **Game loop** is implemented in `core/Game.{hpp,cpp}`:
  - SDL3 window + renderer
  - Fixed timestep update at 60 FPS (`kFixedDt = 1.0f / 60.0f`)
  - Variable rendering with an accumulator to avoid the “spiral of death”.
- **Scripting** is isolated behind a `ScriptingEngine` in `src/scripting/` to avoid sol2 compile-time bleed into the rest of the codebase.

The goal is to keep the core small and explicit: the `Game` object owns one `World` and one `ScriptingEngine`, and drives them from a central loop.

### Current gameplay demo: Snake

The current demo is a minimal, classic grid-based **Snake** implementation used to exercise the engine:

- The **snake head** is an entity with:
  - `TransformComponent` (grid-aligned position and size)
  - `SpriteComponent` (rendered as a green square)
  - `TagComponent` with `"snake_head"`
- **Food** is a separate entity with:
  - `TransformComponent` (grid-aligned position and size)
  - `SpriteComponent` (rendered as a red square)
  - `TagComponent` with `"food"`
- Movement is handled via:
  - A unit grid direction stored in the transform (velocity `{±1,0}` or `{0,±1}`)
  - A fixed **snake step time** (e.g. 10 moves per second), moving the head exactly one cell per step
  - 180° reversals are prevented by rejecting direction changes that are exact opposites of the current direction.

Snake logic lives in `Game` for now as a C++ prototype and is intended to be moved into Lua scripts once the engine-side API (entity creation, component access, input, timing) is stable.

## Prerequisites

You need the following tools installed locally:

- A C++20-capable compiler
  - GCC 12+ or Clang 15+ are recommended
- **CMake 3.28+** (used to configure and build the project)
- **SDL3**, **GLM**, **Lua 5.4** development headers, and a working link setup if your platform does not use prebuilt packages
- A build toolchain supported by CMake (e.g. Ninja, Make, MSBuild)

On Debian/Ubuntu-like systems, the basic toolchain can be installed with:

```bash
sudo apt-get install build-essential cmake ninja-build
```

Library installation will depend on your distro and how you prefer to obtain SDL3, Lua, and GLM (system packages vs. custom builds).

## Cloning

Clone the repository and enter the project directory:

```bash
git clone https://github.com/itzi97/engine2d.git
cd engine2d
```

## Building and running

From the project root:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/2d-engine
```

- The CMake target produces a single `2d-engine` executable in the `build/` directory by default.
- Use `CMAKE_BUILD_TYPE=Release` for an optimized build:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Windows notes

On Windows, run CMake from a “x64 Native Tools Command Prompt“ for Visual Studio, then:

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
.\build\Debug\2d-engine.exe
```

Adjust the config (`Debug`/`Release`) as needed.

## Controls / runtime behavior

- A window titled **“2D Engine”** is created at 1280×720 by default.
- Press **Escape** or close the window to exit the main loop.
- The background currently clears to a dark gray, and the `World` is responsible for updating and rendering all components each frame.
- The Snake demo currently consists of:
  - A snake head that moves one grid cell at a time in response to arrow keys.
  - A food block that spawns at a fixed grid position at startup.
  - (Work in progress) eating food and growing the snake body.

## Project layout

High‑level source layout:

```text
src/
  core/       # Game loop, SDL initialization, main engine entry (includes Snake demo for now)
  ecs/        # ECS primitives and components (Entity, Component, World, Transform, Sprite, Tag, ...)
  scripting/  # Lua / sol2 bindings (ScriptingEngine)
  main.cpp    # entry point that creates and runs Game
assets/
  ...         # textures, maps, and other runtime data (Tiled JSON etc.)
```

The intention is to keep SDL- and Lua-specific code isolated to `core/` and `scripting/` respectively, so the ECS layer remains decoupled from platform details.

## Third‑party libraries

This project uses the following third‑party libraries and tools:

- **SDL3** – windowing, rendering, input, and basic platform abstraction. Licensed under the zlib license.
- **GLM** – header‑only math library for vectors, matrices, and transforms. Licensed under the MIT license.
- **Lua 5.4** – embedded scripting language used for gameplay scripting. Licensed under the MIT license.
- **sol2** – C++17/20 bindings between Lua and C++. Licensed under the MIT license.
- **nlohmann/json** – header‑only JSON library used for parsing Tiled maps. Licensed under the MIT license.
- **CMake** – build system generator.

You are responsible for complying with the licenses of these dependencies when using or redistributing this project.

## Status

This project is a work in progress. The short‑term roadmap includes:

- Flesh out core components (Transform, Sprite, Tag, simple motion, etc.)
- Finalize a thin C++ API for entity/component access, input, and timing
- Move the Snake demo game logic into Lua as a scripted example
- Add basic tilemap rendering (Tiled JSON) and collision for more complex demos

Contributions and feedback are welcome once the public API stabilizes.
