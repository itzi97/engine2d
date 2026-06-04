# engine2d

A 2D game engine built from scratch in **C++20**, using SDL3, Lua 5.4, and a cache-friendly ECS.

All game logic lives in Lua. The C++ layer owns the ECS, rendering loop, and platform bindings — it knows nothing about any specific game.

Licensed under the **MIT License**.

## Stack

| Layer | Library |
|---|---|
| Windowing / Rendering / Input | SDL3 |
| Math | GLM 1.0.1 |
| Scripting | Lua 5.4 + sol2 (develop) |
| Map parsing | Tiled JSON + nlohmann/json 3.11 |
| Build / Dependencies | CMake 3.28+ + FetchContent |

## Building

```bash
git clone https://github.com/itzi97/engine2d.git
cd engine2d
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/2d-engine
```

Requires: GCC 12+ or Clang 15+, CMake 3.28+, and a standard build toolchain.

```bash
# Debian/Ubuntu
sudo apt-get install build-essential cmake ninja-build
```

**Windows:**
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\2d-engine.exe
```

## Controls (Snake demo)

- Arrow keys — control direction
- Escape / window close — quit

## Docs

| Document | Contents |
|---|---|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | ECS design, module boundaries, storage internals |
| [docs/MAIN_LOOP.md](docs/MAIN_LOOP.md) | Frame order, fixed timestep, correctness invariants |
| [docs/LUA_API.md](docs/LUA_API.md) | Full Lua scripting reference with examples |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Coding conventions, commit style, PR flow |

## Status

| Feature | Status |
|---|---|
| Flat ECS with PackedStorage | ✅ |
| Lua scripting (embedded scripts) | ✅ |
| SDL3 audio (OGG/WAV) | ✅ |
| Texture & font rendering | ✅ |
| Snake demo | ✅ |
| Spatial hash broadphase | 🔲 |
| Tiled JSON map loading | 🔲 |
| Scene management | 🔲 |

## Third-party licenses

- **SDL3** — zlib | **GLM** — MIT | **Lua 5.4** — MIT | **sol2** — MIT | **nlohmann/json** — MIT
