-- snake.lua  (loaded at runtime via dofile from main_menu)
-- Controls: arrow keys. ESCAPE = pause. R = resume. M = main menu.

engine.set_window_title("Snake")
engine.set_window_size(1280, 704)

local W     = 1280
local CELL  = 32
local COLS  = 40
local ROWS  = 22
local STEP  = 1/10

-- ─── helpers ───────────────────────────────────────────────────────────────

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

-- ─── pause overlay ─────────────────────────────────────────────────────────
-- Returns a controller table with :show(sel) and :hide().
-- `sel` is 1 = Resume highlighted, 2 = Main Menu highlighted.

local function make_pause_overlay()
  local cx = W / 2

  local bg = world.create_entity()
  world.add_transform(bg, cx - 160, 270, 320, 160)
  world.add_sprite(bg, 20, 20, 20, 200, 20)

  local title = world.create_entity()
  world.add_transform(title, cx - 68, 282, 0, 0)
  world.add_text(title, "PAUSED", 32, 255, 220, 80)

  local opt1 = world.create_entity()
  world.add_transform(opt1, cx - 60, 330, 0, 0)
  world.add_text(opt1, "Resume", 26, 180, 180, 180)

  local opt2 = world.create_entity()
  world.add_transform(opt2, cx - 75, 368, 0, 0)
  world.add_text(opt2, "Main Menu", 26, 180, 180, 180)

  local hint = world.create_entity()
  world.add_transform(hint, cx - 128, 406, 0, 0)
  world.add_text(hint, "UP/DOWN  ENTER  or  R / M", 18, 100, 100, 100)

  local SEL_COL   = {255, 220, 80}
  local UNSEL_COL = {180, 180, 180}

  local function refresh(sel)
    if sel == 1 then
      world.set_text_color(opt1, SEL_COL[1],   SEL_COL[2],   SEL_COL[3],   255)
      world.set_text_color(opt2, UNSEL_COL[1], UNSEL_COL[2], UNSEL_COL[3], 255)
    else
      world.set_text_color(opt1, UNSEL_COL[1], UNSEL_COL[2], UNSEL_COL[3], 255)
      world.set_text_color(opt2, SEL_COL[1],   SEL_COL[2],   SEL_COL[3],   255)
    end
  end

  return {
    show = function(sel) refresh(sel) end,
    hide = function()
      -- tint everything invisible (alpha 0 not supported in text; just blank text)
      world.set_text(title, "")
      world.set_text(opt1,  "")
      world.set_text(opt2,  "")
      world.set_text(hint,  "")
      world.add_sprite(bg, 0, 0, 0, 0, 20)
    end,
  }
end

-- ─── game over screen ───────────────────────────────────────────────────

local function game_over(score)
  local cx = W/2
  local e1 = world.create_entity()
  world.add_transform(e1, cx - 130, 260, 0, 0)
  world.add_text(e1, "GAME OVER", 56, 220, 60, 60)

  local e2 = world.create_entity()
  world.add_transform(e2, cx - 100, 340, 0, 0)
  world.add_text(e2, "Length: " .. score, 32, 255, 255, 255)

  local e3 = world.create_entity()
  world.add_transform(e3, cx - 200, 430, 0, 0)
  world.add_text(e3, "R  retry     M  main menu", 26, 180, 180, 180)

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("R") then
      engine.load_scene(function() dofile("scripts/snake.lua") end)
    end
    if engine.is_key_just_pressed("M") or engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
  end)

  log("snake: game over, score=" .. score)
end

-- ─── start screen ──────────────────────────────────────────────────────────

local function start_screen()
  local cx = W/2
  local e1 = world.create_entity()
  world.add_transform(e1, cx - 70, 260, 0, 0)
  world.add_text(e1, "SNAKE", 64, 0, 220, 0)

  local e2 = world.create_entity()
  world.add_transform(e2, cx - 160, 360, 0, 0)
  world.add_text(e2, "ENTER to play   M for menu", 26, 180, 180, 180)

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
      engine.load_scene(init)
    end
    if engine.is_key_just_pressed("M") or engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
  end)

  log("snake: start screen")
end

-- ─── gameplay ────────────────────────────────────────────────────────────────

function init()
  math.randomseed(os.time and os.time() or 12345)

  local head        = make_segment(10 * CELL, 10 * CELL, 0, 255, 0)
  local body        = {}
  local dir         = {x=1, y=0}
  local next_dir    = {x=1, y=0}
  local accumulator = 0
  local score       = 1
  local paused      = false
  local pause_sel   = 1   -- 1 = Resume, 2 = Main Menu

  local food_entity = world.create_entity()
  do
    local fx, fy = spawn_food({head})
    world.add_transform(food_entity, fx, fy, CELL, CELL)
    world.add_sprite(food_entity, 255, 0, 0, 255)
  end

  local score_entity = world.create_entity()
  world.add_transform(score_entity, 10, 4, 0, 0)
  world.add_text(score_entity, "Length: 1", 22, 255, 255, 255)

  local overlay = make_pause_overlay()
  overlay.hide()

  -- ── snake step ───────────────────────────────────────────────────────
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
    if nhx < 0 or nhy < 0 or nhx >= COLS*CELL or nhy >= ROWS*CELL then
      engine.load_scene(function() game_over(score) end); return
    end
    for i = 2, #body do
      local sx, sy = world.get_position(body[i])
      if nhx == sx and nhy == sy then
        engine.load_scene(function() game_over(score) end); return
      end
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

  -- ── update ───────────────────────────────────────────────────────────
  engine.on_update(function(dt)
    -- toggle pause
    if engine.is_key_just_pressed("ESCAPE") then
      paused = not paused
      if paused then
        pause_sel = 1
        overlay.show(pause_sel)
      else
        overlay.hide()
      end
      return
    end

    if paused then
      -- navigate
      if engine.is_key_just_pressed("UP") or engine.is_key_just_pressed("DOWN") then
        pause_sel = (pause_sel == 1) and 2 or 1
        overlay.show(pause_sel)
      end
      -- direct shortcuts
      if engine.is_key_just_pressed("R") then
        paused = false; overlay.hide(); return
      end
      if engine.is_key_just_pressed("M") then
        engine.load_scene(function() dofile("scripts/main_menu.lua") end); return
      end
      -- confirm with ENTER
      if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
        if pause_sel == 1 then
          paused = false; overlay.hide()
        else
          engine.load_scene(function() dofile("scripts/main_menu.lua") end)
        end
      end
      return
    end

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

  log("snake: gameplay started")
end

start_screen()
