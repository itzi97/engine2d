-- scripts/scenes/ski.lua
-- Kenney Tiny Ski — full game scene.

-- ── Display config ────────────────────────────────────────────────────────────────
local TILE  = 16
local MAP_W = 31
local MAP_H = 18
local VIEW_W = MAP_W * TILE
local VIEW_H = MAP_H * TILE

engine.set_window_size(VIEW_W, VIEW_H)
engine.set_window_title("Tiny Ski")

-- ── Game constants ────────────────────────────────────────────────────────
local GRAVITY  = 200
local STEER    = 90
local MAX_SPD  = 280
local CAM_LERP = 0.08

-- ── Boot ──────────────────────────────────────────────────────────────────
local objects = world.load_tiled_map("assets/maps/sampleMap.tmj")

local spawn_x = (MAP_W / 2) * TILE
local spawn_y = 2 * TILE
for _, obj in pairs(objects) do
  if type(obj) == "table" then
    if obj.type == "spawn" or obj.name == "spawn" then
      spawn_x, spawn_y = obj.x, obj.y
      break
    end
  end
end

local atlas = engine.load_texture("assets/ski/tilemap_packed.png")
print("[ski] atlas pointer:", atlas)

local aw, ah = engine.texture_size(atlas)
print("[ski] atlas size:", aw, "x", ah)

-- ── Player entity ─────────────────────────────────────────────────────────
local player = world.create_entity()
world.add_transform(player, spawn_x, spawn_y, TILE, TILE)
world.add_sprite(player, 255, 255, 255, 255, 10)
world.set_sprite_texture(player, atlas, 0, 0, TILE, TILE)
world.add_kinematic(player)
world.add_tag(player, "player")

print("[ski] player entity:", player, "spawn:", spawn_x, spawn_y)

-- ── Camera ───────────────────────────────────────────────────────────────────
engine.set_camera(
  spawn_x - VIEW_W * 0.5 + TILE * 0.5,
  spawn_y - VIEW_H * 0.3
)

-- ── Update loop ───────────────────────────────────────────────────────────
engine.on_update(function(dt)
  if engine.is_key_just_pressed("escape") then engine.quit() end

  local vx, vy = world.get_velocity(player)
  vy = vy + GRAVITY * dt

  if engine.is_key_pressed("left") then
    vx = vx - STEER * dt
  elseif engine.is_key_pressed("right") then
    vx = vx + STEER * dt
  else
    vx = vx * (1.0 - 4.0 * dt)
  end

  local spd = math.sqrt(vx * vx + vy * vy)
  if spd > MAX_SPD then
    local inv = MAX_SPD / spd
    vx, vy = vx * inv, vy * inv
  end

  world.set_velocity(player, vx, vy)

  -- Camera follow
  local px, py = world.get_position(player)
  local cx, cy = engine.get_camera()
  engine.set_camera(
    cx + (px - VIEW_W * 0.5 + TILE * 0.5 - cx) * CAM_LERP,
    cy + (py - VIEW_H * 0.3              - cy) * CAM_LERP
  )

  -- AABB tile collision
  px, py = world.get_position(player)
  local pw, ph = TILE, TILE
  local function solid(wx, wy)
    return world.is_tile_solid(math.floor(wx / TILE), math.floor(wy / TILE))
  end

  local bottom = py + ph
  if solid(px + pw * 0.5, bottom) or solid(px + 1, bottom) or solid(px + pw - 1, bottom) then
    world.set_position(player, px, math.floor(bottom / TILE) * TILE - ph)
    px, py = world.get_position(player)
    if vy > 0 then world.set_velocity(player, vx, 0); vy = 0 end
  end
  if solid(px, py + ph * 0.5) then
    world.set_position(player, math.floor(px / TILE) * TILE + TILE, py)
    if vx < 0 then world.set_velocity(player, 0, vy) end
  end
  if solid(px + pw, py + ph * 0.5) then
    world.set_position(player, math.floor((px + pw) / TILE) * TILE - pw, py)
    if vx > 0 then world.set_velocity(player, 0, vy) end
  end
end)
