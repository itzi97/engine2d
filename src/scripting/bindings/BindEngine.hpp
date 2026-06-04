#pragma once
#include <functional>

// Forward-declare sol::state without pulling in sol/sol.hpp.
namespace sol { class state; }
class InputManager;

// Populates the global "engine" Lua table with:
//   on_update, is_key_*, is_mouse_*, mouse_position, load_scene, quit
//
// onUpdateOut is written when Lua calls engine.on_update(fn).
void BindEngine(sol::state &lua,
                InputManager *input,
                sol::function &onUpdateOut,
                std::function<void()> &pendingSceneOut);
