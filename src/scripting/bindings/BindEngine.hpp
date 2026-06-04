#pragma once
#include <functional>

// Forward-declare sol::state and SDL_Window without pulling in their headers.
namespace sol { class state; }
struct SDL_Window;
class InputManager;

// Populates the global "engine" Lua table with:
//   on_update, is_key_*, is_mouse_*, mouse_position,
//   load_scene, quit, set_window_size, set_window_title
//
// onUpdateOut   is written when Lua calls engine.on_update(fn).
// pendingSceneOut is written when Lua calls engine.load_scene(fn).
void BindEngine(sol::state            &lua,
                InputManager          *input,
                SDL_Window            *window,
                sol::function         &onUpdateOut,
                std::function<void()> &pendingSceneOut);
