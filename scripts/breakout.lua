-- breakout.lua  (loaded at runtime via dofile from main_menu)
-- Controls: LEFT/RIGHT to move. ESCAPE = pause. R = resume. M = main menu.
--
-- Bricks are driven entirely by the tilemap object layer.
-- Each brick object must have class="brick" and custom properties:
--   color_r, color_g, color_b  (int, 0-255)

dofile("scripts/util.lua")

engine.set_window_title("Breakout")
engine.set_window_size(1280, 720)

local W, H           = 1280, 720
local BALL_S         = 16
local PAD_W, PAD_H   = 120, 14
local BRICK_W, BRICK_H = 72, 20
local PAD_SPEED      = 520

local LEVELS = {
  { name = "Dungeon", map = "assets/maps/level1.tmj", label_col = {100, 120, 180} },
  { name = "Neon",    map = "assets/maps/level2.tmj", label_col = {60,  200, 180} },
}

-- ─── AABB side detection ─────────────────────────────────────────────────────

local function hit_side(ax, ay, aw, ah, bx, by, bw, bh)
  local ol  = (ax + aw) - bx
  local or_ = (bx + bw) - ax
  local ot  = (ay + ah) - by
  local ob  = (by + bh) - ay
  if math.min(ol, or_) < math.min(ot, ob) then
    return ol < or_ and "left" or "right"
  else
    return ot < ob and "top" or "bottom"
  end
end

-- ─── game over screen ────────────────────────────────────────────────────────

local function game_over(score, won)
  local cx  = W / 2
  local msg = won and "YOU WIN!" or "GAME OVER"
  local col = won and {80, 220, 80} or {220, 60, 60}

  make_label(cx - 140, 260, msg,                          56, col[1], col[2], col[3])
  make_label(cx -  80, 340, "Score: " .. score,          32, 255, 255, 255)
  make_label(cx - 200, 430, "R  retry     M  main menu", 26, 180, 180, 180)

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("R") then
      engine.load_scene(function() dofile("scripts/breakout.lua") end)
    end
    if engine.is_key_just_pressed("M") or engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
  end)

  log("breakout: game over, score=" .. score .. ", won=" .. tostring(won))
end

-- ─── level select ────────────────────────────────────────────────────────────

local chosen_level = 1

local function level_select()
  local cx = W / 2

  make_label(cx - 110, 200, "BREAKOUT",       56,  80, 160, 255)
  make_label(cx - 100, 278, "Choose a level", 26, 180, 180, 180)
  make_label(cx - 220, 420,
    "LEFT/RIGHT or 1/2 to pick   ENTER to confirm   M = menu",
    20, 120, 120, 120)

  local sel_labels = {}
  for i, lv in ipairs(LEVELS) do
    local lx = cx - 200 + (i - 1) * 230
    local e  = make_label(lx, 340, "[" .. i .. "]  " .. lv.name, 32,
                          lv.label_col[1], lv.label_col[2], lv.label_col[3])
    sel_labels[i] = e
  end

  local cursor_e = make_label(0, 390, "v", 20, 255, 220, 80)

  local sel = 1
  local function refresh()
    for i, e in ipairs(sel_labels) do
      local lv = LEVELS[i]
      if i == sel then
        world.set_text_color(e, 255, 255, 100, 255)
      else
        world.set_text_color(e, lv.label_col[1], lv.label_col[2], lv.label_col[3], 255)
      end
    end
    world.set_position(cursor_e, cx - 200 + (sel - 1) * 230 + 30, 390)
  end
  refresh()

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("LEFT") then
      sel = (sel == 1) and #LEVELS or sel - 1; refresh()
    end
    if engine.is_key_just_pressed("RIGHT") then
      sel = (sel == #LEVELS) and 1 or sel + 1; refresh()
    end
    if engine.is_key_just_pressed("1") then sel = 1; refresh() end
    if engine.is_key_just_pressed("2") then sel = 2; refresh() end
    if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
      chosen_level = sel
      engine.load_scene(start_screen)
    end
    if engine.is_key_just_pressed("M") or engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
  end)

  log("breakout: level select")
end

-- ─── start screen ────────────────────────────────────────────────────────────

function start_screen()
  local cx = W / 2
  local lv = LEVELS[chosen_level]

  make_label(cx - 110, 260, "BREAKOUT",       64,  80, 160, 255)
  make_label(cx -  80, 345, "Level: " .. lv.name, 28,
             lv.label_col[1], lv.label_col[2], lv.label_col[3])
  make_label(cx - 210, 400,
             "ENTER to play   BACKSPACE = pick level   M = menu",
             22, 180, 180, 180)

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
      engine.load_scene(init)
    end
    if engine.is_key_just_pressed("BACKSPACE") then
      engine.load_scene(level_select)
    end
    if engine.is_key_just_pressed("M") or engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
  end)

  log("breakout: start screen, level=" .. lv.name)
