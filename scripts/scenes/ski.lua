-- scripts/scenes/ski.lua
local TILE  = 16
local MAP_W = 31
local MAP_H = 80   -- tall scrolling slope
local VIEW_W = MAP_W * TILE
local VIEW_H = 18  * TILE   -- viewport stays 18 tiles tall

engine.set_window_size(VIEW_W, VIEW_H)
engine.set_window_title("Tiny Ski")

local SPEED = 120
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

  local left  = engine.is_key_pressed("left")
  local right = engine.is_key_pressed("right")
  local up    = engine.is_key_pressed("up")
  local down  = engine.is_key_pressed("down")

  local vx, vy = 0, 0
  if left  then vx = -SPEED end
  if right then vx =  SPEED end
  if up    then vy = -SPEED end
  if down  then vy =  SPEED end

  if vx ~= 0 and vy ~= 0 then
    local inv = SPEED / math.sqrt(vx*vx + vy*vy)
    vx, vy = vx * inv, vy * inv
  end

  world.set_velocity(player, vx, vy)

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

  local px, py = world.get_position(player)
  engine.set_camera(px - PIN_X, py - PIN_Y)

  local pw, ph = TILE, TILE
  local function solid(wx, wy)
    return world.is_tile_solid(math.floor(wx / TILE), math.floor(wy / TILE))
  end
  local bottom = py + ph
  if solid(px + pw*0.5, bottom) or solid(px+1, bottom) or solid(px+pw-1, bottom) then
    world.set_position(player, px, math.floor(bottom/TILE)*TILE - ph)
    if vy > 0 then world.set_velocity(player, vx, 0) end
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
