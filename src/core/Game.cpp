#include "core/Game.hpp"

#include "audio/AudioManager.hpp"
#include "core/HotReload.hpp"
#include "core/SceneManager.hpp"
#include "ecs/World.hpp"
#include "ecs/systems/TextSystem.hpp"
#include "input/InputManager.hpp"
#include "rendering/FontManager.hpp"
#include "rendering/TextureManager.hpp"
#include "scripting/ScriptingEngine.hpp"

#include <SDL3/SDL.h>
#include <chrono>
#include <thread>

#include "game_script_shim.hpp"

Game::Game()  = default;
Game::~Game() = default;

bool Game::Initialize() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }

  m_window = SDL_CreateWindow(kTitle.data(), kWidth, kHeight, SDL_WINDOW_RESIZABLE);
  if (!m_window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return false;
  }

  SDL_RaiseWindow(m_window);

  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

  m_renderer = SDL_CreateRenderer(m_window, nullptr);
  if (!m_renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    return false;
  }

  if (!SDL_SetRenderVSync(m_renderer, -1))
    SDL_SetRenderVSync(m_renderer, 1);

  m_world     = std::make_unique<World>();
  m_input     = std::make_unique<InputManager>();
  m_fonts     = std::make_unique<FontManager>();
  m_textures  = std::make_unique<TextureManager>(m_renderer);
  m_audio     = std::make_unique<AudioManager>();
  m_scripting = std::make_unique<ScriptingEngine>();
  m_scenes    = std::make_unique<SceneManager>(*m_scripting);
  m_hotReload = std::make_unique<HotReload>(1.0f);

  m_scripting->BindWorld(m_world.get(), m_textures.get());
  m_scripting->BindInput(m_input.get(), m_window, m_renderer,
                         m_scenes.get(), m_world.get());
  m_scripting->BindTextures(m_textures.get());
  m_scripting->BindAudio(m_audio.get());

  // Run the bootstrap script. It calls engine.set_font_path(),
  // scene.register(), scene.load() — no game knowledge lives in C++.
  if (!m_scripting->RunString(game_script::source, game_script::name)) {
    SDL_Log("Failed to load bootstrap script: %s", game_script::name);
    return false;
  }

  // Hot-reload: watch every path the SceneManager knows about.
#ifdef HOTRELOAD_ENABLED
  for (const auto &[name, path] : m_scenes->AllScenes()) {
    m_hotReload->Watch(path, [this, n = name] { m_scenes->Load(n); });
  }
#endif

  return true;
}

void Game::ProcessEvents(bool &running) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) running = false;
    m_input->ProcessEvent(event);
  }
}

void Game::Update(float dt) {
  m_world->Update(dt);
  m_scripting->CallOnUpdate(dt);
  m_world->RunCollision();
  m_world->FlushDestroyQueue();

#ifdef HOTRELOAD_ENABLED
  m_hotReload->Poll();
#endif

  if (auto scene = m_scripting->TakePendingScene()) {
    m_world->ClearAll();
    m_scripting->ResetOnUpdate();
    (*scene)();
  }
}

void Game::Render() {
  SDL_SetRenderDrawColor(m_renderer, 15, 15, 15, 255);
  SDL_RenderClear(m_renderer);
  m_world->Render(m_renderer);
  TextSystem::Render(*m_world, m_renderer, *m_fonts, m_world->GetFontPath());
  SDL_RenderPresent(m_renderer);
}

void Game::Run() {
  using namespace std::chrono;
  using dur = duration<double>;

  constexpr double kTargetDt = 1.0 / 60.0;
  constexpr float  kMaxDt    = 0.05f;

  auto previous = steady_clock::now();
  bool running  = true;

  while (running) {
    m_input->EndFrame();
    ProcessEvents(running);

    const auto  now = steady_clock::now();
    const float dt  = std::min(
        static_cast<float>(dur(now - previous).count()),
        kMaxDt);
    previous = now;

    Update(dt);
    Render();

    const auto   frameEnd = steady_clock::now();
    const double elapsed  = dur(frameEnd - now).count();
    if (elapsed < kTargetDt)
      std::this_thread::sleep_for(dur(kTargetDt - elapsed));
  }
}

void Game::Shutdown() {
  m_scripting.reset();
  m_hotReload.reset();
  m_scenes.reset();
  m_world.reset();
  m_textures.reset();
  m_audio.reset();
  m_fonts.reset();
  m_input.reset();
  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
  m_renderer = nullptr;
  m_window   = nullptr;
  SDL_Quit();
}
