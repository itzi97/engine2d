-- snake.lua

local CELL  = 32
local COLS  = 40   -- 1280 / 32
local ROWS  = 22   -- 720  / 32
local STEP  = 1/10 -- snake ticks per second

-- ── helpers ──────────────────────────────────────────────────────────────────

local function make_segment(x, y, r, g, b)
  local e = world.create_entity()
  world.add_transform(e, x, y, CELL, CELL)
  world.add_sprite(e, r, g, b, 255)
  return e
end

local function spawn_food(snake_body)
  local x, y
  repeat
    x = math.random(0, COLS - 1) * CELL
    y = math.random(0, ROWS - 1) * CELL
    local collision = false
    for _, seg in ipairs(snake_body) do
      local sx, sy = world.get_position(seg)
      if sx == x and sy == y then collision = true; break end
    end
  until not collision
  return x, y
end

-- ── state ────────────────────────────────────────────────────────────────────

math.randomseed(os.time and os.time() or 12345)

local head        = make_segment(10 * CELL, 10 * CELL, 0, 255, 0)
local body        = {}           -- ordered list of segment EntityIds (excl. head)
local dir         = {x=1, y=0}   -- current direction
local next_dir    = {x=1, y=0}   -- buffered next direction
local accumulator = 0

local food_entity = world.create_entity()
do
  local fx, fy = spawn_food({head})
  world.add_transform(food_entity, fx, fy, CELL, CELL)
  world.add_sprite(food_entity, 255, 0, 0, 255)
  world.add_tag(food_entity, "food")
end

log("snake.lua loaded")

-- ── reset ────────────────────────────────────────────────────────────────────

local function reset()
  for _, seg in ipairs(body) do
    world.destroy_entity(seg)
  end
  body        = {}
  dir         = {x=1, y=0}
  next_dir    = {x=1, y=0}
  accumulator = 0   -- prevent a leftover tick firing immediately after reset
  world.set_position(head, 10 * CELL, 10 * CELL)

  local fx, fy = spawn_food({head})
  world.set_position(food_entity, fx, fy)
  log("reset")
end

-- ── snake step ───────────────────────────────────────────────────────────────

local function snake_step()
  dir = next_dir

  local hx, hy = world.get_position(head)
  local prev   = {x = hx, y = hy}

  world.set_position(head, hx + dir.x * CELL, hy + dir.y * CELL)

  for _, seg in ipairs(body) do
    local sx, sy = world.get_position(seg)
    world.set_position(seg, prev.x, prev.y)
    prev = {x = sx, y = sy}
  end

  local nhx, nhy = world.get_position(head)

  -- bounds check
  if nhx < 0 or nhy < 0 or nhx >= COLS * CELL or nhy >= ROWS * CELL then
    reset(); return
  end

  -- self collision (skip segment 1 = direct follower)
  for i = 2, #body do
    local sx, sy = world.get_position(body[i])
    if nhx == sx and nhy == sy then reset(); return end
  end

  -- food collision
  local fx, fy = world.get_position(food_entity)
  if nhx == fx and nhy == fy then
    local seg = make_segment(-CELL, -CELL, 0, 200, 0)
    table.insert(body, seg)

    local all = {head}
    for _, s in ipairs(body) do table.insert(all, s) end
    local nfx, nfy = spawn_food(all)
    world.set_position(food_entity, nfx, nfy)
  end
end

-- ── update ───────────────────────────────────────────────────────────────────

engine.on_update(function(dt)
  if engine.is_key_pressed("UP")    and dir.y ~=  1 then next_dir = {x=0,  y=-1} end
  if engine.is_key_pressed("DOWN")  and dir.y ~= -1 then next_dir = {x=0,  y= 1} end
  if engine.is_key_pressed("LEFT")  and dir.x ~=  1 then next_dir = {x=-1, y= 0} end
  if engine.is_key_pressed("RIGHT") and dir.x ~= -1 then next_dir = {x= 1, y= 0} end

  accumulator = accumulator + dt
  while accumulator >= STEP do
    snake_step()
    accumulator = accumulator - STEP
  end
end)
