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
| `world.set_rotation(id, deg)` | — | Set rotation in degrees |
| `world.get_rotation(id)` | `deg` | Get rotation in degrees |

### Physics (KinematicComponent)

Adding a `KinematicComponent` opts the entity into `PhysicsSystem`, which
integrates velocity and acceleration into position each frame.

| Function | Returns | Description |
|---|---|---|
| `world.add_kinematic(id)` | — | Add a `KinematicComponent` (required for velocity integration) |
| `world.set_velocity(id, vx, vy)` | — | Set velocity (units/sec) |
| `world.get_velocity(id)` | `vx, vy` | Get velocity |
| `world.set_acceleration(id, ax, ay)` | — | Set constant acceleration (units/sec²) |

### Sprite

| Function | Returns | Description |
|---|---|---|
| `world.add_sprite(id, r, g, b, a [, layer])` | — | Add a solid-colour rect sprite. Optional `layer` (int, default 0) sets render order. |
| `world.set_layer(id, layer)` | — | Set render layer (lower = drawn first). Calls `MarkSortDirty` internally. |
| `world.set_sprite_texture(id, tex, sx, sy, sw, sh)` | — | Set texture handle + source rect in one call |
| `world.set_sprite_src(id, sx, sy, sw, sh)` | — | Update source rect on an already-textured sprite |
| `world.set_sprite_flip(id, flipX, flipY)` | — | Flip sprite horizontally and/or vertically |
| `world.set_sprite_tint(id, r, g, b [, a])` | — | Tint colour multiplied over the sprite (0–255 per channel). `a` defaults to 255. |

### Animation

An `AnimationComponent` requires a `SpriteComponent` to be present. It drives
the sprite's `srcRect` each frame.

| Function | Returns | Description |
|---|---|---|
| `world.add_animation(id, frames, dur [, loop])` | — | Add an animation. `frames` is an array of `{x,y,w,h}` tables, `dur` is seconds per frame, `loop` defaults to `true`. |
| `world.set_animation_playing(id, bool)` | — | Pause (`false`) or resume (`true`) the animation. |
| `world.reset_animation(id)` | — | Restart animation from frame 0. |

```lua
world.add_animation(ship, {
  { x=0,  y=0, w=32, h=32 },
  { x=32, y=0, w=32, h=32 },
}, 0.1)   -- 0.1s per frame, looping
```

### Text

| Function | Returns | Description |
|---|---|---|
| `world.add_text(id, text, size, r, g, b)` | — | Add a `TextComponent`. Requires a `TransformComponent` for position. |
| `world.set_text(id, text)` | — | Update the displayed string |
| `world.set_text_color(id, r, g, b, a)` | — | Update the text colour (0–255 per channel) |
| `world.measure_text(id)` | `w, h` | Return the pixel dimensions of the rendered text texture. Returns `0, 0` if the entity has no `TextComponent` or the texture hasn't been built yet (text is empty or first `Render` hasn't run). |
| `world.set_text_anchor(id, ax, ay)` | — | Set the anchor point in normalised `[0, 1]` coordinates. The anchor defines which point on the text bounding box is placed at the entity's transform position. |

**Anchor reference:**

| `ax, ay` | Meaning |
|---|---|
| `0, 0` | Top-left *(default, legacy behaviour)* |
| `0.5, 0.5` | Centre |
| `1, 0` | Top-right |
| `0, 1` | Bottom-left |
| `1, 1` | Bottom-right |

**Centring a label on screen** — preferred pattern:
```lua
-- One-time setup: anchor at centre, position at screen centre.
-- Works for any string length with no manual width calculation.
world.set_text_anchor(label, 0.5, 0.5)
world.set_position(label, W/2, H/2)

-- Updating text later re-uses the same anchor — no repositioning needed.
world.set_text(label, "GAME OVER")
```

**Using `measure_text` for manual layout:**
```lua
-- Right-align a score label 8px from the right edge.
local tw, _ = world.measure_text(score_label)
world.set_position(score_label, W - tw - 8, 8)
```

> **Note:** `measure_text` returns the size of the *cached* texture. If you
> call it immediately after `set_text` but before the next `Render` pass, it
> will still reflect the *previous* string's dimensions. For most use cases
> `set_text_anchor` is simpler and avoids this timing issue entirely.

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
| `world.load_tiled_map(path)` | table | Parse a Tiled `.tmj` file. Tile layers are spawned automatically as sprite entities. Returns a table keyed by object **name**. |

Each entry in the returned table has the following fields:

