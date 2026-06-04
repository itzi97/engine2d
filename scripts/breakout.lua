-- breakout.lua
-- Controls: LEFT/RIGHT arrows to move paddle. R to reset. ESCAPE to pause.

engine.set_window_title("Breakout")
engine.set_window_size(1280, 720)

if not pause_menu then
  dofile("scripts/pause_menu.lua")
end

local W, H   = 1280, 720
local BALL_S = 16
local PAD_W, PAD_H     = 120, 14
local BRICK_W, BRICK_H = 74, 24
local BRICK_COLS, BRICK_ROWS = 14, 6
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

-- ─── init ──────────────────────────────────────────────────────────────────

local function init()
  local ball_entity   = make_entity(W/2 - BALL_S/2, H - 160, BALL_S, BALL_S, 255, 255, 255)
  local ball_vx, ball_vy = 260, -320

  local paddle_entity = make_entity(W/2 - PAD_W/2, H - 48, PAD_W, PAD_H, 80, 160, 255)
  world.add_tag(paddle_entity, "paddle")

  local bricks = {}
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

  local status_entity = world.create_entity()
  world.add_transform(status_entity, W/2 - 160, H/2 - 20, 0, 0)
  world.add_text(status_entity, "", 32, 255, 220, 80)

  local score     = 0
  local game_over = false
  local won       = false

  -- ─── update ──────────────────────────────────────────────────────────────

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() pause_menu(init) end)
      return
    end

    if game_over or won then
      if engine.is_key_just_pressed("R") then engine.load_scene(init) end
      return
    end

    local px, py = world.get_position(paddle_entity)
    if engine.is_key_pressed("LEFT")  then px = math.max(0,       px - PAD_SPEED * dt) end
    if engine.is_key_pressed("RIGHT") then px = math.min(W-PAD_W, px + PAD_SPEED * dt) end
    world.set_position(paddle_entity, px, py)

    local bx, by = world.get_position(ball_entity)
    bx = bx + ball_vx * dt
    by = by + ball_vy * dt

    if bx <= 0            then bx = 0;        ball_vx =  math.abs(ball_vx) end
    if bx + BALL_S >= W   then bx = W-BALL_S; ball_vx = -math.abs(ball_vx) end
    if by <= 0            then by = 0;        ball_vy =  math.abs(ball_vy) end
    if by > H then
      game_over = true
      world.set_text(status_entity, "GAME OVER  -  press R")
      world.set_position(ball_entity, bx, by)
      return
    end

    world.set_position(ball_entity, bx, by)

    local hits    = world.get_collisions_for(ball_entity)
    local bounced = false
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
            brick.alive = false
            alive_count = alive_count - 1
            score = score + 10
            world.set_text(score_entity, "Score: " .. score)
            break
          end
        end
      end
    end

    if alive_count == 0 then
      won = true
      world.set_text(status_entity, "YOU WIN!  -  press R")
    end
  end)

  log("breakout: scene loaded")
end

init()
