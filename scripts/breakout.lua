-- breakout.lua
-- Controls: LEFT / RIGHT arrows to move paddle. R to reset after game over.

local W, H   = 1280, 720
local BALL_S = 16          -- ball side
local PAD_W, PAD_H = 120, 14
local BRICK_W, BRICK_H = 74, 24
local BRICK_COLS, BRICK_ROWS = 14, 6
local BRICK_PADDING = 8
local BRICK_OFFSET_X = 43  -- left margin so grid is centred
local BRICK_OFFSET_Y = 60  -- top margin

-- ── AABB helpers ─────────────────────────────────────────────────────────────

local function overlaps(ax, ay, aw, ah, bx, by, bw, bh)
  return ax < bx + bw and ax + aw > bx
     and ay < by + bh and ay + ah > by
end

-- Returns which side of rect B the rect A hit: "left","right","top","bottom"
-- Assumes overlap already confirmed. Uses overlap depth on each axis.
local function hit_side(ax, ay, aw, ah, bx, by, bw, bh)
  local ol = (ax + aw) - bx   -- overlap from left
  local or_ = (bx + bw) - ax  -- overlap from right
  local ot = (ay + ah) - by   -- overlap from top
  local ob = (by + bh) - ay   -- overlap from bottom
  local min_h = math.min(ol, or_)
  local min_v = math.min(ot, ob)
  if min_h < min_v then
    return ol < or_ and "left" or "right"
  else
    return ot < ob and "top" or "bottom"
  end
end

-- ── state ────────────────────────────────────────────────────────────────────

local ball_entity, paddle_entity
local bricks = {}   -- list of {entity, alive}
local ball_vx, ball_vy
local game_over = false
local won       = false

local function make_entity(x, y, w, h, r, g, b)
  local e = world.create_entity()
  world.add_transform(e, x, y, w, h)
  world.add_sprite(e, r, g, b, 255)
  return e
end

local function init()
  -- ball
  local bx = W / 2 - BALL_S / 2
  local by = H - 160
  ball_entity = make_entity(bx, by, BALL_S, BALL_S, 255, 255, 255)
  ball_vx, ball_vy = 260, -320

  -- paddle
  local px = W / 2 - PAD_W / 2
  local py = H - 48
  paddle_entity = make_entity(px, py, PAD_W, PAD_H, 80, 160, 255)

  -- bricks
  bricks = {}
  local colours = {
    {220, 50,  50 },  -- row 1 red
    {220, 130, 50 },  -- row 2 orange
    {220, 220, 50 },  -- row 3 yellow
    {50,  220, 50 },  -- row 4 green
    {50,  180, 220},  -- row 5 cyan
    {130, 50,  220},  -- row 6 purple
  }
  for row = 0, BRICK_ROWS - 1 do
    local c = colours[row + 1]
    for col = 0, BRICK_COLS - 1 do
      local bkx = BRICK_OFFSET_X + col * (BRICK_W + BRICK_PADDING)
      local bky = BRICK_OFFSET_Y + row * (BRICK_H + BRICK_PADDING)
      local e   = make_entity(bkx, bky, BRICK_W, BRICK_H, c[1], c[2], c[3])
      world.add_tag(e, "brick")
      table.insert(bricks, {entity = e, alive = true})
    end
  end

  game_over = false
  won       = false
  log("breakout.lua loaded")
end

local function reset()
  -- destroy everything and re-init
  world.destroy_entity(ball_entity)
  world.destroy_entity(paddle_entity)
  for _, b in ipairs(bricks) do
    if b.alive then world.destroy_entity(b.entity) end
  end
  init()
end

init()

-- ── update ───────────────────────────────────────────────────────────────────

local PAD_SPEED = 520

engine.on_update(function(dt)
  if game_over or won then
    if engine.is_key_pressed("R") then reset() end
    return
  end

  -- paddle movement
  local px, py = world.get_position(paddle_entity)
  if engine.is_key_pressed("LEFT") then
    px = math.max(0, px - PAD_SPEED * dt)
  end
  if engine.is_key_pressed("RIGHT") then
    px = math.min(W - PAD_W, px + PAD_SPEED * dt)
  end
  world.set_position(paddle_entity, px, py)

  -- move ball
  local bx, by = world.get_position(ball_entity)
  bx = bx + ball_vx * dt
  by = by + ball_vy * dt

  -- wall collisions (left / right)
  if bx <= 0 then
    bx = 0; ball_vx = math.abs(ball_vx)
  elseif bx + BALL_S >= W then
    bx = W - BALL_S; ball_vx = -math.abs(ball_vx)
  end

  -- ceiling
  if by <= 0 then
    by = 0; ball_vy = math.abs(ball_vy)
  end

  -- fell below screen -> game over
  if by > H then
    game_over = true
    log("game over! press R to restart")
    world.set_position(ball_entity, bx, by)
    return
  end

  -- paddle collision
  if overlaps(bx, by, BALL_S, BALL_S, px, py, PAD_W, PAD_H) then
    -- bias horizontal velocity based on where ball hit the paddle
    local hit_pos = (bx + BALL_S / 2) - (px + PAD_W / 2)  -- -60..+60
    ball_vx = hit_pos * 5.5
    ball_vy = -math.abs(ball_vy)  -- always bounce up
    by = py - BALL_S              -- push out
  end

  -- brick collisions
  local alive_count = 0
  for _, brick in ipairs(bricks) do
    if brick.alive then
      alive_count = alive_count + 1
      local rx, ry = world.get_position(brick.entity)
      if overlaps(bx, by, BALL_S, BALL_S, rx, ry, BRICK_W, BRICK_H) then
        local side = hit_side(bx, by, BALL_S, BALL_S, rx, ry, BRICK_W, BRICK_H)
        if side == "left" or side == "right" then
          ball_vx = -ball_vx
        else
          ball_vy = -ball_vy
        end
        world.destroy_entity(brick.entity)
        brick.alive = false
        alive_count = alive_count - 1
      end
    end
  end

  world.set_position(ball_entity, bx, by)

  if alive_count == 0 then
    won = true
    log("you win! press R to play again")
  end
end)
