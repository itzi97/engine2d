-- smoke.lua
-- Headless regression test: initialises the engine and quits after one frame.
-- Run under Xvfb in CI; no window interaction needed.

engine.on_update(function(_dt)
  engine.quit()
end)

log("smoke: loaded, will quit on first update")
