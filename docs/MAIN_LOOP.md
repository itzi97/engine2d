# Main Loop

## Frame Order

The following order is **not arbitrary** — each step has a correctness
dependency on the one before it.

```
1. InputManager::EndFrame()
2. SDL_PollEvent loop
3. World::Update(dt)          — PhysicsSystem → AnimationSystem
4. ScriptingEngine::CallOnUpdate(dt)  — Lua engine.on_update
5. World::RunCollision()
6. World::FlushDestroyQueue()
7. Render + SDL_RenderPresent
```

### Why EndFrame comes first

`InputManager::EndFrame()` clears the previous frame's key-transition flags
(`justPressed`, `justReleased`). It must run **before** `SDL_PollEvent` so
that new `KEY_DOWN`/`KEY_UP` events captured this frame are not immediately
cleared. If `EndFrame` ran after polling, `IsKeyJustPressed` would always
return false.

### Why FlushDestroyQueue comes after scripting

Lua `on_update` callbacks can call `world.destroy_entity`. The entity must
still be alive during the current frame's collision query and any other
per-frame reads. `FlushDestroyQueue` applies all pending destroys at the end
of the logic phase, before rendering, so no system reads a partially-destroyed
entity mid-frame.

### Why collision runs after scripting

Scripts may reposition entities (via `world.set_position`) during `on_update`.
Running collision after scripting ensures the collision results reflect the
positions entities actually occupy at end-of-logic, not stale positions from
the previous frame.

---

## Fixed Timestep

The loop uses an **accumulator-based fixed timestep** at 60 Hz (`kFixedDt = 1/60 s`).

```
accumulator += rawDeltaTime
while accumulator >= kFixedDt:
    Update(kFixedDt)
    accumulator -= kFixedDt
```

This decouples physics/animation update rate from render frame rate and
prevents the spiral-of-death (where a slow frame causes a larger dt which
causes more work which causes an even slower frame).

**Consequence:** on a very slow machine, multiple `Update` ticks may run per
render frame. Systems must not assume one update = one rendered frame.

---

## SDL Event Handling

`SDL_EVENT_QUIT` (window close button) is caught in the poll loop and sets
a quit flag that exits the main loop cleanly. `InputManager` receives all
`SDL_EVENT_KEY_DOWN` and `SDL_EVENT_KEY_UP` events and updates its state maps.

Key transition tracking (`IsKeyJustPressed`) is driven from `KEY_DOWN` event
transitions, not snapshot diffing — this means it fires exactly once per
physical key press regardless of frame rate.
