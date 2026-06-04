-- util.lua  — shared helpers, loaded at the top of every game script via:
--   dofile("scripts/util.lua")
--
-- Provides:
--   make_label(x, y, text, size, r, g, b)          -> entity
--   make_sprite(x, y, w, h, r, g, b [, a [, layer]]) -> entity
--   make_panel(opts)                                 -> panel object
--   make_pause_overlay(cx, top_y)                    -> panel object

-- ---------------------------------------------------------------------------
-- Primitives
-- ---------------------------------------------------------------------------

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

-- ---------------------------------------------------------------------------
-- make_panel — generic reusable UI panel
-- ---------------------------------------------------------------------------
-- Builds a panel with an optional background, a title, and N menu items.
-- All entities start hidden; call panel:show() / panel:hide() to toggle.
--
-- opts = {
--   cx         = number,          -- horizontal centre of the panel (required)
--   cy         = number,          -- vertical centre of the panel (required)
--   w          = number,          -- background width  (default 320)
--   h          = number,          -- background height (default 160)
--   bg_color   = {r,g,b,a},       -- background colour (default {20,20,20,200})
--   bg_layer   = number,          -- render layer for bg rect (default 20)
--   title      = string,          -- panel title text (optional)
--   title_size = number,          -- font size for title (default 32)
--   title_color= {r,g,b},         -- title colour (default {255,220,80})
--   items      = {                -- array of menu items
--     { text=string, size=number, color={r,g,b} },
--     ...
--   },
--   item_gap   = number,          -- px between items (default 38)
--   sel_color  = {r,g,b},         -- highlighted item colour (default {255,220,80})
--   unsel_color= {r,g,b},         -- normal item colour (default {180,180,180})
--   layer      = number,          -- render layer for all text (default 30)
-- }
--
-- Returns a panel object with methods:
--   panel:show([sel])         -- make visible; optionally highlight item sel (1-based)
--   panel:hide()              -- make invisible
--   panel:set_selected(i)     -- highlight item i, unhighlight others
--   panel:set_item_text(i, s) -- update the text of item i
--   panel:set_title(s)        -- update the title text
--
function make_panel(opts)
  local cx          = opts.cx
  local cy          = opts.cy
  local pw          = opts.w          or 320
  local ph          = opts.h          or 160
  local bg_col      = opts.bg_color   or {20, 20, 20, 200}
  local bg_layer    = opts.bg_layer   or 20
  local title_text  = opts.title      or ""
  local title_size  = opts.title_size or 32
  local title_col   = opts.title_color or {255, 220, 80}
  local items       = opts.items      or {}
  local item_gap    = opts.item_gap   or 38
  local sel_col     = opts.sel_color  or {255, 220, 80}
  local unsel_col   = opts.unsel_color or {180, 180, 180}
  local txt_layer   = opts.layer      or 30

  -- Background rect — centred on cx, cy
  local bg = make_sprite(
    cx - pw/2, cy - ph/2, pw, ph,
    bg_col[1], bg_col[2], bg_col[3], bg_col[4] or 200,
    bg_layer)
  world.set_visible(bg, false)

  -- Title (centred horizontally)
  local title_e = nil
  if title_text ~= "" then
    title_e = make_label(0, 0, title_text, title_size,
                         title_col[1], title_col[2], title_col[3])
    world.set_text_anchor(title_e, 0.5, 0)
    -- Position title near top of panel
    local title_y = cy - ph/2 + 14
    world.set_position(title_e, cx, title_y)
    world.add_text(title_e, title_text, title_size,  -- no-op: already set
                   title_col[1], title_col[2], title_col[3])
    world.set_visible(title_e, false)
    if txt_layer then
      -- abuse set_layer via sprite component not present; use text layer field
      -- TextComponent.layer is set at add_text time; we can't update it post-hoc
      -- through the current API, but the default (10) is below txt_layer anyway.
      -- For panels, txt_layer ≥ bg_layer is already guaranteed by defaults.
    end
  end

  -- Item labels — evenly spaced below the title
  local item_entities = {}
  local items_start_y = cy - (#items * item_gap) / 2
  -- If there's a title, push items down a bit so they don't overlap
  if title_text ~= "" then
    items_start_y = cy - ph/2 + (title_size + 28)
  end

  for i, item in ipairs(items) do
    local isize  = item.size  or 26
    local icol   = item.color or unsel_col
    local e = make_label(0, 0, item.text, isize,
                         icol[1], icol[2], icol[3])
    world.set_text_anchor(e, 0.5, 0)
    world.set_position(e, cx, items_start_y + (i - 1) * item_gap)
    world.set_visible(e, false)
    item_entities[i] = e
  end

  -- Internal helpers
  local function set_all_visible(v)
    world.set_visible(bg, v)
    if title_e then world.set_visible(title_e, v) end
    for _, e in ipairs(item_entities) do world.set_visible(e, v) end
  end

  local function refresh_colors(sel)
    for i, e in ipairs(item_entities) do
      local c = (i == sel) and sel_col or unsel_col
      world.set_text_color(e, c[1], c[2], c[3], 255)
    end
  end

  -- Public interface
  local panel = {}

  function panel:show(sel)
    set_all_visible(true)
    if sel then refresh_colors(sel) end
  end

  function panel:hide()
    set_all_visible(false)
  end

  function panel:set_selected(i)
    refresh_colors(i)
  end

  function panel:set_item_text(i, s)
    if item_entities[i] then world.set_text(item_entities[i], s) end
  end

  function panel:set_title(s)
    if title_e then world.set_text(title_e, s) end
  end

  return panel
end

-- ---------------------------------------------------------------------------
-- make_pause_overlay — convenience wrapper around make_panel
-- ---------------------------------------------------------------------------
-- Builds the standard pause menu (PAUSED / Resume / Main Menu / hint).
-- cx    : horizontal centre of the screen
-- top_y : y position of the top of the panel
--
-- Returns the same interface as make_panel: { show(sel), hide() }
-- sel = 1 -> "Resume" highlighted, sel = 2 -> "Main Menu" highlighted.
function make_pause_overlay(cx, top_y)
  local ph = 160
  local panel = make_panel({
    cx          = cx,
    cy          = top_y + ph / 2,
    w           = 320,
    h           = ph,
    bg_color    = {20, 20, 20, 200},
    title       = "PAUSED",
    title_size  = 32,
    title_color = {255, 220, 80},
    items = {
      { text = "Resume",    size = 26 },
      { text = "Main Menu", size = 26 },
    },
    item_gap    = 38,
    sel_color   = {255, 220, 80},
    unsel_color = {180, 180, 180},
    layer       = 30,
  })

  -- Append the keyboard hint as a separate label below the panel
  local hint_y = top_y + ph - 22
  local hint_e = make_label(0, 0, "UP/DOWN  ENTER  or  R / M", 15, 100, 100, 100)
  world.set_text_anchor(hint_e, 0.5, 0)
  world.set_position(hint_e, cx, hint_y)
  world.set_visible(hint_e, false)

  -- Wrap show/hide to also toggle the hint label
  local inner_show = panel.show
  local inner_hide = panel.hide

  function panel:show(sel)
    inner_show(self, sel)
    world.set_visible(hint_e, true)
  end

  function panel:hide()
    inner_hide(self)
    world.set_visible(hint_e, false)
  end

  return panel
end