end

-- ─── gameplay ────────────────────────────────────────────────────────────────

function init()
  local lv = LEVELS[chosen_level]

  local map_objects = world.load_tiled_map(lv.map)
  log("breakout: loaded map " .. lv.map)

  local bricks = {}
  for _, obj in pairs(map_objects) do
    if obj.type == "brick" then
      local r = tonumber(obj.properties["color_r"]) or 200
      local g = tonumber(obj.properties["color_g"]) or 200
      local b = tonumber(obj.properties["color_b"]) or 200
      local e = make_sprite(obj.x, obj.y, obj.w, obj.h, r, g, b)
      world.add_tag(e, "brick")
      table.insert(bricks, { entity = e, alive = true })
    end
  end

  local total_bricks = #bricks
  log("breakout: spawned " .. total_bricks .. " bricks from map")

  local ball_entity      = make_sprite(W/2 - BALL_S/2, H - 160, BALL_S, BALL_S, 255, 255, 255)
  local ball_vx, ball_vy = 260, -320
  local paddle_entity    = make_sprite(W/2 - PAD_W/2, H - 48, PAD_W, PAD_H, 80, 160, 255)
  world.add_tag(paddle_entity, "paddle")

  local score_entity = make_label(10, 4, "Score: 0", 22, 255, 255, 255)
  local score        = 0
  local paused       = false
  local pause_sel    = 1

  local overlay = make_pause_overlay(W / 2, H / 2 - 90)
  overlay.hide()

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("ESCAPE") then
      paused = not paused
      if paused then pause_sel = 1; overlay.show(pause_sel)
      else overlay.hide() end
      return
    end

    if paused then
      if engine.is_key_just_pressed("UP") or engine.is_key_just_pressed("DOWN") then
        pause_sel = (pause_sel == 1) and 2 or 1
        overlay.show(pause_sel)
      end
      if engine.is_key_just_pressed("R") then paused = false; overlay.hide(); return end
      if engine.is_key_just_pressed("M") then
        engine.load_scene(function() dofile("scripts/main_menu.lua") end); return
      end
      if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
        if pause_sel == 1 then paused = false; overlay.hide()
        else engine.load_scene(function() dofile("scripts/main_menu.lua") end) end
      end
      return
    end

    local px, py = world.get_position(paddle_entity)
    if engine.is_key_pressed("LEFT")  then px = math.max(0,       px - PAD_SPEED * dt) end
    if engine.is_key_pressed("RIGHT") then px = math.min(W-PAD_W, px + PAD_SPEED * dt) end
    world.set_position(paddle_entity, px, py)

    local bx, by = world.get_position(ball_entity)
    bx = bx + ball_vx * dt
    by = by + ball_vy * dt

    if bx <= 0          then bx = 0;        ball_vx =  math.abs(ball_vx) end
    if bx + BALL_S >= W then bx = W-BALL_S; ball_vx = -math.abs(ball_vx) end
    if by <= 0          then by = 0;        ball_vy =  math.abs(ball_vy) end

    if by > H then
      world.set_position(ball_entity, bx, by)
      engine.load_scene(function() game_over(score, false) end)
      return
    end

    world.set_position(ball_entity, bx, by)

    local hits      = world.get_collisions_for(ball_entity)
    local bounced   = false
    local alive_count = 0
    for _, brick in ipairs(bricks) do
      if brick.alive then alive_count = alive_count + 1 end
    end

    for _, col in ipairs(hits) do
      local other = col.a == ball_entity and col.b or col.a
      local tag   = world.get_tag(other)

      if tag == "paddle" then
        local hit_pos = (bx + BALL_S/2) - (px + PAD_W/2)
        ball_vx = hit_pos * 5.5
        ball_vy = -math.abs(ball_vy)
        by = py - BALL_S
        world.set_position(ball_entity, bx, by)

      elseif tag == "brick" then
        for _, brick in ipairs(bricks) do
          if brick.alive and brick.entity == other then
            if not bounced then
              local rx, ry = world.get_position(other)
              local side   = hit_side(bx, by, BALL_S, BALL_S, rx, ry, BRICK_W, BRICK_H)
              if side == "left" or side == "right" then ball_vx = -ball_vx
              else ball_vy = -ball_vy end
              bounced = true
            end
            world.destroy_entity(brick.entity)
            brick.alive  = false
            alive_count  = alive_count - 1
            score        = score + 10
            world.set_text(score_entity, "Score: " .. score)
            break
          end
        end
      end
    end

    if alive_count == 0 then
      engine.load_scene(function() game_over(score, true) end)
    end
  end)

  log("breakout: gameplay started, level=" .. lv.name)
end

level_select()
