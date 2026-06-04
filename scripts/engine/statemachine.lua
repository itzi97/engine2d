-- engine/statemachine.lua
-- Minimal flat FSM for game logic (enemy AI, player states, menus).
--
-- Usage:
--   local SM = require("engine.statemachine")
--   local fsm = SM.new("idle")
--   fsm:add("idle",
--     function() print("enter idle") end,
--     function(dt) -- update end,
--     function() print("exit idle") end)
--   fsm:add("chase", on_enter, on_update, on_exit)
--   fsm:transition("chase")
--   -- in on_update:
--   fsm:update(dt)

local SM = {}
SM.__index = SM

function SM.new(initial)
    return setmetatable({ state = initial, states = {} }, SM)
end

function SM:add(name, on_enter, on_update, on_exit)
    self.states[name] = {
        enter  = on_enter,
        update = on_update,
        exit   = on_exit,
    }
end

function SM:transition(next_state)
    if self.state == next_state then return end
    local cur = self.states[self.state]
    if cur and cur.exit then cur.exit() end
    self.state = next_state
    local nxt = self.states[next_state]
    if nxt and nxt.enter then nxt.enter() end
end

function SM:update(dt)
    local cur = self.states[self.state]
    if cur and cur.update then cur.update(dt) end
end

return SM
