-- pause_menu.lua
-- Shared pause overlay. Call: pause_menu(resume_fn)
-- ESCAPE / ENTER on RESUME  → resume_fn()
-- ENTER on QUIT             → engine.quit()

function pause_menu(resume_fn)
  local W, H    = 1280, 720
  local options = { "RESUME", "QUIT" }
  local selected = 1

  -- Dim overlay (semi-transparent black rectangle)
  local overlay = world.create_entity()
  world.add_transform(overlay, 0, 0, W, H)
  world.add_sprite(overlay, 0, 0, 0, 180, 20)  -- layer 20 → above game

  -- Title
  local title_e = world.create_entity()
  world.add_transform(title_e, 0, H/2 - 100, 0, 0)
  world.add_text(title_e, "PAUSED", 56, 255, 255, 255, 21)
  -- centre horizontally via x offset approximation
  world.set_position(title_e, W/2 - 110, H/2 - 100)

  -- Option entities
  local option_entities = {}
  for i, label in ipairs(options) do
    local e = world.create_entity()
    world.add_transform(e, W/2 - 80, H/2 - 10 + (i-1) * 60, 0, 0)
    world.add_text(e, label, 36, 180, 180, 180, 21)
    table.insert(option_entities, e)
  end

  local function refresh_colours()
    for i, e in ipairs(option_entities) do
      if i == selected then
        world.set_text_color(e, 255, 220, 80, 255)   -- highlighted
      else
        world.set_text_color(e, 160, 160, 160, 255)  -- dimmed
      end
    end
  end

  refresh_colours()

  engine.on_update(function(dt)
    if engine.is_key_just_pressed("DOWN") then
      selected = selected % #options + 1
      refresh_colours()
    end
    if engine.is_key_just_pressed("UP") then
      selected = (selected - 2) % #options + 1
      refresh_colours()
    end

    if engine.is_key_just_pressed("RETURN") or engine.is_key_just_pressed("RETURN2") then
      if selected == 1 then
        engine.load_scene(resume_fn)
      else
        engine.quit()
      end
    end

    -- ESCAPE = quick resume
    if engine.is_key_just_pressed("ESCAPE") then
      engine.load_scene(resume_fn)
    end
  end)

  log("pause_menu: active")
end
