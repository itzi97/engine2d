-- scripts/scenes/ski.lua
-- Kenney Tiny Ski — full game scene.
-- Loaded by SceneManager when scene.load("ski") is called.

-- ── Display config (owned here, not in the engine) ───────────────────────────
-- Map is 31x18 tiles at 16px each = 496x288. The window and logical
-- renderer resolution are set here so the engine stays resolution-agnostic.
local TILE   = 16
local MAP_W  = 31
local MAP_H  = 18
local VIEW_W = MAP_W * TILE   -- 496
local VIEW_H = MAP_H * TILE   -- 288

engine.set_window_size(VIEW_W, VIEW_H)
engine.set_window_title("Tiny Ski")

-- ── Game constants ────────────────────────────────────────────────────────────
local GRAVITY  = 200    -- px/s² downward
local STEER    = 90     -- lateral acceleration px/s²
local MAX_SPD  = 280    -- px/s max resultant speed
local CAM_LERP = 0.08   -- camera smoothing (0 = frozen, 1 = snap)

-- ── Boot ─────────────────────────────────────────────────────────────────────
local objects = world.load_tiled_map("assets/maps/sampleMap.tmj")

-- Resolve spawn point from Tiled object layer.
-- Accepts an object whose name or type equals "spawn"; falls back to (40, 40).
local spawn_x, spawn_y = 40, 40
for _, obj in pairs(objects) do
  if obj.type == "spawn" or obj.name == "spawn" then
    spawn_x, spawn_y = obj.x, obj.y
    break
  end
end

-- Load the Kenney Tiny Ski atlas (1 px spacing, no margin, 16x16 tiles).
local atlas = engine.load_texture("assets/ski/tilemap_packed.png")

-- ── Player entity ─────────────────────────────────────────────────────────────
-- Skier-down sprite: column 0, row 0 in the atlas → src rect (0, 0, 16, 16).
local player = world.create_entity()
world.add_transform(player, spawn_x, spawn_y, TILE, TILE)
world.add_sprite(player, 255, 255, 255, 255, 10)
world.set_sprite_texture(player, atlas, 0, 0, TILE, TILE)
world.add_kinematic(player)
world.add_tag(player, "player")

-- ── Camera initialisation ─────────────────────────────────────────────────────
-- Place the player in the upper-centre of the viewport.
engine.set_camera(
  spawn_x - VIEW_W * 0.5,
  spawn_y - VIEW_H * 0.3
)

-- ── Update loop ───────────────────────────────────────────────────────────────
engine.on_update = function(dt)
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end

  local vx, vy = world.get_velocity(player)

  -- Gravity pulls straight down.
  vy = vy + GRAVITY * dt

  -- Left / right steering; bleed lateral speed when no key held.
  if engine.is_key_pressed("left") then
    vx = vx - STEER * dt
  elseif engine.is_key_pressed("right") then
    vx = vx + STEER * dt
  else
    vx = vx * (1.0 - 4.0 * dt)
  end

  -- Clamp to maximum resultant speed.
  local spd = math.sqrt(vx * vx + vy * vy)
  if spd > MAX_SPD then
    local inv = MAX_SPD / spd
    vx = vx * inv
    vy = vy * inv
  end

  world.set_velocity(player, vx, vy)

  -- ── Camera follow (lerp to keep player in upper-centre) ───────────────────
  local px, py = world.get_position(player)
  local cx, cy = engine.get_camera()
  local tx = px - VIEW_W * 0.5
  local ty = py - VIEW_H * 0.3
  engine.set_camera(
    cx + (tx - cx) * CAM_LERP,
    cy + (ty - cy) * CAM_LERP
  )

  -- ── Tile AABB collision ───────────────────────────────────────────────────
  px, py = world.get_position(player)
  local pw, ph = TILE, TILE

  local function solid(wx, wy)
    return world.is_tile_solid(math.floor(wx / TILE), math.floor(wy / TILE))
  end

  -- Bottom edge: push up if landing on a solid tile.
  local bottom = py + ph
  if solid(px + pw * 0.5, bottom) or
     solid(px + 1,        bottom) or
     solid(px + pw - 1,   bottom) then
    local tile_top = math.floor(bottom / TILE) * TILE
    world.set_position(player, px, tile_top - ph)
    px, py = world.get_position(player)
    if vy > 0 then vy = 0 end
    world.set_velocity(player, vx, vy)
  end

  -- Left edge.
  if solid(px, py + ph * 0.5) then
    world.set_position(player, math.floor(px / TILE) * TILE + TILE, py)
    if vx < 0 then world.set_velocity(player, 0, vy) end
  end

  -- Right edge.
  if solid(px + pw, py + ph * 0.5) then
    world.set_position(player, math.floor((px + pw) / TILE) * TILE - pw, py)
    if vx > 0 then world.set_velocity(player, 0, vy) end
  end
end
