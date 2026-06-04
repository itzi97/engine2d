-- asteroids.lua
-- Controls: LEFT/RIGHT rotate, UP thrust, SPACE fire, P pause, ESCAPE main menu

dofile("scripts/util.lua")

-- ---------------------------------------------------------------------------
-- Constants
-- ---------------------------------------------------------------------------
local W, H         = 800, 720
local SHEET        = "assets/sprites/simpleSpace_sheet.png"
local MAP_WAVE     = {
  "assets/maps/asteroids_wave1.tmj",
  "assets/maps/asteroids_wave2.tmj",
  "assets/maps/asteroids_wave3.tmj",
  "assets/maps/asteroids_wave4.tmj",
  "assets/maps/asteroids_wave5.tmj",
}

local ROT_SPEED     = 180
local THRUST        = 260
local DRAG          = 0.98
local BULLET_SPEED  = 520
local BULLET_LIFE   = 1.2
local FIRE_COOLDOWN = 0.22
local MAX_LIVES     = 3
local INVULN_TIME   = 2.0

-- Asteroid base spin (deg/s).  Each rock is randomised in spawn_asteroid.
local SPIN_BASE     = 60

local ASTEROID_RENDER = { large = 48, medium = 32, small = 20 }

local SHIP_FRAMES = {
  { x =  64, y =   0, w = 48, h = 32 },
  { x =  60, y =  32, w = 48, h = 48 },
  { x =  56, y =  92, w = 48, h = 48 },
}
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
local wave       = 1
local score      = 0
local lives      = MAX_LIVES
local paused     = false
local game_over  = false
local you_win    = false
local wave_clear = false
local wc_timer   = 0

local player     = nil
local ship_angle = 0
local invuln     = 0
local fire_cd    = 0

local asteroids  = {}
local bullets    = {}
local explosions = {}

local hud_score       = nil
local hud_lives       = nil
local hud_wave        = nil
local hud_overlay     = nil   -- main message  (large, centred)
local hud_overlay_sub = nil   -- hint line     (small, centred below)

-- ---------------------------------------------------------------------------
-- Helpers
-- ---------------------------------------------------------------------------
local function deg2rad(d) return d * math.pi / 180 end

local function wrap(x, y)
  if x >  W + 24 then x = -24     end
  if x < -24     then x =  W + 24 end
  if y >  H + 24 then y = -24     end
  if y < -24     then y =  H + 24 end
  return x, y
end

-- Centre an overlay label at a given screen position.
-- Uses the anchor API so text is always pixel-perfect regardless of string length.
local function overlay_centre(entity, cx, cy)
  world.set_text_anchor(entity, 0.5, 0.5)
  world.set_position(entity, cx, cy)
end

local function overlay_show(msg, sub, mr, mg, mb, sr, sg, sb)
  world.set_text(hud_overlay, msg)
  world.set_text_color(hud_overlay, mr or 255, mg or 255, mb or 255, 255)
  overlay_centre(hud_overlay, W/2, H/2 - 22)

  world.set_text(hud_overlay_sub, sub)
  world.set_text_color(hud_overlay_sub, sr or 200, sg or 200, sb or 200, 255)
  overlay_centre(hud_overlay_sub, W/2, H/2 + 22)
end

