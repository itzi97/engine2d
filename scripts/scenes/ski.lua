-- scripts/scenes/ski.lua
local TILE  = 16      -- native tile size, matches the atlas
local MAP_W = 31
local MAP_H = 200
local VIEW_W = MAP_W * TILE   -- 496
local VIEW_H = 18  * TILE   -- 288

engine.set_window_size(VIEW_W, VIEW_H)
engine.set_window_title("Tiny Ski")

-- ── Speed gears ────────────────────────────────────────────────────────────
local GEARS      = { "slow", "normal", "fast" }
local GEAR_SPEED = { slow = 60,  normal = 120, fast = 200 }
local GEAR_STEER = { slow = 55,  normal = 100, fast = 150 }
local gear_idx   = 2

local PIN_X = VIEW_W * 0.5 - TILE * 0.5
local PIN_Y = VIEW_H * 0.3

-- Atlas frames (source coords, 16px grid)
local FRAME_IDLE  = { x = 10*TILE, y = 5*TILE, w = TILE, h = TILE }
local FRAME_STEER = { x = 11*TILE, y = 5*TILE, w = TILE, h = TILE }
local TILT = 15

-- Trail sprites
local TRAIL_SOLID   = { x = 10*TILE, y = 4*TILE, w = TILE, h = TILE }
local TRAIL_FADE    = { x = 11*TILE, y = 4*TILE, w = TILE, h = TILE }
local TRAIL_SPACING = TILE
local TRAIL_TTL     = 2.0

-- Load map — tiles spawn at native 16px, no scaling needed
local objects = world.load_tiled_map("assets/maps/longMap.tmj")

local spawn_x = (MAP_W / 2) * TILE
local spawn_y = 2 * TILE
for _, obj in pairs(objects) do
  if type(obj) == "table" then
    if obj.type == "spawn" or obj.name == "spawn" then
      spawn_x = obj.x
      spawn_y = obj.y
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

local last_frame    = FRAME_IDLE
local last_rotation = 0

local function set_frame(f)
  if f ~= last_frame then
    world.set_sprite_src(player, f.x, f.y, f.w, f.h)
    last_frame = f
  end
end

local function set_rotation(deg)
  if deg ~= last_rotation then
    world.set_rotation(player, deg)
    last_rotation = deg
  end
end

local trail         = {}
local trail_y_accum = 0

local function spawn_trail_segment(x, y)
  if #trail > 0 then
    local prev = trail[#trail]
    world.set_sprite_src(prev.entity,
      TRAIL_SOLID.x, TRAIL_SOLID.y, TRAIL_SOLID.w, TRAIL_SOLID.h)
  end
  local e = world.create_entity()
  world.add_transform(e, x, y, TILE, TILE)
  world.add_sprite(e, 255, 255, 255, 255, 5)
  world.set_sprite_texture(e, atlas,
    TRAIL_FADE.x, TRAIL_FADE.y, TRAIL_FADE.w, TRAIL_FADE.h)
  table.insert(trail, { entity = e, ttl = TRAIL_TTL })
end

local function update_trail(dt)
  local i = 1
  while i <= #trail do
    local seg = trail[i]
    seg.ttl = seg.ttl - dt
    if seg.ttl <= 0 then
      world.destroy_entity(seg.entity)
      table.remove(trail, i)
    else
      i = i + 1
    end
  end
end

engine.on_update(function(dt)
  if engine.is_key_just_pressed("escape") then engine.quit() end

  local pushing = engine.is_key_pressed("down")
  if engine.is_key_just_pressed("up")   then gear_idx = math.max(1,       gear_idx - 1) end
  if engine.is_key_just_pressed("down") then gear_idx = math.min(#GEARS,  gear_idx + 1) end

  local gear_name = GEARS[gear_idx]
  local vy = GEAR_SPEED[gear_name]

  local left  = engine.is_key_pressed("left")
  local right = engine.is_key_pressed("right")
  local steer = GEAR_STEER[gear_name]
  local vx = 0
  if left  then vx = -steer end
  if right then vx =  steer end

  world.set_velocity(player, vx, vy)

  if pushing or left or right then set_frame(FRAME_STEER)
  else                             set_frame(FRAME_IDLE) end

  if left then
    world.set_sprite_flip(player, true,  false); set_rotation( TILT)
  elseif right then
    world.set_sprite_flip(player, false, false); set_rotation(-TILT)
  else
    set_rotation(0)
  end

  local px, py = world.get_position(player)
  engine.set_camera(px - PIN_X, py - PIN_Y)

  trail_y_accum = trail_y_accum + vy * dt
  if trail_y_accum >= TRAIL_SPACING then
    trail_y_accum = trail_y_accum - TRAIL_SPACING
    spawn_trail_segment(px, py)
  end

  update_trail(dt)

  engine.set_window_title(string.format(
    "Tiny Ski  |  [%s]  world=(%.0f,%.0f)", gear_name, px, py))

  -- Border collision (tile coords = world px / TILE)
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
