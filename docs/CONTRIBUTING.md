# Contributing

## Code Style

- **C++20** — concepts, ranges, `std::span` freely. No modules yet.
- **No Boost, no physics library.** AABB collision is manual.
- `std::unique_ptr` over raw owning pointers everywhere.
- Prefer free functions over static member functions for systems.
- Public headers must not include `<sol/sol.hpp>` — forward-declare via the
  namespace stub in `scripting/` if sol state is needed.
- New component types: header-only structs in `src/ecs/components/`. No `.cpp`
  unless there is non-trivial implementation.
- New systems: free function `void XSystem::Update(World&, ...)` in
  `src/ecs/systems/`. Register in `World::Update` or `World::Render`.

## Naming Conventions

| Thing | Convention | Example |
|---|---|---|
| Types / structs | `PascalCase` | `PackedStorage`, `TransformComponent` |
| Functions / methods | `PascalCase` | `CreateEntity()`, `ForEachSorted()` |
| Member variables | `m_camelCase` | `m_storages`, `m_nextId` |
| Local variables | `camelCase` | `sortOrder`, `deltaTime` |
| Constants | `kPascalCase` | `kFixedDt` |
| Template params | Single uppercase or short `PascalCase` | `T`, `Fn`, `ComponentType` |

## Commit Style

Conventional Commits with a scope:

```
<type>(<scope>): <short description>

<optional body>
```

| Type | When |
|---|---|
| `feat` | New feature or system |
| `fix` | Bug fix |
| `refactor` | Code restructure with no behaviour change |
| `chore` | Build system, deps, tooling |
| `docs` | Documentation only |
| `perf` | Performance improvement |

**Scopes:** `ecs`, `scripting`, `audio`, `input`, `rendering`, `core`, `build`, `docs`

Examples:
```
feat(ecs): add multi-component ForEach<A,B> view
fix(input): EndFrame before SDL_PollEvent to fix IsKeyJustPressed
refactor(scripting): split Impl into per-domain binding TUs
chore(build): add scripting/bindings/* to ENGINE_SOURCES
```

## Pull Request Flow

1. Branch from `main` — use `feature/`, `fix/`, or `refactor/` prefix.
2. One logical change per PR. Keep diffs reviewable.
3. Update `docs/` if the change affects architecture, loop order, or Lua API.
4. Update the Status table in `README.md` if a roadmap item is completed.
5. Squash fixup commits before merging.

## Adding a New Binding

1. Create `src/scripting/bindings/BindFoo.hpp` — forward-declare only, no sol2 include.
2. Create `src/scripting/bindings/BindFoo.cpp` — include `SOL_ALL_SAFETIES_ON`,
   then `<sol/sol.hpp>`, then implement.
3. Call `::BindFoo(m_impl->lua, ...)` from `ScriptingEngine.cpp`.
4. Add `BindFoo.cpp` to `ENGINE_SOURCES` in `CMakeLists.txt`.
5. Document new functions in `docs/LUA_API.md`.

## Known Invariants (Do Not Break)

- `PackedStorage::index[e]` must always equal the current slot of entity `e`.
  Every mutation path must keep `index` in sync.
- `InputManager::EndFrame()` must run before `SDL_PollEvent` each frame.
- `World::FlushDestroyQueue()` must run after all per-frame reads and before
  the next frame's `Update`.
- sol2 headers (`<sol/sol.hpp>`) must never appear outside `src/scripting/`.
