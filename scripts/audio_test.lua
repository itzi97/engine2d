-- audio_test.lua
-- Tests the full audio API:
--   SPACE      -> play laser SFX
--   E          -> play explosion SFX
--   M          -> toggle music pause/resume
--   UP/DOWN    -> music volume +/-
--   ESCAPE     -> quit

local SFX_LASER     = "assets/audio/sfx/laserSmall_000.ogg"
local SFX_EXPLOSION = "assets/audio/sfx/explosionCrunch_000.ogg"
local MUSIC_TRACK   = "assets/audio/music/jingles_NES00.ogg"

-- Load assets (returns -1 and prints a warning if the file is missing)
local laser     = engine.load_sfx(SFX_LASER)
local explosion = engine.load_sfx(SFX_EXPLOSION)
local music     = engine.load_music(MUSIC_TRACK)

if laser     == -1 then log("WARNING: could not load " .. SFX_LASER)     end
if explosion == -1 then log("WARNING: could not load " .. SFX_EXPLOSION) end
if music     == -1 then log("WARNING: could not load " .. MUSIC_TRACK)   end

-- Start music looping at half volume
engine.set_music_volume(0.5)
engine.play_music(music, true)

-- HUD
local function make_label(text, y)
  local e = world.create_entity()
  world.add_transform(e, 20, y, 0, 0)
  world.add_text(e, text, 16, 220, 220, 220)
  return e
end

make_label("SPACE       play laser SFX",      30)
make_label("E           play explosion SFX",  55)
make_label("M           pause / resume music",80)
make_label("UP / DOWN   music volume +/-",   105)
make_label("ESCAPE      quit",               130)

local volume    = 0.5
local paused    = false
local vol_label = world.create_entity()
world.add_transform(vol_label, 20, 160, 0, 0)
world.add_text(vol_label, string.format("Music volume: %.0f%%", volume * 100), 16, 100, 220, 100)

engine.on_update(function()
  if engine.is_key_just_pressed("SPACE") and laser ~= -1 then
    engine.play_sfx(laser, 1.0)
  end

  if engine.is_key_just_pressed("F") and explosion ~= -1 then
    engine.play_sfx(explosion, 1.0)
  end

  if engine.is_key_just_pressed("P") then
    if paused then
      engine.resume_music()
      paused = false
    else
      engine.pause_music()
      paused = true
    end
  end

  if engine.is_key_just_pressed("UP") then
    volume = math.min(1.0, volume + 0.1)
    engine.set_music_volume(volume)
    world.set_text(vol_label, string.format("Music volume: %.0f%%", volume * 100))
  end

  if engine.is_key_just_pressed("DOWN") then
    volume = math.max(0.0, volume - 0.1)
    engine.set_music_volume(volume)
    world.set_text(vol_label, string.format("Music volume: %.0f%%", volume * 100))
  end

  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)
