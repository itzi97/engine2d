-- tests/sprite_test.lua  (manual dev harness — not reachable from the main menu)
-- Tests textures + AnimationSystem + sprite tinting.
--
-- Build: cmake -B build -DGAME=sprite_test && cmake --build build -j
-- Run:   ./build/2d-engine

local SHEET = "assets/sprites/simpleSpace_sheet.png"

local SHIP_FRAMES = {
  { x =  64, y =   0, w = 48, h = 32 },
  { x =  60, y =  32, w = 48, h = 48 },
  { x =  56, y =  92, w = 48, h = 48 },
  { x =  52, y = 244, w = 48, h = 48 },
  { x =  52, y = 292, w = 48, h = 48 },
  { x =  96, y = 388, w = 48, h = 48 },
  { x =  96, y = 436, w = 48, h = 48 },
}

local ENEMY_FRAMES = {
  { x =   0, y =   0, w = 64, h = 32 },
  { x = 100, y = 140, w = 48, h = 48 },
  { x = 100, y = 188, w = 48, h = 48 },
  { x = 100, y = 332, w = 48, h = 48 },
  { x =   0, y = 420, w = 48, h = 48 },
}

local fade_star  = nil
local fade_alpha = 255

local tex = engine.load_texture(SHEET)
if not tex then
  log("[sprite_test] ERROR: could not load " .. SHEET)
else
  local ship = world.create_entity()
  world.add_transform(ship, 40, 60, 96, 96)
  world.add_sprite(ship, 255, 255, 255, 255)
  world.set_sprite_texture(ship, tex, SHIP_FRAMES[1].x, SHIP_FRAMES[1].y,
                                      SHIP_FRAMES[1].w, SHIP_FRAMES[1].h)
  world.add_animation(ship, SHIP_FRAMES, 0.15)

  local enemy = world.create_entity()
  world.add_transform(enemy, 180, 60, 96, 96)
  world.add_sprite(enemy, 255, 255, 255, 255)
  world.set_sprite_texture(enemy, tex, ENEMY_FRAMES[1].x, ENEMY_FRAMES[1].y,
                                       ENEMY_FRAMES[1].w, ENEMY_FRAMES[1].h)
  world.add_animation(enemy, ENEMY_FRAMES, 0.2)
  world.set_sprite_tint(enemy, 255, 80, 80)

  local rock = world.create_entity()
  world.add_transform(rock, 320, 60, 96, 96)
  world.add_sprite(rock, 255, 255, 255, 255)
  world.set_sprite_texture(rock, tex, 144, 380, 48, 48)
  world.set_sprite_tint(rock, 80, 80, 255)

  fade_star = world.create_entity()
  world.add_transform(fade_star, 40, 220, 96, 96)
  world.add_sprite(fade_star, 255, 255, 255, 255)
  world.set_sprite_texture(fade_star, tex, 52, 196, 48, 48)
  world.add_animation(fade_star, {
    { x = 52, y = 196, w = 48, h = 48 },
    { x = 52, y = 148, w = 48, h = 48 },
  }, 0.3, false)

  local ship2 = world.create_entity()
  world.add_transform(ship2, 180, 220, 96, 96)
  world.add_sprite(ship2, 255, 255, 255, 255)
  world.set_sprite_texture(ship2, tex, SHIP_FRAMES[1].x, SHIP_FRAMES[1].y,
                                       SHIP_FRAMES[1].w, SHIP_FRAMES[1].h)
  world.set_sprite_tint(ship2, 80, 255, 80)

  log("[sprite_test] scene ready")
end

engine.on_update(function(dt)
  if fade_star and fade_alpha > 0 then
    fade_alpha = math.max(0, fade_alpha - dt * 80)
    world.set_sprite_tint(fade_star, 255, 255, 255, math.floor(fade_alpha))
  end
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)
