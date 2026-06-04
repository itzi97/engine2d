# Main Loop

## Frame Order

The following order is **not arbitrary** — each step has a correctness
dependency on the one before it.

```
1. InputManager::EndFrame()               — clear previous-frame transition flags
2. SDL_PollEvent loop                     — feed events into InputManager; catch SDL_EVENT_QUIT
3. World::Update(dt)                      — PhysicsSystem → AnimationSystem
4. ScriptingEngine::CallOnUpdate(dt)      — Lua engine.on_update callback
5. World::RunCollision()                  — SpatialHash broadphase + AABB narrow-phase
6. World::FlushDestroyQueue()             — apply all pending DestroyEntity calls
7. Render + SDL_RenderPresent
```

### Why EndFrame comes first

`InputManager::EndFrame()` clears the previous frame's key-transition flags
(`justPressed`, `justReleased`). It must run **before** `SDL_PollEvent` so
that new `KEY_DOWN` / `KEY_UP` events captured this frame are not immediately
cleared. If `EndFrame` ran after polling, `IsKeyJustPressed` would always
return false.

`IsKeyJustPressed` is driven from `SDL_EVENT_KEY_DOWN` event transitions, not
snapshot diffing — it fires exactly once per physical key press regardless of
frame rate.

### Why collision runs after scripting

Scripts may reposition entities (via `world.set_position`) during `on_update`.
Running collision after scripting ensures results reflect the positions
entities actually occupy at end-of-logic, not stale positions from the
previous frame.

### Why FlushDestroyQueue comes after scripting

Lua `on_update` callbacks can call `world.destroy_entity`. The entity must
still be alive during the current frame's collision query and any other
per-frame reads. `FlushDestroyQueue` applies all pending destroys at the end
of the logic phase, before rendering.

---

## Timestep

The loop uses a **variable timestep** — raw wall-clock delta time between
frames, capped at 50 ms to prevent the spiral-of-death on very slow frames
(e.g. a debugger pause or OS scheduler hiccup).

```cpp
const float dt = std::min(
    static_cast<float>(duration<double>(now - previous).count()),
    0.05f);   // cap: never simulate more than 50 ms in one tick
```

This means physics and animation run at the actual frame rate. For games where
physics determinism or a stable simulation rate matters, a fixed-timestep
accumulator can be layered in by wrapping `World::Update(kFixedDt)` in the
standard accumulator pattern — but this is not currently implemented.

---

## SDL Event Handling

`SDL_EVENT_QUIT` (window close) is caught in the poll loop and sets a quit
flag that exits the main loop cleanly.

`InputManager` receives all `SDL_EVENT_KEY_DOWN` and `SDL_EVENT_KEY_UP`
events and updates its state maps. Key names are looked up via
`KeycodeFromString` (uppercase string → `SDL_Keycode`) so Lua scripts can use
human-readable names (`"ESCAPE"`, `"RETURN"`, `"LEFT"`, etc.) without
knowledge of SDL constants.

---

## Scene Transitions

`engine.load_scene(fn)` is the safe way to switch scenes from Lua. Internally
it schedules a scene reload: at the end of the current frame the engine calls
`World::ClearAll()` (destroys all entities and flushes storages) and then
executes `fn()` as the new scene's init function. This avoids destroying
entities mid-frame while `on_update` is still running.
