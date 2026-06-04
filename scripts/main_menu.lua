-- main_menu.lua  (loaded at startup)
-- UP/DOWN to navigate, ENTER to launch.

dofile("scripts/util.lua")

engine.set_window_title("engine2d")
engine.set_window_size(1280, 720)

local W, H = 1280, 720

local games = {
  { label = "Snake",   file = "scripts/snake.lua"    },
  { label = "Breakout", file = "scripts/breakout.lua" },
}
local selected = 1

make_label(W/2 - 100, H/2 - 160, "engine2d",      64, 255, 255, 255)
make_label(W/2 - 140, H/2 -  70, "select a game", 24, 160, 160, 160)
make_label(W/2 - 160, H   -  60, "UP / DOWN to navigate   ENTER to play", 18, 100, 100, 100)

local option_entities = {}
for i, g in ipairs(games) do
  local e = make_label(W/2 - 80, H/2 + (i - 1) * 60, g.label, 40, 180, 180, 180)
  table.insert(option_entities, e)
end

local function refresh()
  for i, e in ipairs(option_entities) do
    if i == selected then
      world.set_text_color(e, 255, 220, 80, 255)
    else
      world.set_text_color(e, 140, 140, 140, 255)
    end
  end
end

refresh()

engine.on_update(function(dt)
  if engine.is_key_just_pressed("DOWN") then
    selected = selected % #games + 1
    refresh()
  end
  if engine.is_key_just_pressed("UP") then
    selected = (selected - 2) % #games + 1
    refresh()
  end
  if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
    local path = games[selected].file
    engine.load_scene(function() dofile(path) end)
  end
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)

log("main_menu: ready")
