-- engine/scene.lua
-- Thin alias so game scripts can require("engine.scene") instead of
-- calling the engine.scene table directly. Both work identically.
--
-- Usage:
--   local Scene = require("engine.scene")
--   Scene.load("ski")     -- transition to a named scene
--   Scene.reload()        -- reload the current scene (dev hot-reload)

return {
  load   = function(name) scene.load(name) end,
  reload = function()     scene.reload()   end,
}
