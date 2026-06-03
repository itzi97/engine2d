-- breakout.lua
-- Controls: LEFT / RIGHT arrows to move paddle. R to reset after game over.

local W, H   = 1280, 720
local BALL_S = 16
local PAD_W, PAD_H = 120, 14
local BRICK_W, BRICK_H = 74, 24
local BRICK_COLS, BRICK_ROWS = 14, 6
local BRICK_PADDING = 8
local BRICK_OFFSET_X = 43
local BRICK_OFFSET_Y = 60

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
  local bx = W / 2 - BALL_S / 2
  local by = H - 160
  ball_entity = make_entity(bx, by, BALL_S, BALL_S, 255, 255, 255)
  ball_vx, ball_vy = 260, -320

  local px = W / 2 - PAD_W / 2
  local py = H - 48
  paddle_entity = make_entity(px, py, PAD_W, PAD_H, 80, 160, 255)
  world.add_tag(paddle_entity, "paddle")

  bricks = {}
  local colours = {
    {220, 50,  50 },
    {220, 130, 50 },
    {220, 220, 50 },
    {50,  220, 50 },
    {50,  180, 220},
    {130, 50,  220},
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
  world.destroy_entity(ball_entity)
  world.destroy_entity(paddle_entity)
  for _, b in ipairs(bricks) do
    if b.alive then world.destroy_entity(b.entity) end
  end
  init()
end

init()

-- ── AABB side detection (still needed for bounce direction) ──────────────────
-- CollisionSystem tells us *that* a collision happened; we still need to know
-- *which side* the ball hit so we can reflect the right velocity component.

local function hit_side(ax, ay, aw, ah, bx, by, bw, bh)
  local ol  = (ax + aw) - bx
  local or_ = (bx + bw) - ax
  local ot  = (ay + ah) - by
  local ob  = (by + bh) - ay
  local min_h = math.min(ol, or_)
  local min_v = math.min(ot, ob)
  if min_h < min_v then
    return ol < or_ and "left" or "right"
  else
    return ot < ob and "top" or "bottom"
  end
end

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

  -- move ball manually (not kinematic — needs custom paddle bounce logic)
  local bx, by = world.get_position(ball_entity)
  bx = bx + ball_vx * dt
  by = by + ball_vy * dt

  -- wall collisions
  if bx <= 0 then
    bx = 0; ball_vx = math.abs(ball_vx)
  elseif bx + BALL_S >= W then
    bx = W - BALL_S; ball_vx = -math.abs(ball_vx)
  end
  if by <= 0 then
    by = 0; ball_vy = math.abs(ball_vy)
  end
  if by > H then
    game_over = true
    log("game over! press R to restart")
    world.set_position(ball_entity, bx, by)
    return
  end

  -- commit ball position so CollisionSystem sees it this frame
  world.set_position(ball_entity, bx, by)

  -- ── collision queries via engine ─────────────────────────────────────────

  -- paddle
  local ball_hits = world.get_collisions_for(ball_entity)
  for _, col in ipairs(ball_hits) do
    local other = col.a == ball_entity and col.b or col.a
    local tag   = world.get_tag(other)

    if tag == "paddle" then
      local hit_pos = (bx + BALL_S / 2) - (px + PAD_W / 2)
      ball_vx = hit_pos * 5.5
      ball_vy = -math.abs(ball_vy)
      by = py - BALL_S
      world.set_position(ball_entity, bx, by)
    end
  end

  -- bricks — use tagged query so we don't iterate dead bricks
  local brick_hits = world.get_collisions_tagged("brick")
  local alive_count = 0
  -- build a fast lookup of which brick entities were hit this frame
  local hit_set = {}
  for _, col in ipairs(brick_hits) do
    local brick_e = world.get_tag(col.a) == "brick" and col.a or col.b
    local other_e = brick_e == col.a and col.b or col.a
    if other_e == ball_entity then
      hit_set[brick_e] = true
    end
  end

  local bounced = false
  for _, brick in ipairs(bricks) do
    if brick.alive then
      if hit_set[brick.entity] then
        if not bounced then
          -- determine side for reflection
          local rx, ry = world.get_position(brick.entity)
          local side = hit_side(bx, by, BALL_S, BALL_S, rx, ry, BRICK_W, BRICK_H)
          if side == "left" or side == "right" then
            ball_vx = -ball_vx
          else
            ball_vy = -ball_vy
          end
          bounced = true  -- only reflect once per frame even if multi-brick
        end
        world.destroy_entity(brick.entity)
        brick.alive = false
      else
        alive_count = alive_count + 1
      end
    end
  end

  if alive_count == 0 then
    won = true
    log("you win! press R to play again")
  end
end)
