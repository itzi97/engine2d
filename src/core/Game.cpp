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

void Game::RegisterScenes() {
  m_scenes->Register("menu",   "scripts/scenes/menu.lua");
  m_scenes->Register("ski",    "scripts/scenes/ski.lua");
  m_scenes->Register("finish", "scripts/scenes/finish.lua");
  m_scenes->Register("death",  "scripts/scenes/death.lua");
}

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

  // Request adaptive vsync (SDL3: 1=vsync, -1=adaptive, 0=immediate)
  // Adaptive falls back to regular vsync if the driver doesn't support it.
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

  m_renderer = SDL_CreateRenderer(m_window, nullptr);
  if (!m_renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    return false;
  }

  // Try to lock to display refresh rate via SDL3's vsync API.
  // -1 = adaptive vsync (tears on missed frame instead of stalling),
  //  1 = regular vsync. Ignore failure — fallback sleep handles it.
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

  RegisterScenes();

  m_scripting->BindWorld(m_world.get(), m_textures.get());
  m_scripting->BindInput(m_input.get(), m_window, m_renderer,
                         m_scenes.get(), m_world.get());
  m_scripting->BindTextures(m_textures.get());
  m_scripting->BindAudio(m_audio.get());

#ifdef HOTRELOAD_ENABLED
  m_hotReload->Watch("scripts/scenes/ski.lua",    [this]{ m_scenes->Load("ski"); });
  m_hotReload->Watch("scripts/scenes/menu.lua",   [this]{ m_scenes->Load("menu"); });
  m_hotReload->Watch("scripts/scenes/finish.lua", [this]{ m_scenes->Load("finish"); });
  m_hotReload->Watch("scripts/scenes/death.lua",  [this]{ m_scenes->Load("death"); });
#endif

  if (!m_scripting->RunString(game_script::source, game_script::name)) {
    SDL_Log("Failed to load game script: %s", game_script::name);
    return false;
  }

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
  TextSystem::Render(*m_world, m_renderer, *m_fonts, FONT_PATH);
  SDL_RenderPresent(m_renderer);
}

void Game::Run() {
  using namespace std::chrono;
  using dur = duration<double>;

  // Fallback frame cap: used only when vsync is unavailable.
  // Keeps the loop from busy-spinning at thousands of FPS.
  constexpr double kTargetDt   = 1.0 / 60.0;
  constexpr float  kMaxDt      = 0.05f;  // clamp spike frames to 50ms

  auto previous = steady_clock::now();
  bool running  = true;

  while (running) {
    m_input->EndFrame();
    ProcessEvents(running);

    const auto  now     = steady_clock::now();
    const float dt      = std::min(
        static_cast<float>(dur(now - previous).count()),
        kMaxDt);
    previous = now;

    Update(dt);
    Render();  // SDL_RenderPresent blocks here when vsync is active

    // Fallback sleep: if the whole frame took less than the target,
    // sleep the remainder so we don't spin the CPU to 100%.
    // When vsync is working this sleep is effectively 0.
    const auto frameEnd  = steady_clock::now();
    const double elapsed = dur(frameEnd - now).count();
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
