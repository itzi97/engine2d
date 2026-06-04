-- scripts/ski.lua
-- CMake-embedded entrypoint for GAME=ski.
-- Immediately delegates to the full scene via SceneManager so that
-- hot-reload, scene transitions, and the scenes/ directory layout
-- all work correctly.
scene.load("ski")
