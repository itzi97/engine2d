# engine2d

A 2D game engine built from scratch in **C++20**, using SDL3, Lua 5.4 + sol2,
and a cache-friendly flat ECS. All game logic lives in Lua — the C++ layer
owns the ECS, rendering loop, and platform bindings and knows nothing about
any specific game.

Licensed under the **MIT License**.

---

## Stack

| Layer | Library |
|---|---|
| Windowing / Rendering / Input | SDL3 |
| Math | GLM 1.0.1 |
| Scripting | Lua 5.4 + sol2 (develop branch) |
| Map loading | Tiled JSON export + nlohmann/json 3.11 |
| Build / Dependencies | CMake 3.28+ + FetchContent |

---

## Building

**Linux / macOS**
```bash
git clone https://github.com/itzi97/engine2d.git
cd engine2d
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/2d-engine
```

Requires: GCC 12+ or Clang 15+, CMake 3.28+, and a standard build toolchain.

```bash
# Debian / Ubuntu
sudo apt-get install build-essential cmake ninja-build
```

**Windows**
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\2d-engine.exe
```

---

## Running

The engine boots into a main menu. Use **UP / DOWN** to select a game and
**ENTER** to launch it.

| Game / Demo | Controls |
|---|---|
| **Breakout** | LEFT / RIGHT — move paddle · ESCAPE — pause · R — resume · M — main menu |
| **Snake** | Arrow keys — steer · ESCAPE — quit |
| **Audio test** | Keys shown on screen |
| **Sprite test** | Keys shown on screen |

---

## Docs

| Document | Contents |
|---|---|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | ECS design, module boundaries, invariants, conventions |
| [docs/MAIN_LOOP.md](docs/MAIN_LOOP.md) | Frame order, fixed timestep, correctness invariants |
| [docs/LUA_API.md](docs/LUA_API.md) | Full Lua scripting reference with examples |

---

## Status

| Feature | Status |
|---|---|
| Flat ECS with `PackedStorage<T>` | ✅ |
| `sortDirty` flag — sort only on layer change | ✅ |
| Lua scripting via sol2 (isolated to `src/scripting/`) | ✅ |
| Scripting bindings split into focused TUs | ✅ |
| SDL3 audio — OGG/WAV via stb_vorbis | ✅ |
| Texture & font rendering | ✅ |
| Input — `IsKeyJustPressed` via `KEY_DOWN` transitions | ✅ |
| Scene management (`engine.load_scene`) | ✅ |
| Tiled JSON map loading (`world.load_tiled_map`) | ✅ |
| Main menu + Breakout (2 levels) + Snake | ✅ |
| Spatial hash broadphase (replaces O(n²)) | 🔲 |
| Lua-side `world.spawn_from_map` helper | 🔲 |

---

## Third-party licenses

- **SDL3** — zlib
- **GLM** — MIT
- **Lua 5.4** — MIT
- **sol2** — MIT
- **nlohmann/json** — MIT
- **stb_vorbis** — public domain / MIT
