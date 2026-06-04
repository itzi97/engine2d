-- asteroids.lua
-- Controls: LEFT/RIGHT rotate, UP thrust, SPACE fire, P pause, ESCAPE main menu
-- Waves are loaded from Tiled .tmj files (object layer only).
-- Each asteroid object carries "speed" and "size" custom properties.

dofile("scripts/util.lua")

-- ---------------------------------------------------------------------------
-- Constants
-- ---------------------------------------------------------------------------
local W, H         = 800, 720
local SHEET        = "assets/sprites/simpleSpace_sheet.png"
local MAP_WAVE     = {
  "assets/maps/asteroids_wave1.tmj",
  "assets/maps/asteroids_wave2.tmj",
}

local ROT_SPEED    = 180   -- degrees / sec
local THRUST       = 260   -- pixels / sec²
local DRAG         = 0.98  -- velocity multiplier per frame
local BULLET_SPEED = 520
local BULLET_LIFE  = 1.2   -- seconds
local FIRE_COOLDOWN = 0.22 -- seconds between shots
local MAX_LIVES    = 3
local INVULN_TIME  = 2.0   -- seconds of invulnerability after respawn

-- Asteroid render size by size class
local ASTEROID_RENDER = { large = 48, medium = 32, small = 20 }

-- Ship frames from sprite sheet
local SHIP_FRAMES = {
  { x =  64, y =   0, w = 48, h = 32 },
  { x =  60, y =  32, w = 48, h = 48 },
  { x =  56, y =  92, w = 48, h = 48 },
}
-- Asteroid frame from sprite sheet
local ROCK_SRC = { x = 144, y = 380, w = 48, h = 48 }

-- ---------------------------------------------------------------------------
-- Assets
-- ---------------------------------------------------------------------------
engine.set_window_title("Asteroids")
engine.set_window_size(W, H)

local tex           = engine.load_texture(SHEET)
local sfx_laser     = engine.load_sfx("assets/audio/sfx/laserSmall_000.ogg")
local sfx_explosion = engine.load_sfx("assets/audio/sfx/explosionCrunch_000.ogg")

-- ---------------------------------------------------------------------------
-- State
-- ---------------------------------------------------------------------------
local wave         = 1
local score        = 0
local lives        = MAX_LIVES
local paused       = false
local game_over    = false
local wave_clear   = false   -- brief "WAVE CLEAR" flash
local wc_timer     = 0

-- Player
local player       = nil
local ship_angle   = 0    -- degrees, 0 = up
local invuln       = 0    -- seconds remaining
local fire_cd      = 0    -- seconds until next shot allowed

-- Tables of live entity IDs
local asteroids    = {}   -- { id, vx, vy, rot_speed, size }
local bullets      = {}   -- { id, vx, vy, life }
local explosions   = {}   -- { id, timer }

-- HUD entities
local hud_score    = nil
local hud_lives    = nil
local hud_wave     = nil
local hud_overlay  = nil  -- GAME OVER / WAVE CLEAR text

-- ---------------------------------------------------------------------------
-- Helpers
-- ---------------------------------------------------------------------------
local function deg2rad(d) return d * math.pi / 180 end

local function wrap(x, y)
  if x >  W + 24 then x = -24 end
  if x < -24     then x =  W + 24 end
  if y >  H + 24 then y = -24 end
  if y < -24     then y =  H + 24 end
  return x, y
end

