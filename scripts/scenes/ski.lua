-- scripts/scenes/ski.lua
local TILE  = 16
local MAP_W = 31
local MAP_H = 80   -- tall scrolling slope
local VIEW_W = MAP_W * TILE
local VIEW_H = 18  * TILE   -- viewport stays 18 tiles tall

engine.set_window_size(VIEW_W, VIEW_H)
engine.set_window_title("Tiny Ski")

-- ── Speed gears ────────────────────────────────────────────────────────────
-- ↑ (just pressed) → shift down one gear
-- ↓ (just pressed) → shift up one gear
-- Forward (Y) speed is always positive — skier never stops.
-- Horizontal speed scales with the current forward speed so steering
-- feels consistent at every gear.
local GEARS      = { "slow", "normal", "fast" }
local GEAR_SPEED = { slow = 60, normal = 120, fast = 200 }
local GEAR_STEER = { slow = 55, normal = 100, fast = 150 }
local gear_idx   = 2   -- start at "normal"

local PIN_X = VIEW_W * 0.5 - TILE * 0.5
local PIN_Y = VIEW_H * 0.3

local FRAME_IDLE  = { x = 10*TILE, y = 5*TILE, w = TILE, h = TILE }
local FRAME_STEER = { x = 11*TILE, y = 5*TILE, w = TILE, h = TILE }

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

local player = world.create_entity()
world.add_transform(player, spawn_x, spawn_y, TILE, TILE)
world.add_sprite(player, 255, 255, 255, 255, 10)
world.set_sprite_texture(player, atlas, FRAME_IDLE.x, FRAME_IDLE.y, FRAME_IDLE.w, FRAME_IDLE.h)
world.add_kinematic(player)
world.add_tag(player, "player")

engine.set_camera(spawn_x - PIN_X, spawn_y - PIN_Y)

local steering = false

engine.on_update(function(dt)
  if engine.is_key_just_pressed("escape") then engine.quit() end

  -- ── Gear shifting (one tap = one step) ─────────────────────────────────
  if engine.is_key_just_pressed("up") then
    gear_idx = math.max(1, gear_idx - 1)   -- shift down (slow)
  end
  if engine.is_key_just_pressed("down") then
    gear_idx = math.min(#GEARS, gear_idx + 1) -- shift up (fast)
  end

  local gear_name = GEARS[gear_idx]
  local vy = GEAR_SPEED[gear_name]   -- always positive: always skiing forward

  -- ── Horizontal steering ─────────────────────────────────────────────────
  local left  = engine.is_key_pressed("left")
  local right = engine.is_key_pressed("right")
  local steer = GEAR_STEER[gear_name]
  local vx = 0
  if left  then vx = -steer end
  if right then vx =  steer end

  world.set_velocity(player, vx, vy)

  -- ── Sprite ──────────────────────────────────────────────────────────────
  local is_steering = left or right
  if is_steering ~= steering then
    steering = is_steering
    local f = is_steering and FRAME_STEER or FRAME_IDLE
    world.set_sprite_src(player, f.x, f.y, f.w, f.h)
  end
  if vx < 0 then
    world.set_sprite_flip(player, true, false)
  elseif vx > 0 then
    world.set_sprite_flip(player, false, false)
  end

  -- ── Camera follow ───────────────────────────────────────────────────────
  local px, py = world.get_position(player)
  local cx, cy = px - PIN_X, py - PIN_Y
  engine.set_camera(cx, cy)

  -- Title: gear indicator + position (remove when done debugging)
  engine.set_window_title(string.format(
    "Tiny Ski  |  [%s]  world=(%.0f,%.0f)",
    gear_name, px, py))

  -- ── Tile collision (solid layers only) ──────────────────────────────────
  local pw, ph = TILE, TILE
  local function solid(wx, wy)
    return world.is_tile_solid(math.floor(wx / TILE), math.floor(wy / TILE))
  end
  local bottom = py + ph
  if solid(px + pw*0.5, bottom) or solid(px+1, bottom) or solid(px+pw-1, bottom) then
    world.set_position(player, px, math.floor(bottom/TILE)*TILE - ph)
    world.set_velocity(player, vx, 0)
  end
  if solid(px, py + ph*0.5) then
    world.set_position(player, math.floor(px/TILE)*TILE + TILE, py)
    if vx < 0 then world.set_velocity(player, 0, vy) end
  end
  if solid(px+pw, py + ph*0.5) then
    world.set_position(player, math.floor((px+pw)/TILE)*TILE - pw, py)
    if vx > 0 then world.set_velocity(player, 0, vy) end
  end
end)
