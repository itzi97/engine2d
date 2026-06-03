-- sprite_test.lua
-- Tests engine.load_texture, world.set_sprite_texture, set_sprite_src,
-- set_sprite_flip. Uses the bundled simpleSpace_sheet.png atlas.
--
-- Build with: cmake -DGAME=sprite_test ...

local SHEET = "assets/sprites/simpleSpace_sheet.png"

-- Atlas coords for a couple of sprites inside simpleSpace_sheet
-- (all values in pixels, verified against simpleSpace_sheet.xml)
local SPR = {
  ship    = { x =   0, y =   0, w = 99, h = 75 },
  enemy   = { x =   0, y = 144, w = 82, h = 84 },
  laser   = { x = 843, y =   0, w = 13, h = 57 },
}

local tex

local function make_sprite(x, y, w, h, src)
  local e = world.create_entity()
  world.add_transform(e, x, y, w, h)
  world.add_sprite(e, 255, 255, 255, 255)
  world.set_sprite_texture(e, tex, src.x, src.y, src.w, src.h)
  return e
end

local function load_scene()
  tex = engine.load_texture(SHEET)
  if not tex then
    log("[sprite_test] ERROR: could not load " .. SHEET)
    return
  end
  log("[sprite_test] texture loaded OK")

  -- Row 1: player ship, normal + H-flipped + V-flipped
  make_sprite( 50,  80, 99, 75, SPR.ship)

  local ship_flip_h = make_sprite(200,  80, 99, 75, SPR.ship)
  world.set_sprite_flip(ship_flip_h, true, false)

  local ship_flip_v = make_sprite(350,  80, 99, 75, SPR.ship)
  world.set_sprite_flip(ship_flip_v, false, true)

  -- Row 2: enemy, normal + both axes flipped
  make_sprite( 50, 220, 82, 84, SPR.enemy)

  local enemy_flip = make_sprite(200, 220, 82, 84, SPR.enemy)
  world.set_sprite_flip(enemy_flip, true, true)

  -- Row 3: laser (tall thin sprite — tests non-square src rects)
  make_sprite( 50, 380, 13, 57, SPR.laser)
  make_sprite(100, 380, 13, 57, SPR.laser)
  make_sprite(150, 380, 13, 57, SPR.laser)

  log("[sprite_test] scene ready")
end

load_scene()

engine.on_update(function(dt)
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)
