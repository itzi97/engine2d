-- ski.lua  --  minimal tilemap render demo
-- Loads sampleMap.tmj and displays all 4 tile layers.
-- Arrow keys scroll the camera. ESC quits.

local CAM_SPEED = 120   -- pixels/s
local cam_x, cam_y = 0, 0

engine.on_start = function()
    world.load_tiled_map("assets/maps/sampleMap.tmj")
end

engine.on_update = function(dt)
    if engine.is_key_just_pressed("ESCAPE") then
        engine.quit()
        return
    end

    if engine.is_key_held("RIGHT") then cam_x = cam_x + CAM_SPEED * dt end
    if engine.is_key_held("LEFT")  then cam_x = cam_x - CAM_SPEED * dt end
    if engine.is_key_held("DOWN")  then cam_y = cam_y + CAM_SPEED * dt end
    if engine.is_key_held("UP")    then cam_y = cam_y - CAM_SPEED * dt end

    engine.set_camera(cam_x, cam_y)
end
