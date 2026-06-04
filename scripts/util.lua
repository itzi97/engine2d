-- util.lua  — shared helpers, loaded at the top of every game script via:
--   dofile("scripts/util.lua")
--
-- Provides:
--   make_label(x, y, text, size, r, g, b)  -> entity
--   make_sprite(x, y, w, h, r, g, b)       -> entity
--   make_pause_overlay(cx, top_y)           -> { show(sel), hide() }

-- Create a text entity in one call.
function make_label(x, y, text, size, r, g, b)
  local e = world.create_entity()
  world.add_transform(e, x, y, 0, 0)
  world.add_text(e, text, size, r, g, b)
  return e
end

-- Create a coloured rect entity in one call.
function make_sprite(x, y, w, h, r, g, b, a, layer)
  local e = world.create_entity()
  world.add_transform(e, x, y, w, h)
  world.add_sprite(e, r, g, b, a or 255, layer or 0)
  return e
end

-- Shared pause overlay used by both breakout and snake.
-- cx    : horizontal centre of the screen
-- top_y : y position of the top of the panel
--
-- Returns a controller: { show(sel), hide() }
-- sel = 1 -> "Resume" highlighted, sel = 2 -> "Main Menu" highlighted.
function make_pause_overlay(cx, top_y)
  local BG_X = cx - 160
  local BG_Y = top_y

  local bg    = make_sprite(BG_X, BG_Y, 320, 160, 20, 20, 20, 200, 20)
  local title = make_label(cx - 68, top_y + 12,  "PAUSED",                    32, 255, 220,  80)
  local opt1  = make_label(cx - 60, top_y + 60,  "Resume",                    26, 180, 180, 180)
  local opt2  = make_label(cx - 75, top_y + 98,  "Main Menu",                 26, 180, 180, 180)
  local hint  = make_label(cx - 128, top_y + 136, "UP/DOWN  ENTER  or  R / M", 18, 100, 100, 100)

  local SEL   = {255, 220, 80}
  local UNSEL = {180, 180, 180}

  local function refresh(sel)
    if sel == 1 then
      world.set_text_color(opt1, SEL[1],   SEL[2],   SEL[3],   255)
      world.set_text_color(opt2, UNSEL[1], UNSEL[2], UNSEL[3], 255)
    else
      world.set_text_color(opt1, UNSEL[1], UNSEL[2], UNSEL[3], 255)
      world.set_text_color(opt2, SEL[1],   SEL[2],   SEL[3],   255)
    end
  end

  return {
    show = function(sel)
      world.set_position(bg,    BG_X, BG_Y)
      world.set_text(title, "PAUSED")
      world.set_text(opt1,  "Resume")
      world.set_text(opt2,  "Main Menu")
      world.set_text(hint,  "UP/DOWN  ENTER  or  R / M")
      refresh(sel)
    end,
    hide = function()
      world.set_position(bg, -9999, -9999)
      world.set_text(title, "")
      world.set_text(opt1,  "")
      world.set_text(opt2,  "")
      world.set_text(hint,  "")
    end,
  }
end
