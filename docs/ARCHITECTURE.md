# Architecture

engine2d is structured as a scripting-driven game engine. The C++ layer owns
platform bindings, the ECS, and the render/audio pipelines. It has no
knowledge of any specific game — all game logic lives in Lua scripts.

## Module Boundaries

```
src/
  core/        Game loop, SDL init, window management (Game.hpp / Game.cpp)
  ecs/         ECS kernel: Entity, World, PackedStorage, component types, systems
  scripting/   sol2 confinement layer — sol2 headers never leave this directory
    bindings/  One .hpp/.cpp pair per binding group (BindWorld, BindEngine, …)
  input/       InputManager: key state + per-frame transition tracking
  audio/       AudioManager: SDL3 audio streams, OGG/WAV via stb_vorbis
  rendering/   TextureManager, FontManager, SDL_Renderer helpers
  map/         TiledMap data structs + MapLoader (nlohmann/json → TiledMap)
```

**Isolation rule:** `scripting/` is the only directory allowed to include
`<sol/sol.hpp>`. All public binding headers forward-declare `sol::state` via
a minimal namespace stub so sol2's compile-time cost stays inside its own TUs.

---

## ECS

### Entities

An entity is a `uint32_t` ID. `World` issues IDs from a monotonic counter and
recycles retired IDs through a free list.

```cpp
EntityId e = world.CreateEntity();   // O(1)
world.DestroyEntity(e);              // deferred — see Destroy Queue below
```

### PackedStorage\<T\>

Each component type gets exactly one `PackedStorage<T>`, owned by `World` and
keyed by `std::type_index`. The storage keeps three parallel arrays:

```
components[]   — contiguous T values           (cache-friendly iteration)
entities[]     — parallel EntityId ownership
index{}        — unordered_map<EntityId, size_t>   O(1) lookup
```

**Deletion is swap-and-pop.** The removed slot is filled by the last element
and `index` is updated. This keeps the arrays fully packed at all times.

**Invariant:** `index[e]` is always the correct slot for entity `e`.
Every mutation path (`Add`, `Erase`, `Clear`) must maintain this.

### Sorted Iteration

`ForEachSorted` iterates components in ascending `layer` order for rendering.
It maintains a `sortOrder` index array and a `sortDirty` flag. The
`stable_sort` only runs when `sortDirty == true` — i.e. when a layer value
changes, an entity is added/removed, or `World::MarkSortDirty<T>()` is called
explicitly.

If you mutate a component's `.layer` field directly (outside of `Add`/`Erase`),
you **must** call `world.MarkSortDirty<T>()` to invalidate the cached order.

### Destroy Queue

`DestroyEntity` pushes to `m_pendingDestroy`. Destruction is deferred until
`World::FlushDestroyQueue()`, which runs at the end of each frame after
collision and scripting. This prevents mid-iteration invalidation.

**Never read from a destroyed entity's components in the same frame after
`FlushDestroyQueue` has run.**

### Multi-component queries

The current API provides single-type `ForEach<T>` and `View<T>`. For queries
that span two component types, iterate the smaller storage and probe the other
with `GetComponent<U>(e)`:

```cpp
world.ForEach<KinematicComponent>([&](EntityId e, auto& kin) {
    if (auto* tr = world.GetComponent<TransformComponent>(e))
        // both components present
});
```

---

## Scripting Layer

`ScriptingEngine` wraps a `sol::state` behind a pimpl (`ScriptingEngine::Impl`)
so the rest of the engine never sees a sol2 header. The monolithic `Impl` has
been split into focused translation units:

| File | Registers |
|---|---|
| `bindings/BindWorld.cpp` | `world.*` table — entities, components, map loading |
| `bindings/BindEngine.cpp` | `engine.*` input, lifecycle, `load_scene`, quit |
| `bindings/BindTextures.cpp` | `engine.load_texture` |
| `bindings/BindAudio.cpp` | `engine.play_music`, `engine.play_sfx`, etc. |
| `bindings/KeycodeFromString.cpp` | Uppercase key name → `SDL_Keycode` helper |

`ScriptingEngine.cpp` is ~80 lines of Impl plumbing + `CallOnUpdate` +
`RunScript`/`RunString`. Each binding TU includes its own
`SOL_ALL_SAFETIES_ON` + `<sol/sol.hpp>` so incremental rebuilds stay fast.

`BindAudio` extends the existing `engine` table rather than recreating it, so
binding order does not matter.

### on_update contract

`engine.on_update` must be assigned by the Lua script during its initial
execution (before the first frame). `ScriptingEngine::CallOnUpdate(dt)` is a
no-op if `onUpdateFn` is not yet valid — it will not crash, but input and
game logic will not run.

**Debug tip:** if callbacks are not firing, add a one-shot C++ log:
```cpp
std::cout << "[Engine] on_update valid: " << m_impl->onUpdateFn.valid() << '\n';
```
If this prints `0` on frame 1, `RunScript` is executing after the first
update tick, or `BindInput` was called after `RunScript`.

