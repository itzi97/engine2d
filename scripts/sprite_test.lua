-- sprite_test.lua
-- Tests textures + AnimationSystem.
-- One entity cycles through all ship variants as an animation,
-- another shows a static meteor for comparison.
--
-- Build: cmake -B build -DGAME=sprite_test && cmake --build build -j
-- Run:   ./build/2d-engine  (from repo root)

local SHEET = "assets/sprites/simpleSpace_sheet.png"

-- All ship frames from simpleSpace_sheet.xml, in order
local SHIP_FRAMES = {
  { x =  64, y =   0, w = 48, h = 32 },  -- ship_D
  { x =  60, y =  32, w = 48, h = 48 },  -- ship_G
  { x =  56, y =  92, w = 48, h = 48 },  -- ship_H
  { x =  52, y = 244, w = 48, h = 48 },  -- ship_L
  { x =  52, y = 292, w = 48, h = 48 },  -- ship_J
  { x =  96, y = 388, w = 48, h = 48 },  -- ship_F
  { x =  96, y = 436, w = 48, h = 48 },  -- ship_E
}

-- Enemy frames
local ENEMY_FRAMES = {
  { x =   0, y =   0, w = 64, h = 32 },  -- enemy_C
  { x = 100, y = 140, w = 48, h = 48 },  -- enemy_B
  { x = 100, y = 188, w = 48, h = 48 },  -- enemy_D
  { x = 100, y = 332, w = 48, h = 48 },  -- enemy_E
  { x =   0, y = 420, w = 48, h = 48 },  -- enemy_A
}

local tex

local function load_scene()
  tex = engine.load_texture(SHEET)
  if not tex then
    log("[sprite_test] ERROR: could not load " .. SHEET)
    return
  end

  -- Animated ship: cycles through all ship variants at 0.15s/frame
  local ship = world.create_entity()
  world.add_transform(ship, 80, 80, 96, 96)
  world.add_sprite(ship, 255, 255, 255, 255)
  world.set_sprite_texture(ship, tex, SHIP_FRAMES[1].x, SHIP_FRAMES[1].y,
                                      SHIP_FRAMES[1].w, SHIP_FRAMES[1].h)
  world.add_animation(ship, SHIP_FRAMES, 0.15)

  -- Animated enemy: cycles through enemy variants at 0.2s/frame
  local enemy = world.create_entity()
  world.add_transform(enemy, 240, 80, 96, 96)
  world.add_sprite(enemy, 255, 255, 255, 255)
  world.set_sprite_texture(enemy, tex, ENEMY_FRAMES[1].x, ENEMY_FRAMES[1].y,
                                       ENEMY_FRAMES[1].w, ENEMY_FRAMES[1].h)
  world.add_animation(enemy, ENEMY_FRAMES, 0.2)

  -- Static meteor for comparison (no AnimationComponent)
  local rock = world.create_entity()
  world.add_transform(rock, 400, 80, 96, 96)
  world.add_sprite(rock, 255, 255, 255, 255)
  world.set_sprite_texture(rock, tex, 144, 380, 48, 48)  -- meteor_large

  -- One-shot animation (loop=false): plays once then holds last frame
  local star = world.create_entity()
  world.add_transform(star, 80, 240, 96, 96)
  world.add_sprite(star, 255, 255, 255, 255)
  world.set_sprite_texture(star, tex, 52, 196, 48, 48)
  world.add_animation(star, {
    { x = 52, y = 196, w = 48, h = 48 },  -- star_large
    { x = 52, y = 148, w = 48, h = 48 },  -- star_medium
  }, 0.3, false)  -- loop=false

  log("[sprite_test] scene ready")
end

load_scene()

engine.on_update(function(dt)
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)
