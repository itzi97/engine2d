-- scripts/main.lua
-- Bootstrap script embedded at compile time.
-- This is the ONLY game-specific thing the engine binary knows about.
-- Everything below is game configuration, not engine code.

engine.set_window_title("Tiny Ski")
engine.set_window_size(496, 288)
engine.set_font_path("assets/fonts/DejaVuSans.ttf")

scene.register("ski",    "scripts/scenes/ski.lua")
scene.register("menu",   "scripts/scenes/menu.lua")
scene.register("finish", "scripts/scenes/finish.lua")
scene.register("death",  "scripts/scenes/death.lua")

scene.load("ski")