---

## Map Loading

`MapLoader::Load(path)` parses a Tiled `.tmj` file into a `TiledMap` struct:

```
TiledMap
  tilesets[]     — image path, firstGid, tile dimensions
  tileLayers[]   — flat GID data arrays
  objectLayers[] — named object layers with MapObject entries
```

`MapObject` carries `id`, `name`, `type` (from Tiled `class` field), `x`, `y`,
`w`, `h`, and a `properties` string map (custom properties from Tiled).

`BindWorld` exposes the map to Lua via `world.load_tiled_map(path)`, which
returns a table keyed by object name. Each entry has `.type`, `.x`, `.y`,
`.w`, `.h`, and `.properties` (a sub-table of string → string pairs).

Game scripts are responsible for interpreting objects and spawning ECS
entities. This keeps the C++ loader data-format-agnostic.

---

## Audio

`AudioManager` uses SDL3 audio streams. Music is fed via a stream callback;
SFX clips are decoded once at load time (OGG via `stb_vorbis_decode_filename`
to `short*`, converted to `float` — not `float**`; WAV via `SDL_LoadWAV`)
and played by submitting samples to a stream.

Volume scaling uses `SDL_SetAudioStreamGain`. The music callback uses a
fixed-size stack buffer to avoid heap allocation on the audio thread.

`stb_vorbis` is compiled as a C translation unit (`stb_vorbis_impl.c`) with
`-w` to suppress third-party warnings.

---

## Collision

Collision detection runs as a broadphase + AABB check in `World::RunCollision()`.
The current broadphase is **O(n²)** over all entities with a
`TransformComponent` — there is a `TODO` comment marking this location.
Results are stored as `vector<Collision>` on `World` and queried by Lua via
`world.get_collisions_for` / `world.get_collisions_tagged`.

**Planned replacement:** a spatial hash broadphase. Cell size ~2× average
entity size; `QueryPairs()` deduplicates candidate pairs via an ordered-key
`unordered_set` to avoid testing the same pair twice.

---

## Known Invariants

Things that must stay true. Break any of these and the engine misbehaves
silently rather than crashing loudly.

- **`PackedStorage` index sync** — `index[e]` must equal the current slot of
  entity `e` at all times. Every mutation path (`Add`, `Erase`, `Clear`) must
  update `index`.

- **`EndFrame` before poll** — `InputManager::EndFrame()` must run before
  `SDL_PollEvent` each frame, or `IsKeyJustPressed` will always return false.

- **`FlushDestroyQueue` after all reads** — destroyed entities are still alive
  until `FlushDestroyQueue` runs. Never read from a destroyed entity's
  components after flushing has occurred in the same frame.

- **sol2 stays in `src/scripting/`** — `<sol/sol.hpp>` must never appear
  outside `src/scripting/`. Forward-declare `sol::state` via the namespace
  stub if a header outside scripting needs to reference it.

- **`sortDirty` on direct layer mutation** — if you write to a component's
  `.layer` field directly (not via `Add`), call `world.MarkSortDirty<T>()`
  or render order will silently use stale data.

---

## Conventions

### Naming

| Thing | Style | Example |
|---|---|---|
| Types / structs | `PascalCase` | `PackedStorage`, `TransformComponent` |
| Functions / methods | `PascalCase` | `CreateEntity()`, `ForEachSorted()` |
| Member variables | `m_camelCase` | `m_storages`, `m_nextId` |
| Local variables | `camelCase` | `sortOrder`, `deltaTime` |
| Constants | `kPascalCase` | `kFixedDt` |
| Template params | Short uppercase | `T`, `Fn` |

### New component

Header-only struct in `src/ecs/components/`. No `.cpp` unless there is
non-trivial implementation. Empty `.cpp` files must not be committed —
remove them and update `CMakeLists.txt`.

### New system

Free function `void XSystem::Update(World&, ...)` in `src/ecs/systems/`.
Register the call in `World::Update` or `World::Render`.

### New Lua binding

1. `src/scripting/bindings/BindFoo.hpp` — forward-declare only, no sol2 include.
2. `src/scripting/bindings/BindFoo.cpp` — `SOL_ALL_SAFETIES_ON` + `<sol/sol.hpp>` + implementation.
3. Call `::BindFoo(m_impl->lua, ...)` from `ScriptingEngine.cpp`.
4. Add `BindFoo.cpp` to `ENGINE_SOURCES` in `CMakeLists.txt`.
5. Document new functions in `docs/LUA_API.md`.

If the binding extends an existing table (like `engine`), use
`lua["engine"]` to retrieve the table rather than recreating it — see
`BindAudio.cpp` for the pattern.

### Commit style

```
<type>(<scope>): <short description>
```

Types: `feat`, `fix`, `refactor`, `chore`, `docs`, `perf`  
Scopes: `ecs`, `scripting`, `audio`, `input`, `rendering`, `core`, `map`, `build`, `docs`
