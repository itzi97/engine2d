#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <string_view>

class AudioManager;
class FontManager;
class InputManager;
class ScriptingEngine;
class TextureManager;
class World;

class Game {
public:
  Game();
  ~Game();

  bool Initialize();
  void Run();
  void Shutdown();

private:
  void ProcessEvents(bool &running);
  void Update(float dt);
  void Render();

  // Defaults — Lua scripts may override via engine.set_window_size / set_window_title.
  static constexpr std::string_view kTitle   = "engine2d";
  static constexpr int              kWidth   = 1280;
  static constexpr int              kHeight  = 720;
  static constexpr float            kFixedDt = 1.f / 60.f;

  SDL_Window   *m_window{nullptr};
  SDL_Renderer *m_renderer{nullptr};

  std::unique_ptr<World>           m_world;
  std::unique_ptr<InputManager>    m_input;
  std::unique_ptr<FontManager>     m_fonts;
  std::unique_ptr<TextureManager>  m_textures;
  std::unique_ptr<AudioManager>    m_audio;
  std::unique_ptr<ScriptingEngine> m_scripting;
};
