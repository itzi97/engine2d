-- Breakout  –  level1.lua
-- Window assumed to be 800 × 600.
-- Requires: world.*, engine.* bindings as wired in BindWorld / BindEngine.

local WIN_W  = 800
local WIN_H  = 600
local BALL_SPEED   = 300   -- pixels/s (magnitude preserved on bounces)
local PADDLE_SPEED = 420
local PADDLE_W     = 100
local PADDLE_H     = 14
local BALL_SIZE    = 12

-- Entity handles (created once in on_start)
local paddle = nil
local ball   = nil
local bricks = {}   -- array of EntityId
local lives  = 3
local score  = 0
local score_label  = nil
local lives_label  = nil
local state        = "playing"   -- "playing" | "won" | "lost"
local state_label  = nil

-- ─── helpers ──────────────────────────────────────────────────────────────────

local function reset_ball()
    world.set_position(ball, WIN_W / 2 - BALL_SIZE / 2, WIN_H / 2)
    world.set_velocity(ball, BALL_SPEED, BALL_SPEED)
end

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

-- ─── level load ───────────────────────────────────────────────────────────────

local function load_level(path)
    -- Destroy any existing bricks (reload / restart)
    for _, e in ipairs(bricks) do world.destroy_entity(e) end
    bricks = {}

    local objects = world.load_tiled_map(path)
    for _, obj in pairs(objects) do
        if obj.type == "brick" then
            local e = obj.entity
            -- TransformComponent is already set by MapSystem::Spawn
            local r = tonumber(obj.properties.color_r) or 200
            local g = tonumber(obj.properties.color_g) or 80
            local b = tonumber(obj.properties.color_b) or 60
            world.add_sprite(e, r, g, b, 255, 1)
            table.insert(bricks, e)
        end
    end
end

-- ─── on_start ─────────────────────────────────────────────────────────────────

engine.on_start = function()
    -- Paddle
    paddle = world.create_entity()
    world.add_transform(paddle,
        WIN_W / 2 - PADDLE_W / 2,
        WIN_H - 40,
        PADDLE_W, PADDLE_H)
    world.add_sprite(paddle, 80, 180, 220, 255, 1)
    world.add_kinematic(paddle)

    -- Ball
    ball = world.create_entity()
    world.add_transform(ball,
        WIN_W / 2 - BALL_SIZE / 2,
        WIN_H / 2,
        BALL_SIZE, BALL_SIZE)
    world.add_sprite(ball, 240, 240, 240, 255, 1)
    world.add_kinematic(ball)
    world.set_velocity(ball, BALL_SPEED, BALL_SPEED)

    -- HUD labels
    score_label = world.create_entity()
    world.add_transform(score_label, 10, 8, 0, 0)
    world.add_text(score_label, "Score: 0", 16, 220, 220, 220, 10)

    lives_label = world.create_entity()
    world.add_transform(lives_label, WIN_W - 90, 8, 0, 0)
    world.add_text(lives_label, "Lives: 3", 16, 220, 220, 220, 10)

    state_label = world.create_entity()
    world.add_transform(state_label, WIN_W / 2 - 80, WIN_H / 2 - 12, 0, 0)
    world.add_text(state_label, "", 22, 255, 230, 80, 10)

    load_level("assets/maps/level1.tmj")
end

-- ─── on_update ────────────────────────────────────────────────────────────────

engine.on_update = function(dt)
    if engine.is_key_just_pressed("ESCAPE") then
        engine.quit()
        return
    end

    -- Restart after win / loss
    if state ~= "playing" then
        if engine.is_key_just_pressed("SPACE") then
            state = "playing"
            lives = 3
            score = 0
            world.set_text(score_label, "Score: 0")
            world.set_text(lives_label, "Lives: 3")
            world.set_text(state_label, "")
            reset_ball()
            load_level("assets/maps/level1.tmj")
        end
        return
    end

    -- ── Paddle movement ──────────────────────────────────────────────────────
    local px, py = world.get_position(paddle)
    if engine.is_key_held("LEFT") then
        world.set_velocity(paddle, -PADDLE_SPEED, 0)
    elseif engine.is_key_held("RIGHT") then
        world.set_velocity(paddle, PADDLE_SPEED, 0)
    else
        world.set_velocity(paddle, 0, 0)
    end
    -- Clamp paddle inside window after physics step
    px = clamp(px, 0, WIN_W - PADDLE_W)
    world.set_position(paddle, px, py)

    -- ── Ball / paddle collision ───────────────────────────────────────────────
    for _, col in ipairs(world.get_collisions_for(ball)) do
        local other = (col.a == ball) and col.b or col.a
        if other == paddle then
            local bx, _  = world.get_position(ball)
            local pcx    = px + PADDLE_W / 2
            local offset = (bx + BALL_SIZE / 2 - pcx) / (PADDLE_W / 2)  -- -1 .. 1
            offset = clamp(offset, -0.95, 0.95)
            local vx = BALL_SPEED * offset
            local vy = -math.sqrt(math.max(0, BALL_SPEED * BALL_SPEED - vx * vx))
            world.set_velocity(ball, vx, vy)
        end
    end

    -- ── Ball / brick collisions ───────────────────────────────────────────────
    local brick_hit = false
    for i = #bricks, 1, -1 do
        local e = bricks[i]
        for _, col in ipairs(world.get_collisions_for(ball)) do
            local other = (col.a == ball) and col.b or col.a
            if other == e then
                world.destroy_entity(e)
                table.remove(bricks, i)
                if not brick_hit then
                    local vx, vy = world.get_velocity(ball)
                    world.set_velocity(ball, vx, -vy)
                    brick_hit = true
                end
                score = score + 10
                world.set_text(score_label, "Score: " .. score)
                break
            end
        end
    end

    -- ── Ball / wall bounces ───────────────────────────────────────────────────
    local bx, by = world.get_position(ball)
    local vx, vy = world.get_velocity(ball)
    if bx <= 0 then
        world.set_position(ball, 0, by)
        world.set_velocity(ball, math.abs(vx), vy)
    elseif bx + BALL_SIZE >= WIN_W then
        world.set_position(ball, WIN_W - BALL_SIZE, by)
        world.set_velocity(ball, -math.abs(vx), vy)
    end
    if by <= 0 then
        world.set_position(ball, bx, 0)
        world.set_velocity(ball, vx, math.abs(vy))
    end

    -- ── Ball out of bounds (lost a life) ─────────────────────────────────────
    if by + BALL_SIZE > WIN_H then
        lives = lives - 1
        world.set_text(lives_label, "Lives: " .. lives)
        if lives <= 0 then
            state = "lost"
            world.set_velocity(ball, 0, 0)
            world.set_text(state_label, "GAME OVER  –  SPACE to retry")
        else
            reset_ball()
        end
    end

    -- ── Win condition ─────────────────────────────────────────────────────────
    if #bricks == 0 and state == "playing" then
        state = "won"
        world.set_velocity(ball, 0, 0)
        world.set_text(state_label, "YOU WIN!  –  SPACE to play again")
    end
end
