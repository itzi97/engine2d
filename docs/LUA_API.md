# Lua API Reference

Game scripts interact with the engine through two global tables: `world` and
`engine`. All game logic lives in Lua — the C++ layer has no knowledge of
specific games.

## Lifecycle

A script runs once at startup. Side effects from that execution (entity
creation, registering `engine.on_update`) persist for the lifetime of the
engine session.

```lua
-- Setup: runs once
local player = world.create_entity()
world.add_transform(player, 100, 100, 32, 32)
world.add_sprite(player, 255, 80, 80, 255)

-- Per-frame: called every frame with delta time in seconds
engine.on_update(function(dt)
    -- game logic here
end)
```

`engine.on_update` must be assigned during the script's initial execution.
If it is not set, no per-frame logic will run.

---

## `world` table

### Entity lifecycle

| Function | Returns | Description |
|---|---|---|
| `world.create_entity()` | `id` | Create a new entity |
| `world.destroy_entity(id)` | — | Destroy entity (deferred to end of frame) |

### Transform

| Function | Returns | Description |
|---|---|---|
| `world.add_transform(id, x, y, w, h)` | — | Add a TransformComponent |
| `world.set_position(id, x, y)` | — | Set world position |
| `world.get_position(id)` | `x, y` | Get world position |
| `world.set_velocity(id, vx, vy)` | — | Set velocity (units/sec) |
| `world.get_velocity(id)` | `vx, vy` | Get velocity |

### Sprite

| Function | Returns | Description |
|---|---|---|
| `world.add_sprite(id, r, g, b, a)` | — | Add a solid-color rect sprite |
| `world.add_texture_sprite(id, tex, sx, sy, sw, sh)` | — | Add a textured sprite with source rect |
| `world.set_layer(id, layer)` | — | Set render layer (lower = drawn first) |

### Tags

| Function | Returns | Description |
|---|---|---|
| `world.add_tag(id, tag)` | — | Assign a string tag to an entity |
| `world.get_tag(id)` | `string` | Get the entity's tag |

### Collisions

| Function | Returns | Description |
|---|---|---|
| `world.get_collisions_tagged(tag)` | `table` of `{a, b}` pairs | All collisions involving an entity with the given tag |

---

## `engine` table

### Lifecycle

| Function | Description |
|---|---|
| `engine.on_update(fn)` | Register per-frame callback. `fn` receives `dt` (seconds). |
| `engine.quit()` | Request engine shutdown at end of current frame. |

### Input

| Function | Returns | Description |
|---|---|---|
| `engine.is_key_pressed(key)` | `bool` | True while key is held |
| `engine.is_key_just_pressed(key)` | `bool` | True only on the first frame the key is down |

**Key names:** `"UP"`, `"DOWN"`, `"LEFT"`, `"RIGHT"`, `"W"`, `"A"`, `"S"`, `"D"`,
`"SPACE"`, `"ESCAPE"`, `"RETURN"`, `"LSHIFT"`, `"RSHIFT"` — and any SDL canonical
key name accepted by `SDL_GetKeyFromName`.

### Textures

| Function | Returns | Description |
|---|---|---|
| `engine.load_texture(path)` | `tex` handle | Load a texture from disk. Returns a handle for use with `add_texture_sprite`. |

### Audio

| Function | Description |
|---|---|
| `engine.play_music(path)` | Start looping background music from file path |
| `engine.stop_music()` | Stop background music |
| `engine.play_sfx(path [, volume])` | Play a one-shot sound effect. `volume` is 0.0–1.0, default 1.0 |
| `engine.set_music_volume(v)` | Set music volume (0.0–1.0) |

---

## Full Example

```lua
-- Load assets
local tex = engine.load_texture("assets/player.png")
engine.play_music("assets/bgm.ogg")

-- Spawn player
local player = world.create_entity()
world.add_transform(player, 100, 100, 32, 32)
world.add_texture_sprite(player, tex, 0, 0, 32, 32)
world.add_tag(player, "player")
world.set_layer(player, 1)

-- Spawn a wall
local wall = world.create_entity()
world.add_transform(wall, 200, 100, 32, 32)
world.add_sprite(wall, 80, 80, 255, 255)
world.add_tag(wall, "wall")

engine.on_update(function(dt)
    -- Movement
    local speed = 150
    local vx, vy = 0, 0
    if engine.is_key_pressed("RIGHT") then vx =  speed end
    if engine.is_key_pressed("LEFT")  then vx = -speed end
    if engine.is_key_pressed("DOWN")  then vy =  speed end
    if engine.is_key_pressed("UP")    then vy = -speed end
    world.set_velocity(player, vx, vy)

    -- Quit on Escape
    if engine.is_key_just_pressed("ESCAPE") then
        engine.quit()
    end

    -- Collision response
    for _, col in ipairs(world.get_collisions_tagged("player")) do
        local other = col.a == player and col.b or col.a
        if world.get_tag(other) == "wall" then
            -- handle wall collision
        end
    end
end)
```
