# 2D Engine (C++20)

A 2D game engine built from scratch in **C++20**, using SDL3, Lua 5.4, and a cache-friendly ECS.

The engine is scripting-driven: all game logic lives in Lua. The C++ layer owns the ECS, rendering loop, and platform bindings — it knows nothing about any specific game. A Snake prototype is embedded in the binary to validate the full stack.

Licensed under the **MIT License**. See `LICENSE` for details.

## Stack

| Layer | Library |
|---|---|
| Windowing / Rendering / Input | SDL3 |
| Math | GLM 1.0.1 |
| Scripting | Lua 5.4 + sol2 (develop) |
| Map parsing | Tiled JSON + nlohmann/json 3.11 |
| Build / Dependencies | CMake 3.28+ + FetchContent |

All dependencies are fetched and built automatically by CMake — no system libraries required.

## Building

```bash
git clone https://github.com/itzi97/engine2d.git
cd engine2d
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/2d-engine
```

Requires: a C++20 compiler (GCC 12+ or Clang 15+), CMake 3.28+, and a standard build toolchain.

```bash
sudo apt-get install build-essential cmake ninja-build  # Debian/Ubuntu
```

### Windows

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\2d-engine.exe
```

## Architecture

### ECS

Entities are `uint32_t` IDs. The `World` owns one `PackedStorage<T>` per component type:

```
PackedStorage<T>
  vector<T>                       — contiguous component data
  vector<EntityId>                — parallel ownership array
  unordered_map<EntityId, size_t> — O(1) entity → slot lookup
```

Iteration over all components of a type is a single linear memory scan. Deletion is swap-and-pop — the last element fills the freed slot so the array stays packed. The public API is unchanged from the caller's perspective:

```cpp
EntityId e = world.CreateEntity();
world.AddComponent<TransformComponent>(e);
world.GetComponent<TransformComponent>(e)->position = {100, 200};
world.RemoveComponent<TransformComponent>(e);
world.DestroyEntity(e);
```

### Game loop

`core/Game` owns one `World` and one `ScriptingEngine` and drives them from a fixed-timestep loop:

- Fixed update at **60 Hz** (`kFixedDt = 1/60`)
- Accumulator-based timestep to avoid spiral-of-death
- SDL3 event pump → `World::Update` → Lua `on_update` → `World::Render`

### Scripting

`ScriptingEngine` wraps a `sol::state` behind a pimpl so sol2 headers never leak into other translation units. It exposes:

- `BindWorld(World*)` — registers the `world.*` and `engine.*` Lua tables
- `RunString(src, chunkName)` — executes embedded Lua source via `luaL_loadbuffer` + `sol::protected_function`
- `CallOnUpdate(dt)` — calls `engine.on_update(dt)` if registered

### Script embedding

Lua scripts are embedded at **CMake build time** — no files need to exist at runtime. `cmake/EmbedScriptRun.cmake` converts any `.lua` file into a `constexpr char[]` header:

```cmake
embed_script(snake scripts/snake.lua)
# generates build/generated/embedded_snake.hpp
# namespace embedded { inline constexpr char snake[] = "..."; }
```

The header is regenerated automatically whenever the source `.lua` file changes.

## Lua API

Game scripts interact with the engine through two global tables:

### `world`

| Function | Description |
|---|---|
| `world.create_entity()` → id | Create a new entity |
| `world.destroy_entity(id)` | Destroy entity and all its components |
| `world.add_transform(id, x, y, w, h)` | Add a TransformComponent |
| `world.set_position(id, x, y)` | Set position |
| `world.get_position(id)` → x, y | Get position |
| `world.set_velocity(id, vx, vy)` | Set velocity |
| `world.get_velocity(id)` → vx, vy | Get velocity |
| `world.add_sprite(id, r, g, b, a)` | Add a colored rect sprite |
| `world.add_tag(id, tag)` | Add a string tag |
| `world.get_tag(id)` → string | Get tag string |

### `engine`

| Function | Description |
|---|---|
| `engine.on_update(fn)` | Register per-frame callback `fn(dt)` |
| `engine.is_key_pressed(key)` | Poll key state (`"UP"`, `"DOWN"`, `"LEFT"`, `"RIGHT"`, `"W"`, `"S"`, `"A"`, `"D"`, `"ESCAPE"`) |

### Example script

```lua
local e = world.create_entity()
world.add_transform(e, 100, 100, 32, 32)
world.add_sprite(e, 255, 80, 80, 255)
world.add_tag(e, "player")

engine.on_update(function(dt)
  if engine.is_key_pressed("RIGHT") then
    local x, y = world.get_position(e)
    world.set_position(e, x + 200 * dt, y)
  end
end)
```

## Project layout

```
src/
  core/           # Game loop, SDL init (Game.hpp / Game.cpp)
  ecs/            # ECS: Entity, Component, World, PackedStorage
    components/   # TransformComponent, SpriteComponent, TagComponent
  scripting/      # ScriptingEngine (sol2 confined here)
  main.cpp
scripts/
  snake.lua       # Snake game — embedded into the binary at build time
cmake/
  EmbedScript.cmake       # embed_script() function
  EmbedScriptRun.cmake    # build-time .lua → .hpp converter
assets/           # textures, Tiled maps (future)
```

## Controls

- Arrow keys — control snake direction
- Escape / window close — quit

## Status

Working and playable. Short-term roadmap:

- Packed ECS storage ✅
- Lua scripting with embedded scripts ✅
- Snake demo ✅
- Input event system (on_key_down / on_key_up callbacks)
- Texture and font rendering (SDL_image / SDL_ttf)
- Tiled JSON map loading (nlohmann/json already linked)
- Scene management

## Third-party licenses

- **SDL3** — zlib license
- **GLM** — MIT license
- **Lua 5.4** — MIT license
- **sol2** — MIT license
- **nlohmann/json** — MIT license
