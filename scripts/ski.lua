-- ski.lua  --  minimal tilemap render demo
-- Loads sampleMap.tmj and displays all 4 tile layers.
-- ESC quits.

-- Map loads immediately at script boot (no on_start binding exists)
world.load_tiled_map("assets/maps/sampleMap.tmj")

engine.on_update = function(dt)
    if engine.is_key_just_pressed("ESCAPE") then
        engine.quit()
    end
end
