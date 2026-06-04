#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <string_view>

class AudioManager;
class FontManager;
class HotReload;
class InputManager;
class SceneManager;
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
  void RegisterScenes();

  static constexpr std::string_view kTitle  = "engine2d";
  // 31 tiles * 16 px  = 496
  // 18 tiles * 16 px  = 288
  static constexpr int              kWidth  = 496;
  static constexpr int              kHeight = 288;

  SDL_Window   *m_window{nullptr};
  SDL_Renderer *m_renderer{nullptr};

  std::unique_ptr<World>           m_world;
  std::unique_ptr<InputManager>    m_input;
  std::unique_ptr<FontManager>     m_fonts;
  std::unique_ptr<TextureManager>  m_textures;
  std::unique_ptr<AudioManager>    m_audio;
  std::unique_ptr<ScriptingEngine> m_scripting;
  std::unique_ptr<SceneManager>    m_scenes;
  std::unique_ptr<HotReload>       m_hotReload;
};