| Field | Type | Description |
|---|---|---|
| `.entity` | id | The ECS entity spawned for this object |
| `.type` | string | Tiled object class (e.g. `"brick"`, `"player"`) |
| `.x`, `.y` | number | Top-left position in pixels |
| `.w`, `.h` | number | Width and height in pixels |
| `.name` | string | Tiled object name |
| `.properties` | table | Custom properties from Tiled, string → string |

```lua
local objects = world.load_tiled_map("assets/maps/level1.tmj")
for name, obj in pairs(objects) do
    if obj.type == "brick" then
        local r = tonumber(obj.properties["color_r"]) or 200
        local g = tonumber(obj.properties["color_g"]) or 200
        local b = tonumber(obj.properties["color_b"]) or 200
        -- obj.entity already has TransformComponent + TagComponent from MapSystem
        world.add_sprite(obj.entity, r, g, b, 255)
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
| `engine.load_texture(path)` | `tex` handle | Load a texture from disk. Pass the handle to `world.set_sprite_texture`. |

### Audio

Audio is handle-based. Load assets once at scene init, then pass handles to
playback functions.

| Function | Returns | Description |
|---|---|---|
| `engine.load_sfx(path)` | `int` handle | Decode and cache a sound effect (OGG/WAV). Returns `-1` on failure. |
| `engine.play_sfx(handle [, volume])` | — | Play a one-shot SFX. `volume` is 0.0–1.0 (default 1.0). |
| `engine.load_music(path)` | `int` handle | Load a music track for streaming. Returns `-1` on failure. |
| `engine.play_music(handle [, loop])` | — | Start music playback. `loop` defaults to `true`. |
| `engine.pause_music()` | — | Pause currently playing music. |
| `engine.resume_music()` | — | Resume paused music. |
| `engine.stop_music()` | — | Stop music and rewind. |
| `engine.set_music_volume(v)` | — | Set music volume (0.0–1.0). |

```lua
-- Typical audio setup in scene init:
local sfx_laser = engine.load_sfx("assets/audio/sfx/laser.ogg")
local music     = engine.load_music("assets/audio/music/theme.ogg")
engine.set_music_volume(0.6)
engine.play_music(music)

engine.on_update(function(dt)
    if engine.is_key_just_pressed("SPACE") then
        engine.play_sfx(sfx_laser)
    end
end)
```

---

## Full Example

```lua
dofile("scripts/util.lua")   -- make_label, make_sprite, make_pause_overlay

engine.set_window_title("My Game")
engine.set_window_size(1280, 720)

-- Load assets
local tex       = engine.load_texture("assets/player.png")
local sfx_jump  = engine.load_sfx("assets/audio/sfx/jump.ogg")
local music     = engine.load_music("assets/audio/music/theme.ogg")
engine.set_music_volume(0.6)
engine.play_music(music)

-- Spawn player
local player = world.create_entity()
world.add_transform(player, 100, 100, 32, 32)
world.set_sprite_texture(player, tex, 0, 0, 32, 32)
world.add_tag(player, "player")
world.set_layer(player, 1)

-- HUD label, pixel-perfect centred horizontally
local title = make_label(0, 20, "MY GAME", 36, 255, 255, 255)
world.set_text_anchor(title, 0.5, 0)   -- centre horizontally, top-aligned
world.set_position(title, 640, 20)     -- x = screen centre

-- Environment from Tiled map
local objects = world.load_tiled_map("assets/maps/level1.tmj")
for name, obj in pairs(objects) do
    if obj.type == "wall" then
        world.add_sprite(obj.entity, 80, 80, 255, 255)
    end
end

local score = 0

engine.on_update(function(dt)
    local speed = 150
    local vx, vy = 0, 0
    if engine.is_key_pressed("RIGHT") then vx =  speed end
    if engine.is_key_pressed("LEFT")  then vx = -speed end
    if engine.is_key_pressed("DOWN")  then vy =  speed end
    if engine.is_key_pressed("UP")    then vy = -speed end
    world.set_velocity(player, vx, vy)

    if engine.is_key_just_pressed("SPACE") then
        engine.play_sfx(sfx_jump)
    end

    if engine.is_key_just_pressed("ESCAPE") then
        engine.quit()
    end

    for _, col in ipairs(world.get_collisions_for(player)) do
        local other = col.a == player and col.b or col.a
        if world.get_tag(other) == "wall" then
            score = score + 1
            world.set_text(hud, "Score: " .. score)
        end
    end
end)
```
