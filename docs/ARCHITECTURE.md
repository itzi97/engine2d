# Architecture

engine2d is structured as a scripting-driven game engine. The C++ layer owns
platform bindings, the ECS, and the render/audio pipelines. It has no knowledge
of any specific game — all game logic lives in Lua scripts.

## Module Boundaries

```
src/
  core/        Game loop, SDL init, window management (Game.hpp / Game.cpp)
  ecs/         ECS kernel: Entity, World, PackedStorage, component types, systems
  scripting/   sol2 confinement layer — sol2 headers never leave this directory
  input/       InputManager: key state + per-frame transition tracking
  audio/       AudioManager: SDL3 audio streams, OGG/WAV via stb_vorbis
  rendering/   TextureManager, FontManager, SDL_Renderer helpers
```

**Isolation rule:** `scripting/` is the only directory allowed to include
`<sol/sol.hpp>`. All public binding headers forward-declare `sol::state` via a
minimal namespace stub so sol2's compile-time cost stays inside its own TUs.

---

## ECS

### Entities

An entity is a `uint32_t` ID. The `World` issues IDs from a monotonic counter
and recycles retired IDs through a free list.

```cpp
EntityId e = world.CreateEntity();   // O(1)
world.DestroyEntity(e);              // deferred — see Destroy Queue below
```

### PackedStorage\<T\>

Each component type gets exactly one `PackedStorage<T>`, owned by `World` and
keyed by `std::type_index`. The storage keeps three parallel arrays:

```
components[]   — contiguous T values          (cache-friendly iteration)
entities[]     — parallel EntityId ownership
index{}        — unordered_map<EntityId, size_t>  O(1) lookup
```

**Deletion is swap-and-pop.** The removed slot is filled by the last element,
and `index` is updated. This keeps the arrays fully packed at all times.

**Invariant:** `index[e]` is always the correct slot for entity `e`.
Every mutation path (`Add`, `Erase`, `Clear`) must maintain this.

### Sorted Iteration

`ForEachSorted` iterates components in ascending `layer` order for rendering.
It maintains a `sortOrder` index array and a `sortDirty` flag. The
`stable_sort` only runs when `sortDirty == true` — i.e. when a layer value
changes, an entity is added/removed, or `MarkSortDirty()` is called explicitly.

If you mutate a component's `.layer` field directly (outside of `Add`/`Erase`),
you **must** call `world.MarkSortDirty<T>()` to invalidate the cached order.

### Destroy Queue

`DestroyEntity` pushes to `m_pendingDestroy`. Destruction is deferred until
`World::FlushDestroyQueue()`, which runs at the end of each frame after
collision and scripting. This prevents mid-iteration invalidation.

**Never call `DestroyEntity` and then read from that entity's components in
the same frame before `FlushDestroyQueue` has run.**

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
so the rest of the engine never sees a sol2 header.

Binding is split into focused translation units:

| File | Registers |
|---|---|
| `bindings/BindWorld.cpp` | `world.*` table |
| `bindings/BindEngine.cpp` | `engine.*` input / lifecycle / quit |
| `bindings/BindTextures.cpp` | `engine.load_texture` |
| `bindings/BindAudio.cpp` | `engine.play_music`, `engine.play_sfx`, etc. |
| `bindings/KeycodeFromString.cpp` | Key name → SDL_Keycode helper |

Each TU includes its own `SOL_ALL_SAFETIES_ON` + `<sol/sol.hpp>` so the
sol2 compilation cost is distributed and incremental rebuilds stay fast.

### on_update contract

`engine.on_update` must be assigned by the Lua script during its initial
execution (before the first frame). `ScriptingEngine::CallOnUpdate(dt)` is a
no-op if `onUpdateFn` is not yet valid — it will not crash, but input and game
logic will not run. If callbacks are not firing, verify that `BindInput` is
called before `RunScript`.

---

## Audio

`AudioManager` uses SDL3 audio streams. Music is fed via a stream callback;
SFX clips are decoded once at load time (OGG via `stb_vorbis`, WAV via
`SDL_LoadWAV`) and played by submitting samples to a stream.

Volume scaling uses `SDL_SetAudioStreamGain` — no per-trigger sample copy.
The music callback uses a fixed-size stack buffer to avoid heap allocation on
the audio thread.

---

## Collision

Collision detection runs as a broadphase + AABB check in `CollisionSystem`.
The current broadphase is O(n²) over all entities with a `TransformComponent`
(marked with a TODO). Results are stored as `vector<Collision>` on `World` and
queried by Lua via `engine.get_collisions_tagged`.

A spatial hash broadphase is the planned replacement.

---

## Known Invariants

Things that must stay true. Break any of these and the engine misbehaves
silently rather than crashing loudly.

- **`PackedStorage` index sync** — `index[e]` must equal the current slot of
  entity `e` at all times. Every mutation path (`Add`, `Erase`, `Clear`) must
  update `index`. If you add a new mutation, update `index` too.

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

Quick reference for staying consistent across the codebase.

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
non-trivial implementation.

### New system

Free function `void XSystem::Update(World&, ...)` in `src/ecs/systems/`.
Register the call in `World::Update` or `World::Render`.

### New Lua binding

1. `src/scripting/bindings/BindFoo.hpp` — forward-declare only, no sol2 include.
2. `src/scripting/bindings/BindFoo.cpp` — `SOL_ALL_SAFETIES_ON` + `<sol/sol.hpp>` + implementation.
3. Call `::BindFoo(m_impl->lua, ...)` from `ScriptingEngine.cpp`.
4. Add `BindFoo.cpp` to `ENGINE_SOURCES` in `CMakeLists.txt`.
5. Document new functions in `docs/LUA_API.md`.

### Commit style

```
<type>(<scope>): <short description>
```

Types: `feat`, `fix`, `refactor`, `chore`, `docs`, `perf`  
Scopes: `ecs`, `scripting`, `audio`, `input`, `rendering`, `core`, `build`, `docs`