local function overlay_hide()
  world.set_text(hud_overlay,     "")
  world.set_text(hud_overlay_sub, "")
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
    local tint = size == "large" and {180,180,200}
             or size == "medium" and {160,200,160}
             or {200,160,140}
    world.set_sprite_tint(e, tint[1], tint[2], tint[3])
  end
  world.add_tag(e, "asteroid")
  local angle = math.random() * math.pi * 2
  local spd   = (size == "large" and 55 or size == "medium" and 85 or 120) * (speed_mul or 1.0)
  -- Random spin: magnitude 0.5x–3x base, direction random
  local spin_mag = SPIN_BASE * (0.5 + math.random() * 2.5)
  local spin_dir = math.random(0, 1) == 0 and 1 or -1
  asteroids[#asteroids + 1] = {
    id        = e,
    vx        = math.cos(angle) * spd,
    vy        = math.sin(angle) * spd,
    rot_speed = spin_dir * spin_mag,
    rot       = math.random() * 360,
    size      = size,
  }
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
  world.add_kinematic(player)
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
  if hud_wave then world.set_text(hud_wave, "WAVE " .. wave .. " / " .. #MAP_WAVE) end
end

-- ---------------------------------------------------------------------------
-- Load wave (in-place, no scene reload)
-- ---------------------------------------------------------------------------
local function load_wave(w_idx)
  if player then world.destroy_entity(player); player = nil end
  for _, b  in ipairs(bullets)    do world.destroy_entity(b.id)  end
  for _, a  in ipairs(asteroids)  do world.destroy_entity(a.id)  end
  for _, ex in ipairs(explosions) do world.destroy_entity(ex.id) end

  asteroids  = {}
  bullets    = {}
  explosions = {}
  wave_clear = false
  wc_timer   = 0

  overlay_hide()

  local objects  = world.load_tiled_map(MAP_WAVE[w_idx])
  local spawn_x, spawn_y = W/2, H/2

  for _, obj in pairs(objects) do
    if obj.type == "player_spawn" then
      spawn_x = obj.x + obj.w / 2
      spawn_y = obj.y + obj.h / 2
    end
  end

  spawn_player(spawn_x, spawn_y)

  for _, obj in pairs(objects) do
    if obj.type == "asteroid" then
      local speed_mul = tonumber(obj.properties and obj.properties["speed"]) or 1.0
      local size      = (obj.properties and obj.properties["size"]) or "large"
      world.destroy_entity(obj.entity)
      spawn_asteroid(obj.x + obj.w/2, obj.y + obj.h/2, size, speed_mul)
    end
  end

  update_hud()
end

-- ---------------------------------------------------------------------------
-- HUD + pause
-- ---------------------------------------------------------------------------
local function build_hud()
  hud_score       = make_label(10,      8,  "SCORE  0",  18, 255, 255, 255)
  hud_lives       = make_label(10,      34, "LIVES ||||", 18, 255, 220,  80)
  hud_wave        = make_label(W-140,   8,  "WAVE 1 / " .. #MAP_WAVE, 18, 180, 220, 255)
  -- Overlay labels start empty; overlay_show centres them via set_text_anchor
  hud_overlay     = make_label(0, 0, "", 28, 255, 255, 255)
  hud_overlay_sub = make_label(0, 0, "", 18, 200, 200, 200)
  for _, e in ipairs({hud_score, hud_lives, hud_wave, hud_overlay, hud_overlay_sub}) do
    world.set_layer(e, 50)
  end
  update_hud()
end

local pause_ui  = nil
local pause_sel = 1

local function toggle_pause()
  paused = not paused
  if paused then pause_sel = 1; pause_ui:show(pause_sel)
  else pause_ui:hide() end
end

-- ---------------------------------------------------------------------------
-- Init
-- ---------------------------------------------------------------------------
local function init()
  wave      = 1
  score     = 0
  lives     = MAX_LIVES
  paused    = false
  game_over = false
  you_win   = false

  build_hud()
  pause_ui = make_pause_overlay(W/2, H/2 - 80)
  pause_ui:hide()

  load_wave(wave)
end

init()

-- ---------------------------------------------------------------------------
-- on_update
-- ---------------------------------------------------------------------------
engine.on_update(function(dt)

  if engine.is_key_just_pressed("P") and not game_over and not you_win then
    toggle_pause()
  end

  if paused then
    if engine.is_key_just_pressed("UP")   then pause_sel = 1; pause_ui:show(pause_sel) end
    if engine.is_key_just_pressed("DOWN") then pause_sel = 2; pause_ui:show(pause_sel) end
    if engine.is_key_just_pressed("RETURN")
    or (pause_sel == 1 and engine.is_key_just_pressed("R"))
    or (pause_sel == 2 and engine.is_key_just_pressed("M")) then
      if pause_sel == 1 then toggle_pause()
      else engine.load_scene(function() dofile("scripts/main_menu.lua") end) end
    end
    return
  end

  if game_over then
    if engine.is_key_just_pressed("RETURN") then
      engine.load_scene(function() dofile("scripts/asteroids.lua") end)
    end
    if engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
    return
  end

  if you_win then
    if engine.is_key_just_pressed("RETURN") then
      engine.load_scene(function() dofile("scripts/asteroids.lua") end)
    end
    if engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(function() dofile("scripts/main_menu.lua") end)
    end
    return
  end

  -- ---- wave clear -----------------------------------------------------------
  if wave_clear then
    wc_timer = wc_timer - dt
    if wc_timer <= 0 then
      wave = wave + 1
      if wave > #MAP_WAVE then
        you_win = true
        overlay_show("YOU WIN!", "ENTER play again   ESC menu",
                     80, 255, 180,  160, 255, 200)
      else
        load_wave(wave)
      end
    end
    return
  end

  invuln  = math.max(0, invuln  - dt)
  fire_cd = math.max(0, fire_cd - dt)

  if player then
    if engine.is_key_pressed("LEFT")  then ship_angle = ship_angle - ROT_SPEED * dt end
    if engine.is_key_pressed("RIGHT") then ship_angle = ship_angle + ROT_SPEED * dt end
    world.set_rotation(player, ship_angle)

    if engine.is_key_pressed("UP") then
      local rad = deg2rad(ship_angle - 90)
      local vx, vy = world.get_velocity(player)
      vx = (vx + math.cos(rad) * THRUST * dt) * DRAG
      vy = (vy + math.sin(rad) * THRUST * dt) * DRAG
      world.set_velocity(player, vx, vy)
    else
      local vx, vy = world.get_velocity(player)
      world.set_velocity(player, vx * DRAG, vy * DRAG)
    end

    local px, py = world.get_position(player)
    local cx, cy = wrap(px + 24, py + 24)
    world.set_position(player, cx - 24, cy - 24)
    px, py = world.get_position(player)

    if engine.is_key_just_pressed("SPACE") and fire_cd == 0 then
      fire_cd   = FIRE_COOLDOWN
      local rad = deg2rad(ship_angle - 90)
      local bx  = px + 24 + math.cos(rad) * 26
      local by  = py + 24 + math.sin(rad) * 26
      local b   = world.create_entity()
      world.add_transform(b, bx - 2, by - 5, 4, 10)
      world.add_sprite(b, 255, 255, 120, 255, 3)
      world.add_tag(b, "bullet")
      world.set_rotation(b, ship_angle)
      local pvx, pvy = world.get_velocity(player)
      world.add_kinematic(b)
      world.set_velocity(b, math.cos(rad)*BULLET_SPEED + pvx, math.sin(rad)*BULLET_SPEED + pvy)
      bullets[#bullets + 1] = { id = b, life = BULLET_LIFE }
      if sfx_laser ~= -1 then engine.play_sfx(sfx_laser, 0.8) end
    end
  end

  for _, a in ipairs(asteroids) do
    local ax, ay = world.get_position(a.id)
    local rs     = ASTEROID_RENDER[a.size] or 32
    ax = ax + a.vx * dt
    ay = ay + a.vy * dt
    local cx, cy = wrap(ax + rs/2, ay + rs/2)
    world.set_position(a.id, cx - rs/2, cy - rs/2)
    a.rot = a.rot + a.rot_speed * dt
    world.set_rotation(a.id, a.rot)
  end

  local live_bullets = {}
  for _, b in ipairs(bullets) do
    b.life = b.life - dt
    if b.life <= 0 then world.destroy_entity(b.id)
    else live_bullets[#live_bullets + 1] = b end
  end
  bullets = live_bullets

  local live_exp = {}
  for _, ex in ipairs(explosions) do
    ex.timer = ex.timer - dt
    if ex.timer <= 0 then world.destroy_entity(ex.id)
    else live_exp[#live_exp + 1] = ex end
  end
  explosions = live_exp

  local hit_asteroids = {}
  local hit_bullets   = {}
  for _, b in ipairs(bullets) do
    for _, col in ipairs(world.get_collisions_for(b.id)) do
      local other = col.a == b.id and col.b or col.a
      if world.get_tag(other) == "asteroid" then
        hit_bullets[b.id] = true; hit_asteroids[other] = true
      end
    end
  end

  local kept_bullets = {}
  for _, b in ipairs(bullets) do
    if hit_bullets[b.id] then world.destroy_entity(b.id)
    else kept_bullets[#kept_bullets + 1] = b end
  end
  bullets = kept_bullets

  local kept_asteroids = {}
  for _, a in ipairs(asteroids) do
    if hit_asteroids[a.id] then
      local ax, ay = world.get_position(a.id)
      local rs     = ASTEROID_RENDER[a.size] or 32
      spawn_explosion(ax + rs/2, ay + rs/2)
      world.destroy_entity(a.id)
      score = score + (a.size == "large" and 20 or a.size == "medium" and 50 or 100)
      update_hud()
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

  if player and invuln == 0 then
    for _, col in ipairs(world.get_collisions_for(player)) do
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
          overlay_show("GAME OVER", "ENTER restart   ESC menu",
                       255, 60, 60,  220, 120, 120)
        else
          spawn_player(W/2, H/2)
        end
        break
      end
    end
  end

  if player and invuln > 0 then
    local flash = math.floor(invuln * 10) % 2 == 0
    world.set_sprite_tint(player, 255, 255, 255, flash and 255 or 80)
  elseif player then
    world.set_sprite_tint(player, 255, 255, 255, 255)
  end

  if #asteroids == 0 and not game_over and not wave_clear and not you_win then
    wave_clear = true
    wc_timer   = 2.5
    if wave < #MAP_WAVE then
      overlay_show("WAVE CLEAR!", "next wave incoming...",
                   80, 255, 140,  140, 220, 160)
    else
      overlay_show("FINAL WAVE CLEAR!", "get ready...",
                   255, 220, 60,  220, 200, 100)
    end
  end

  if engine.is_key_just_pressed("ESCAPE") then
    engine.load_scene(function() dofile("scripts/main_menu.lua") end)
  end
end)
