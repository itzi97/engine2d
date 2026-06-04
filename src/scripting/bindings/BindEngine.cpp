#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindEngine.hpp"
#include "scripting/bindings/KeycodeFromString.hpp"
#include "input/InputManager.hpp"
#include "core/SceneManager.hpp"
#include "ecs/World.hpp"

#include <SDL3/SDL.h>
#include <iostream>

void BindEngine(sol::state            &lua,
                InputManager          *input,
                SDL_Window            *window,
                SDL_Renderer          *renderer,
                sol::function         &onUpdateOut,
                std::function<void()> &pendingSceneOut,
                SceneManager          *scenes,
                World                 *world) {
  auto eng = lua.create_named_table("engine");

  eng.set_function("on_update", [&onUpdateOut](sol::function fn) {
    onUpdateOut = fn;
  });

  // --- Keyboard -----------------------------------------------------------
  eng.set_function("is_key_pressed",
      [input](const std::string &k) { return input->IsKeyPressed(KeycodeFromString(k)); });
  eng.set_function("is_key_just_pressed",
      [input](const std::string &k) { return input->IsKeyJustPressed(KeycodeFromString(k)); });
  eng.set_function("is_key_just_released",
      [input](const std::string &k) { return input->IsKeyJustReleased(KeycodeFromString(k)); });

  // --- Mouse --------------------------------------------------------------
  eng.set_function("is_mouse_pressed",
      [input](int b) { return input->IsMousePressed(b); });
  eng.set_function("is_mouse_just_pressed",
      [input](int b) { return input->IsMouseJustPressed(b); });
  eng.set_function("is_mouse_just_released",
      [input](int b) { return input->IsMouseJustReleased(b); });
  eng.set_function("mouse_position",
      [input]() -> std::tuple<float, float> { return {input->MouseX(), input->MouseY()}; });

  // --- Window -------------------------------------------------------------
  // set_window_size(w, h)
  //   Resizes the OS window AND updates the renderer's logical presentation
  //   so that all draw calls remain in the same [0,w]x[0,h] coordinate space
  //   even if the user later resizes the window manually.
  //   Call this once at scene boot — it is the script's job to own the
  //   game's resolution, not the engine's.
  eng.set_function("set_window_size", [window, renderer](int w, int h) {
    SDL_SetWindowSize(window, w, h);
    SDL_SetRenderLogicalPresentation(
        renderer, w, h,
        SDL_LOGICAL_PRESENTATION_LETTERBOX);
  });
  eng.set_function("set_window_title", [window](const std::string &title) {
    SDL_SetWindowTitle(window, title.c_str());
  });

  // --- Camera -------------------------------------------------------------
  eng.set_function("set_camera", [world](float x, float y) {
    if (world) world->SetCamera(x, y);
  });
  eng.set_function("get_camera", [world]() -> std::tuple<float, float> {
    if (world) return {world->CamX(), world->CamY()};
    return {0.f, 0.f};
  });
  eng.set_function("move_camera", [world](float dx, float dy) {
    if (world) world->SetCamera(world->CamX() + dx, world->CamY() + dy);
  });

  // --- Scene management ---------------------------------------------------
  auto sceneTable = lua.create_named_table("scene");
  if (scenes) {
    sceneTable.set_function("load", [scenes](const std::string &name) {
      scenes->Load(name);
    });
    sceneTable.set_function("reload", [scenes]() {
      const auto &p = scenes->ActivePath();
      if (!p.empty()) scenes->Load(p.stem().string());
    });
  }

  // Legacy raw-callback form — still works for inline scripts.
  eng.set_function("load_scene", [&pendingSceneOut](sol::function fn) {
    pendingSceneOut = [fn]() mutable {
      auto result = fn();
      if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[ScriptingEngine] load_scene error: " << err.what() << '\n';
      }
    };
  });

  // --- Quit ---------------------------------------------------------------
  eng.set_function("quit", []() {
    SDL_Event e;
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
  });
}
