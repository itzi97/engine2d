-- main_menu.lua  (embed with -DGAME=main_menu)
-- Shared entry point. UP/DOWN to navigate, ENTER to launch.

engine.set_window_title("engine2d")
engine.set_window_size(1280, 720)

local W, H = 1280, 720

local games = {
  { label = "Snake",   file = "scripts/snake.lua"    },
  { label = "Breakout",file = "scripts/breakout.lua" },
}
local selected = 1

-- Title
local title_e = world.create_entity()
world.add_transform(title_e, W/2 - 100, H/2 - 160, 0, 0)
world.add_text(title_e, "engine2d", 64, 255, 255, 255)

-- Subtitle
local sub_e = world.create_entity()
world.add_transform(sub_e, W/2 - 140, H/2 - 70, 0, 0)
world.add_text(sub_e, "select a game", 24, 160, 160, 160)

-- Game option entities
local option_entities = {}
for i, g in ipairs(games) do
  local e = world.create_entity()
  world.add_transform(e, W/2 - 80, H/2 + (i - 1) * 60, 0, 0)
  world.add_text(e, g.label, 40, 180, 180, 180)
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

-- Hint
local hint_e = world.create_entity()
world.add_transform(hint_e, W/2 - 160, H - 60, 0, 0)
world.add_text(hint_e, "UP / DOWN to navigate   ENTER to play", 18, 100, 100, 100)

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
    engine.load_scene(function()
      dofile(path)
    end)
  end
  if engine.is_key_just_pressed("ESCAPE") then
    engine.quit()
  end
end)

log("main_menu: ready")
