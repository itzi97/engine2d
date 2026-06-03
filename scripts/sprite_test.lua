-- sprite_test.lua
-- Tests engine.load_texture, world.set_sprite_texture, set_sprite_src,
-- set_sprite_flip. Uses the bundled simpleSpace_sheet.png atlas.
--
-- Build with: cmake -B build -DGAME=sprite_test && cmake --build build -j
-- Run from repo root: ./build/2d-engine

local SHEET = "assets/sprites/simpleSpace_sheet.png"

-- Coords taken directly from simpleSpace_sheet.xml
local SPR = {
  ship_D   = { x =  64, y =   0, w = 48, h = 32 },  -- small fighter
  ship_G   = { x =  60, y =  32, w = 48, h = 48 },  -- medium fighter
  ship_F   = { x =  96, y = 388, w = 48, h = 48 },  -- large fighter
  enemy_A  = { x =   0, y = 420, w = 48, h = 48 },  -- enemy saucer
  enemy_B  = { x = 100, y = 140, w = 48, h = 48 },  -- enemy wedge
  meteor   = { x = 144, y = 380, w = 48, h = 48 },  -- round meteor
  star     = { x =  52, y = 196, w = 48, h = 48 },  -- large star
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

  -- Row 1: three different ships
  make_sprite( 50,  50, 96, 64, SPR.ship_D)
  make_sprite(200,  50, 96, 96, SPR.ship_G)
  make_sprite(350,  50, 96, 96, SPR.ship_F)

  -- Row 2: enemies, normal + H-flipped
  local eA = make_sprite( 50, 200, 96, 96, SPR.enemy_A)
  local eB = make_sprite(200, 200, 96, 96, SPR.enemy_B)
  world.set_sprite_flip(eB, true, false)

  -- Row 3: meteor + star (non-character sprites)
  make_sprite( 50, 360, 96, 96, SPR.meteor)
  make_sprite(200, 360, 96, 96, SPR.star)

  -- One ship flipped vertically to test that axis
  local ship_vflip = make_sprite(350, 200, 96, 96, SPR.ship_G)
  world.set_sprite_flip(ship_vflip, false, true)

  log("[sprite_test] scene ready: 8 sprites, 3 rows")
end

load_scene()

engine.on_update(function(dt)
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)
