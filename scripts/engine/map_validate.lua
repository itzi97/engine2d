-- engine/map_validate.lua
-- Thin Lua wrapper around world.validate_map().
--
-- Usage (in on_init, right after world.load_tiled_map):
--
--   local mv = require("engine.map_validate")
--   mv.check({ strict = false, abort = true })
--
-- opts:
--   strict (bool, default false) -- treat warnings as errors
--   abort  (bool, default true)  -- call error() when ok==false
--
-- Returns the raw { ok, warnings, errors } table from C++.

local M = {}

function M.check(opts)
    opts = opts or {}
    local strict = opts.strict or false
    local abort  = (opts.abort  == nil) and true or opts.abort

    local result = world.validate_map()

    -- In strict mode, any warning is promoted to an error
    if strict then
        for _, w in ipairs(result.warnings) do
            result.errors[#result.errors + 1] = "(promoted) " .. w
        end
        result.warnings = {}
        result.ok = result.ok and (#result.errors == 0)
    end

    -- Pretty-print to stdout (SDL_Log already echoed raw messages from C++,
    -- but this gives a clear pass/fail summary in the Lua layer too)
    if #result.warnings > 0 then
        print("[map_validate] " .. #result.warnings .. " warning(s):")
        for _, w in ipairs(result.warnings) do
            print("  WARN  " .. w)
        end
    end

    if #result.errors > 0 then
        print("[map_validate] " .. #result.errors .. " error(s):")
        for _, e in ipairs(result.errors) do
            print("  ERROR " .. e)
        end
    end

    if result.ok then
        print("[map_validate] OK")
    else
        print("[map_validate] FAILED")
        if abort then
            error("[map_validate] map validation failed — see errors above", 2)
        end
    end

    return result
end

return M
