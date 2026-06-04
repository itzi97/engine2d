#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindEngine.hpp"
#include "scripting/bindings/KeycodeFromString.hpp"
#include "input/InputManager.hpp"
#include "core/SceneManager.hpp"

#include <SDL3/SDL.h>
#include <iostream>

void BindEngine(sol::state            &lua,
                InputManager          *input,
                SDL_Window            *window,
                sol::function         &onUpdateOut,
                std::function<void()> &pendingSceneOut,
                SceneManager          *scenes) {
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
  eng.set_function("set_window_size", [window](int w, int h) {
    SDL_SetWindowSize(window, w, h);
  });
  eng.set_function("set_window_title", [window](const std::string &title) {
    SDL_SetWindowTitle(window, title.c_str());
  });

  // --- Scene management ---------------------------------------------------
  // engine.scene.load("ski")  -- named scene from SceneManager registry
  // engine.load_scene(fn)     -- legacy: raw Lua callback (kept for compat)
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
