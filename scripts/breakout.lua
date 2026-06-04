-- breakout.lua  (loaded at runtime via dofile from main_menu)
-- Controls: LEFT/RIGHT to move. ESCAPE = pause. R = resume. M = main menu.

engine.set_window_title("Breakout")
engine.set_window_size(1280, 720)

local W, H           = 1280, 720
local BALL_S         = 16
local PAD_W, PAD_H   = 120, 14
local BRICK_W, BRICK_H         = 74, 24
local BRICK_COLS, BRICK_ROWS   = 14, 6
local BRICK_PADDING  = 8
local BRICK_OFFSET_X = 43
local BRICK_OFFSET_Y = 60
local PAD_SPEED      = 520

-- ─── helpers ───────────────────────────────────────────────────────────────

local function make_entity(x, y, w, h, r, g, b)
  local e = world.create_entity()
  world.add_transform(e, x, y, w, h)
  world.add_sprite(e, r, g, b, 255)
  return e
end

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

-- ─── pause overlay ─────────────────────────────────────────────────────────

local function make_pause_overlay()
  local cx = W / 2

  local bg = world.create_entity()
  world.add_transform(bg, cx - 160, H/2 - 90, 320, 160)
  world.add_sprite(bg, 20, 20, 20, 200, 20)

  local title = world.create_entity()
  world.add_transform(title, cx - 68, H/2 - 78, 0, 0)
  world.add_text(title, "PAUSED", 32, 255, 220, 80)

  local opt1 = world.create_entity()
  world.add_transform(opt1, cx - 60, H/2 - 30, 0, 0)
  world.add_text(opt1, "Resume", 26, 180, 180, 180)

  local opt2 = world.create_entity()
  world.add_transform(opt2, cx - 75, H/2 + 8, 0, 0)
  world.add_text(opt2, "Main Menu", 26, 180, 180, 180)

  local hint = world.create_entity()
  world.add_transform(hint, cx - 128, H/2 + 46, 0, 0)
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
      world.set_text(title, "")
      world.set_text(opt1,  "")
      world.set_text(opt2,  "")
      world.set_text(hint,  "")
      world.add_sprite(bg, 0, 0, 0, 0, 20)
    end,
  }
end

-- ─── game over screen ───────────────────────────────────────────────────

local function game_over(score, won)
  local cx   = W/2
  local msg  = won and "YOU WIN!" or "GAME OVER"
  local col  = won and {80, 220, 80} or {220, 60, 60}

  local e1 = world.create_entity()
  world.add_transform(e1, cx - 140, 260, 0, 0)
  world.add_text(e1, msg, 56, col[1], col[2], col[3])

  local e2 = world.create_entity()
  world.add_transform(e2, cx - 80, 340, 0, 0)
  world.add_text(e2, "Score: " .. score, 32, 255, 255, 255)

  local e3 = world.create_entity()
  world.add_transform(e3, cx - 200, 430, 0, 0)
  world.add_text(e3, "R  retry     M  main menu", 26, 180, 180, 180)

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

-- ─── start screen ──────────────────────────────────────────────────────────

local function start_screen()
  local cx = W/2
  local e1 = world.create_entity()
  world.add_transform(e1, cx - 110, 260, 0, 0)
  world.add_text(e1, "BREAKOUT", 64, 80, 160, 255)

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

  log("breakout: start screen")
end

-- ─── gameplay ────────────────────────────────────────────────────────────────

function init()
  local ball_entity      = make_entity(W/2 - BALL_S/2, H - 160, BALL_S, BALL_S, 255, 255, 255)
  local ball_vx, ball_vy = 260, -320
  local paddle_entity    = make_entity(W/2 - PAD_W/2, H - 48, PAD_W, PAD_H, 80, 160, 255)
  world.add_tag(paddle_entity, "paddle")

  local bricks  = {}
  local colours = {
    {220, 50,  50 }, {220, 130, 50 }, {220, 220, 50 },
    {50,  220, 50 }, {50,  180, 220}, {130, 50,  220},
  }
  for row = 0, BRICK_ROWS - 1 do
    local c = colours[row + 1]
    for col = 0, BRICK_COLS - 1 do
      local e = make_entity(
        BRICK_OFFSET_X + col * (BRICK_W + BRICK_PADDING),
        BRICK_OFFSET_Y + row * (BRICK_H + BRICK_PADDING),
        BRICK_W, BRICK_H, c[1], c[2], c[3]
      )
      world.add_tag(e, "brick")
      table.insert(bricks, {entity = e, alive = true})
    end
  end

  local score_entity = world.create_entity()
  world.add_transform(score_entity, 10, 4, 0, 0)
  world.add_text(score_entity, "Score: 0", 22, 255, 255, 255)

  local score        = 0
  local paused       = false
  local pause_sel    = 1
  local total_bricks = BRICK_COLS * BRICK_ROWS

  local overlay = make_pause_overlay()
  overlay.hide()

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
      if engine.is_key_just_pressed("UP") or engine.is_key_just_pressed("DOWN") then
        pause_sel = (pause_sel == 1) and 2 or 1
        overlay.show(pause_sel)
      end
      if engine.is_key_just_pressed("R") then
        paused = false; overlay.hide(); return
      end
      if engine.is_key_just_pressed("M") then
        engine.load_scene(function() dofile("scripts/main_menu.lua") end); return
      end
      if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
        if pause_sel == 1 then
          paused = false; overlay.hide()
        else
          engine.load_scene(function() dofile("scripts/main_menu.lua") end)
        end
      end
      return
    end

    -- paddle
    local px, py = world.get_position(paddle_entity)
    if engine.is_key_pressed("LEFT")  then px = math.max(0,       px - PAD_SPEED * dt) end
    if engine.is_key_pressed("RIGHT") then px = math.min(W-PAD_W, px + PAD_SPEED * dt) end
    world.set_position(paddle_entity, px, py)

    -- ball movement
    local bx, by = world.get_position(ball_entity)
    bx = bx + ball_vx * dt
    by = by + ball_vy * dt

    if bx <= 0            then bx = 0;        ball_vx =  math.abs(ball_vx) end
    if bx + BALL_S >= W   then bx = W-BALL_S; ball_vx = -math.abs(ball_vx) end
    if by <= 0            then by = 0;        ball_vy =  math.abs(ball_vy) end

    if by > H then
      world.set_position(ball_entity, bx, by)
      engine.load_scene(function() game_over(score, false) end)
      return
    end

    world.set_position(ball_entity, bx, by)

    local hits    = world.get_collisions_for(ball_entity)
    local bounced = false
    local alive_count = total_bricks
    for _, brick in ipairs(bricks) do
      if not brick.alive then alive_count = alive_count - 1 end
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

  log("breakout: gameplay started")
end

start_screen()
