#pragma once
#include <sol/sol.hpp>
#include <functional>

class InputManager;
class SceneManager;
class World;
struct SDL_Window;
struct SDL_Renderer;

void BindEngine(sol::state            &lua,
                InputManager          *input,
                SDL_Window            *window,
                SDL_Renderer          *renderer,
                sol::function         &onUpdateOut,
                std::function<void()> &pendingSceneOut,
                SceneManager          *scenes,
                World                 *world);