local function spawn_explosion(x, y)
  local e = world.create_entity()
  world.add_transform(e, x - 16, y - 16, 32, 32)
  world.add_sprite(e, 255, 255, 255, 255, 10)
  if tex then
    world.set_sprite_texture(e, tex, 52, 196, 48, 48)
    world.add_animation(e, {
      { x = 52, y = 196, w = 48, h = 48 },
      { x = 52, y = 148, w = 48, h = 48 },
    }, 0.12, false)
  end
  explosions[#explosions + 1] = { id = e, timer = 0.28 }
  if sfx_explosion ~= -1 then engine.play_sfx(sfx_explosion, 0.7) end
end

local function spawn_asteroid(x, y, size, speed_mul)
  local rs = ASTEROID_RENDER[size] or 32
  local e  = world.create_entity()
  world.add_transform(e, x - rs/2, y - rs/2, rs, rs)
  world.add_sprite(e, 255, 255, 255, 255, 1)
  if tex then
    world.set_sprite_texture(e, tex, ROCK_SRC.x, ROCK_SRC.y, ROCK_SRC.w, ROCK_SRC.h)
    local tint = size == "large" and {180,180,200} or size == "medium" and {160,200,160} or {200,160,140}
    world.set_sprite_tint(e, tint[1], tint[2], tint[3])
  end
  world.add_tag(e, "asteroid")
  local angle = math.random() * math.pi * 2
  local spd   = (size == "large" and 55 or size == "medium" and 85 or 120) * (speed_mul or 1.0)
  local vx    = math.cos(angle) * spd
  local vy    = math.sin(angle) * spd
  local rot   = (math.random() - 0.5) * 120
  world.set_velocity(e, vx, vy)
  asteroids[#asteroids + 1] = { id = e, vx = vx, vy = vy, rot_speed = rot, rot = 0, size = size }
end

local function spawn_player(x, y)
  player     = world.create_entity()
  ship_angle = 0
  world.add_transform(player, x - 24, y - 24, 48, 48)
  world.add_sprite(player, 255, 255, 255, 255, 5)
  if tex then
    world.set_sprite_texture(player, tex,
      SHIP_FRAMES[1].x, SHIP_FRAMES[1].y,
      SHIP_FRAMES[1].w, SHIP_FRAMES[1].h)
    world.add_animation(player, SHIP_FRAMES, 0.12)
  end
  world.add_tag(player, "player")
  world.set_velocity(player, 0, 0)
  invuln  = INVULN_TIME
  fire_cd = 0
end

local function update_hud()
  if hud_score then world.set_text(hud_score, "SCORE  " .. score) end
  if hud_lives then
    local s = "LIVES "
    for i = 1, lives do s = s .. "|" end
    world.set_text(hud_lives, s)
  end
  if hud_wave then world.set_text(hud_wave, "WAVE " .. wave) end
end

-- ---------------------------------------------------------------------------
-- Load wave from Tiled map
-- ---------------------------------------------------------------------------
local function load_wave(w_idx)
  asteroids  = {}
  bullets    = {}
  explosions = {}
  wave_clear = false
  wc_timer   = 0

  local map_path = MAP_WAVE[w_idx] or MAP_WAVE[#MAP_WAVE]
  local objects  = world.load_tiled_map(map_path)

  local spawn_x, spawn_y = W/2, H/2

  for name, obj in pairs(objects) do
    if obj.type == "player_spawn" then
      spawn_x = obj.x + obj.w / 2
      spawn_y = obj.y + obj.h / 2
    end
  end

  spawn_player(spawn_x, spawn_y)

  for name, obj in pairs(objects) do
    if obj.type == "asteroid" then
      local speed_mul = tonumber(obj.properties and obj.properties["speed"]) or 1.0
      local size      = (obj.properties and obj.properties["size"]) or "large"
      -- Give the pre-created entity (obj.entity) a tag and treat it as our asteroid
      -- We re-spawn via our helper to get velocity + rotation state tracking.
      -- Destroy the auto-spawned entity from load_tiled_map first.
      world.destroy_entity(obj.entity)
      local cx = obj.x + obj.w / 2
      local cy = obj.y + obj.h / 2
      spawn_asteroid(cx, cy, size, speed_mul)
    end
  end
end

-- ---------------------------------------------------------------------------
-- Build HUD (called once, survives scene reload via load_wave)
-- ---------------------------------------------------------------------------
local function build_hud()
  hud_score   = make_label(10,  8, "SCORE  0",    18, 255, 255, 255)
  hud_lives   = make_label(10, 34, "LIVES ||||",  18, 255, 220,  80)
  hud_wave    = make_label(W - 110, 8, "WAVE 1",  18, 180, 220, 255)
  hud_overlay = make_label(W/2 - 140, H/2 - 20, "", 36, 255, 80, 80)
  world.set_layer(hud_score,   50)
  world.set_layer(hud_lives,   50)
  world.set_layer(hud_wave,    50)
  world.set_layer(hud_overlay, 50)
  update_hud()
end

local pause_ui = nil

local function init()
  wave       = 1
  score      = 0
  lives      = MAX_LIVES
  paused     = false
  game_over  = false

  build_hud()
  pause_ui = make_pause_overlay(W/2, H/2 - 80)
  pause_ui.hide()

  load_wave(wave)
end

-- ---------------------------------------------------------------------------
-- Pause handling
-- ---------------------------------------------------------------------------
local pause_sel = 1

local function toggle_pause()
  paused = not paused
  if paused then
    pause_sel = 1
    pause_ui.show(pause_sel)
  else
    pause_ui.hide()
  end
end

-- ---------------------------------------------------------------------------
-- on_update
-- ---------------------------------------------------------------------------
init()

engine.on_update(function(dt)
  -- ---- pause menu -----------------------------------------------------------
  if engine.is_key_just_pressed("P") and not game_over then
    toggle_pause()
  end

  if paused then
    if engine.is_key_just_pressed("UP")   then pause_sel = 1; pause_ui.show(pause_sel) end
    if engine.is_key_just_pressed("DOWN") then pause_sel = 2; pause_ui.show(pause_sel) end
    if engine.is_key_just_pressed("RETURN") or
       (pause_sel == 1 and engine.is_key_just_pressed("R")) or
       (pause_sel == 2 and engine.is_key_just_pressed("M")) then
      if pause_sel == 1 then
        toggle_pause()
      else
        engine.load_scene(function()
          dofile("scripts/main_menu.lua")
        end)
      end
    end
    return
  end

  -- ---- game over screen -----------------------------------------------------
  if game_over then
    if engine.is_key_just_pressed("RETURN") then
      engine.load_scene(function() dofile("scripts/asteroids.lua") end)
    end
    if engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
    return
  end

  -- ---- wave clear pause -----------------------------------------------------
  if wave_clear then
    wc_timer = wc_timer - dt
    if wc_timer <= 0 then
      wave = wave + 1
      engine.load_scene(function() dofile("scripts/asteroids.lua") end)
    end
    return
  end

  -- ---- timers ---------------------------------------------------------------
  invuln  = math.max(0, invuln  - dt)
  fire_cd = math.max(0, fire_cd - dt)

  -- ---- player input ---------------------------------------------------------
  if player then
    if engine.is_key_pressed("LEFT")  then ship_angle = ship_angle - ROT_SPEED * dt end
    if engine.is_key_pressed("RIGHT") then ship_angle = ship_angle + ROT_SPEED * dt end
    world.set_rotation(player, ship_angle)

    if engine.is_key_pressed("UP") then
      local rad = deg2rad(ship_angle - 90)
      local vx, vy = world.get_velocity(player)
      vx = vx + math.cos(rad) * THRUST * dt
      vy = vy + math.sin(rad) * THRUST * dt
      world.set_velocity(player, vx * DRAG, vy * DRAG)
    else
      local vx, vy = world.get_velocity(player)
      world.set_velocity(player, vx * DRAG, vy * DRAG)
    end

    -- wrap player
    local px, py = world.get_position(player)
    local nx, ny = wrap(px + 24, py + 24)
    world.set_position(player, nx - 24, ny - 24)

    -- fire
    if engine.is_key_just_pressed("SPACE") and fire_cd == 0 then
      fire_cd = FIRE_COOLDOWN
      local rad = deg2rad(ship_angle - 90)
      local bx  = px + 24 + math.cos(rad) * 26
      local by  = py + 24 + math.sin(rad) * 26
      local b   = world.create_entity()
      world.add_transform(b, bx - 2, by - 6, 4, 10)
      world.add_sprite(b, 255, 255, 120, 255, 3)
      world.add_tag(b, "bullet")
      local pvx, pvy = world.get_velocity(player)
      local bvx = math.cos(rad) * BULLET_SPEED + pvx
      local bvy = math.sin(rad) * BULLET_SPEED + pvy
      world.set_velocity(b, bvx, bvy)
      bullets[#bullets + 1] = { id = b, vx = bvx, vy = bvy, life = BULLET_LIFE }
      if sfx_laser ~= -1 then engine.play_sfx(sfx_laser, 0.8) end
    end
  end

  -- ---- move asteroids (manual wrap) -----------------------------------------
  for _, a in ipairs(asteroids) do
    local ax, ay = world.get_position(a.id)
    local rs     = ASTEROID_RENDER[a.size] or 32
    local cx, cy = wrap(ax + rs/2, ay + rs/2)
    world.set_position(a.id, cx - rs/2, cy - rs/2)
    a.rot = a.rot + a.rot_speed * dt
    world.set_rotation(a.id, a.rot)
  end

  -- ---- move & age bullets ---------------------------------------------------
  local live_bullets = {}
  for _, b in ipairs(bullets) do
    b.life = b.life - dt
    if b.life <= 0 then
      world.destroy_entity(b.id)
    else
      local bx, by = world.get_position(b.id)
      local nx, ny = wrap(bx + 2, by + 5)
      world.set_position(b.id, nx - 2, ny - 5)
      live_bullets[#live_bullets + 1] = b
    end
  end
  bullets = live_bullets

  -- ---- age explosions -------------------------------------------------------
  local live_exp = {}
  for _, ex in ipairs(explosions) do
    ex.timer = ex.timer - dt
    if ex.timer <= 0 then
      world.destroy_entity(ex.id)
    else
      live_exp[#live_exp + 1] = ex
    end
  end
  explosions = live_exp

  -- ---- collision: bullet vs asteroid ----------------------------------------
  local hit_asteroids = {}
  local hit_bullets   = {}

  for _, b in ipairs(bullets) do
    local cols = world.get_collisions_for(b.id)
    for _, col in ipairs(cols) do
      local other = col.a == b.id and col.b or col.a
      if world.get_tag(other) == "asteroid" then
        hit_bullets[b.id]     = true
        hit_asteroids[other]  = true
      end
    end
  end

  -- remove hit bullets
  local kept_bullets = {}
  for _, b in ipairs(bullets) do
    if hit_bullets[b.id] then
      world.destroy_entity(b.id)
    else
      kept_bullets[#kept_bullets + 1] = b
    end
  end
  bullets = kept_bullets

  -- remove hit asteroids, spawn fragments, score
  local kept_asteroids = {}
  for _, a in ipairs(asteroids) do
    if hit_asteroids[a.id] then
      local ax, ay = world.get_position(a.id)
      local rs     = ASTEROID_RENDER[a.size] or 32
      spawn_explosion(ax + rs/2, ay + rs/2)
      world.destroy_entity(a.id)
      -- score
      score = score + (a.size == "large" and 20 or a.size == "medium" and 50 or 100)
      update_hud()
      -- fragment: large -> 2 medium, medium -> 2 small
      if a.size == "large" then
        spawn_asteroid(ax + rs/2, ay + rs/2, "medium", 1.0)
        spawn_asteroid(ax + rs/2, ay + rs/2, "medium", 1.0)
      elseif a.size == "medium" then
        spawn_asteroid(ax + rs/2, ay + rs/2, "small", 1.0)
        spawn_asteroid(ax + rs/2, ay + rs/2, "small", 1.0)
      end
    else
      kept_asteroids[#kept_asteroids + 1] = a
    end
  end
  asteroids = kept_asteroids

  -- ---- collision: asteroid vs player ----------------------------------------
  if player and invuln == 0 then
    local cols = world.get_collisions_for(player)
    for _, col in ipairs(cols) do
      local other = col.a == player and col.b or col.a
      if world.get_tag(other) == "asteroid" then
        local px, py = world.get_position(player)
        spawn_explosion(px + 24, py + 24)
        world.destroy_entity(player)
        player = nil
        lives  = lives - 1
        update_hud()
        if lives <= 0 then
          game_over = true
          if hud_overlay then
            world.set_text(hud_overlay, "GAME OVER   ENTER restart   ESC menu")
            world.set_text_color(hud_overlay, 255, 60, 60, 255)
          end
        else
          -- respawn after brief delay via invuln on next spawn
          spawn_player(W/2, H/2)
        end
        break
      end
    end
  end

  -- invuln flash
  if player and invuln > 0 then
    local flash = math.floor(invuln * 10) % 2 == 0
    world.set_sprite_tint(player, 255, 255, 255, flash and 255 or 80)
  elseif player then
    world.set_sprite_tint(player, 255, 255, 255, 255)
  end

  -- ---- wave clear check -----------------------------------------------------
  if #asteroids == 0 and not game_over and not wave_clear then
    wave_clear = true
    wc_timer   = 2.5
    if hud_overlay then
      world.set_text(hud_overlay, "WAVE CLEAR!")
      world.set_text_color(hud_overlay, 80, 255, 140, 255)
    end
  end

  if engine.is_key_just_pressed("ESCAPE") then
    engine.load_scene(function() dofile("scripts/main_menu.lua") end)
  end
end)
