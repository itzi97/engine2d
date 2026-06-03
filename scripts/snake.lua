-- snake.lua

local CELL  = 32
local COLS  = 40
local ROWS  = 22
local STEP  = 1/10

-- ── helpers ─────────────────────────────────────────────────────────────────

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
    local hit = false
    for _, seg in ipairs(snake_body) do
      local sx, sy = world.get_position(seg)
      if sx == x and sy == y then hit = true; break end
    end
  until not hit
  return x, y
end

-- ── init (runs on first load and on every engine.load_scene(init)) ──────────

local function init()
  math.randomseed(os.time and os.time() or 12345)

  local head        = make_segment(10 * CELL, 10 * CELL, 0, 255, 0)
  local body        = {}
  local dir         = {x=1, y=0}
  local next_dir    = {x=1, y=0}
  local accumulator = 0
  local score       = 0

  local food_entity = world.create_entity()
  do
    local fx, fy = spawn_food({head})
    world.add_transform(food_entity, fx, fy, CELL, CELL)
    world.add_sprite(food_entity, 255, 0, 0, 255)
    world.add_tag(food_entity, "food")
  end

  local score_entity = world.create_entity()
  world.add_transform(score_entity, 10, 4, 0, 0)
  world.add_text(score_entity, "Length: 1", 22, 255, 255, 255)

  -- ── snake step ────────────────────────────────────────────────────────────

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

    -- wall or self collision → reload scene cleanly
    if nhx < 0 or nhy < 0 or nhx >= COLS*CELL or nhy >= ROWS*CELL then
      engine.load_scene(init); return
    end
    for i = 2, #body do
      local sx, sy = world.get_position(body[i])
      if nhx == sx and nhy == sy then engine.load_scene(init); return end
    end

    local fx, fy = world.get_position(food_entity)
    if nhx == fx and nhy == fy then
      local seg = make_segment(-CELL, -CELL, 0, 200, 0)
      table.insert(body, seg)
      local all = {head}
      for _, s in ipairs(body) do table.insert(all, s) end
      local nfx, nfy = spawn_food(all)
      world.set_position(food_entity, nfx, nfy)
      score = #body + 1
      world.set_text(score_entity, "Length: " .. score)
    end
  end

  -- ── update ────────────────────────────────────────────────────────────────

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

  log("snake: scene loaded")
end

init()
