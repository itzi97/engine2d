# Lua API Reference

Game scripts interact with the engine through two global tables: `world` and
`engine`. All game logic lives in Lua — the C++ layer has no knowledge of
specific games.

---

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
If it is not set before the first frame, no per-frame logic will run.
See `docs/ARCHITECTURE.md → on_update contract` for the debug procedure if
callbacks are not firing.

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
| `world.add_transform(id, x, y, w, h)` | — | Add a `TransformComponent` |
| `world.set_position(id, x, y)` | — | Set world position |
| `world.get_position(id)` | `x, y` | Get world position |
| `world.set_velocity(id, vx, vy)` | — | Set velocity (units/sec) |
| `world.get_velocity(id)` | `vx, vy` | Get velocity |

### Sprite

| Function | Returns | Description |
|---|---|---|
| `world.add_sprite(id, r, g, b, a [, layer])` | — | Add a solid-colour rect sprite. Optional `layer` (int, default 0) sets render order. |
| `world.add_texture_sprite(id, tex, sx, sy, sw, sh)` | — | Add a textured sprite with source rect |
| `world.set_layer(id, layer)` | — | Set render layer (lower = drawn first). Calls `MarkSortDirty` internally. |

### Text

| Function | Returns | Description |
|---|---|---|
| `world.add_text(id, text, size, r, g, b)` | — | Add a `TextComponent`. Requires a `TransformComponent` for position. |
| `world.set_text(id, text)` | — | Update the displayed string |
| `world.set_text_color(id, r, g, b, a)` | — | Update the text colour (0–255 per channel) |

### Tags

| Function | Returns | Description |
|---|---|---|
| `world.add_tag(id, tag)` | — | Assign a string tag to an entity |
| `world.get_tag(id)` | `string` | Get the entity's tag |

### Collisions

| Function | Returns | Description |
|---|---|---|
| `world.get_collisions_for(id)` | table of `{a, b}` pairs | All collisions involving a specific entity this frame |
| `world.get_collisions_tagged(tag)` | table of `{a, b}` pairs | All collisions involving any entity with the given tag |

Use `get_collisions_for` when you already hold the entity ID (e.g. the ball in
Breakout). Use `get_collisions_tagged` when you want all collisions for a
category (e.g. all `"enemy"` hits) without tracking individual IDs.

### Map loading

| Function | Returns | Description |
|---|---|---|
| `world.load_tiled_map(path)` | table | Parse a Tiled `.tmj` file. Returns a table keyed by object **name**. |

Each entry in the returned table has the following fields:

| Field | Type | Description |
|---|---|---|
| `.type` | string | Tiled object class (e.g. `"brick"`, `"player"`) |
| `.x`, `.y` | number | Top-left position in pixels |
| `.w`, `.h` | number | Width and height in pixels |
| `.properties` | table | Custom properties from Tiled, string → string |

```lua
local objects = world.load_tiled_map("assets/maps/level1.tmj")
for name, obj in pairs(objects) do
    if obj.type == "brick" then
        local r = tonumber(obj.properties["color_r"]) or 200
        local g = tonumber(obj.properties["color_g"]) or 200
        local b = tonumber(obj.properties["color_b"]) or 200
        local e = world.create_entity()
        world.add_transform(e, obj.x, obj.y, obj.w, obj.h)
        world.add_sprite(e, r, g, b, 255)
        world.add_tag(e, "brick")
    end
end
```

---

## `engine` table

### Window

| Function | Description |
|---|---|
| `engine.set_window_title(title)` | Set the OS window title |
| `engine.set_window_size(w, h)` | Resize the window |

### Lifecycle

| Function | Description |
|---|---|
| `engine.on_update(fn)` | Register per-frame callback. `fn` receives `dt` (seconds since last frame). |
| `engine.quit()` | Request engine shutdown at end of current frame. |
| `engine.load_scene(fn)` | Destroy all current entities and call `fn()` as the new scene's init. Safe to call from inside `on_update`. |

### Input

| Function | Returns | Description |
|---|---|---|
| `engine.is_key_pressed(key)` | `bool` | True while the key is held down |
| `engine.is_key_just_pressed(key)` | `bool` | True only on the first frame the key goes down |

**Key names** are uppercase strings matching SDL canonical names:

```
Arrow keys:   "UP"  "DOWN"  "LEFT"  "RIGHT"
Letters:      "A" … "Z"
Digits:       "1" … "9"  "0"
Special:      "SPACE"  "ESCAPE"  "RETURN"  "RETURN2"  "BACKSPACE"
              "LSHIFT"  "RSHIFT"  "LCTRL"  "RCTRL"
```

Any name accepted by SDL's `SDL_GetKeyFromName` is valid.

### Textures

| Function | Returns | Description |
|---|---|---|
| `engine.load_texture(path)` | `tex` handle | Load a texture from disk for use with `add_texture_sprite` |

### Audio

| Function | Description |
|---|---|
| `engine.play_music(path)` | Start looping background music from file |
| `engine.stop_music()` | Stop background music |
| `engine.play_sfx(path [, volume])` | Play a one-shot sound effect. `volume` is 0.0–1.0 (default 1.0) |
| `engine.set_music_volume(v)` | Set music volume (0.0–1.0) |

---

## Full Example

```lua
-- Boot: set window
engine.set_window_title("My Game")
engine.set_window_size(1280, 720)

-- Load assets
local tex = engine.load_texture("assets/player.png")
engine.play_music("assets/bgm.ogg")

-- Spawn player
local player = world.create_entity()
world.add_transform(player, 100, 100, 32, 32)
world.add_texture_sprite(player, tex, 0, 0, 32, 32)
world.add_tag(player, "player")
world.set_layer(player, 1)

-- HUD
local hud = world.create_entity()
world.add_transform(hud, 10, 4, 0, 0)
world.add_text(hud, "Score: 0", 22, 255, 255, 255)

-- Environment (spawned from Tiled map)
local objects = world.load_tiled_map("assets/maps/level1.tmj")
for name, obj in pairs(objects) do
    if obj.type == "wall" then
        local e = world.create_entity()
        world.add_transform(e, obj.x, obj.y, obj.w, obj.h)
        world.add_sprite(e, 80, 80, 255, 255)
        world.add_tag(e, "wall")
    end
end

local score = 0

engine.on_update(function(dt)
    -- Movement
    local speed = 150
    local vx, vy = 0, 0
    if engine.is_key_pressed("RIGHT") then vx =  speed end
    if engine.is_key_pressed("LEFT")  then vx = -speed end
    if engine.is_key_pressed("DOWN")  then vy =  speed end
    if engine.is_key_pressed("UP")    then vy = -speed end
    world.set_velocity(player, vx, vy)

    if engine.is_key_just_pressed("ESCAPE") then
        engine.quit()
    end

    -- Collision response
    for _, col in ipairs(world.get_collisions_for(player)) do
        local other = col.a == player and col.b or col.a
        if world.get_tag(other) == "wall" then
            score = score + 1
            world.set_text(hud, "Score: " .. score)
        end
    end
end)
```
