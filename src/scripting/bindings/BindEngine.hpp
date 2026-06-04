#pragma once
#include <sol/sol.hpp>
#include <functional>

class InputManager;
class SceneManager;
struct SDL_Window;

void BindEngine(sol::state            &lua,
                InputManager          *input,
                SDL_Window            *window,
                sol::function         &onUpdateOut,
                std::function<void()> &pendingSceneOut,
                SceneManager          *scenes = nullptr);
