#include "core/Game.hpp"

#include "audio/AudioManager.hpp"
#include "ecs/World.hpp"
#include "ecs/systems/TextSystem.hpp"
#include "input/InputManager.hpp"
#include "rendering/FontManager.hpp"
#include "rendering/TextureManager.hpp"
#include "scripting/ScriptingEngine.hpp"

#include <SDL3/SDL.h>

#include "game_script_shim.hpp"

Game::Game()  = default;
Game::~Game() = default;

bool Game::Initialize() {
  // Tell SDL to bypass WM focus-stealing prevention so the window
  // gets keyboard focus immediately without a manual click.
  SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }

  m_window = SDL_CreateWindow(kTitle.data(), kWidth, kHeight, 0);
  if (!m_window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return false;
  }

  SDL_RaiseWindow(m_window);

  m_renderer = SDL_CreateRenderer(m_window, nullptr);
  if (!m_renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    return false;
  }

  m_world     = std::make_unique<World>();
  m_input     = std::make_unique<InputManager>();
  m_fonts     = std::make_unique<FontManager>();
  m_textures  = std::make_unique<TextureManager>(m_renderer);
  m_audio     = std::make_unique<AudioManager>();
  m_scripting = std::make_unique<ScriptingEngine>();

  m_scripting->BindWorld(m_world.get());
  m_scripting->BindInput(m_input.get());
  m_scripting->BindFonts(m_fonts.get());
  m_scripting->BindTextures(m_textures.get());
  m_scripting->BindAudio(m_audio.get());

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

  if (auto scene = m_scripting->TakePendingScene()) {
    m_world->ClearAll();
    m_scripting->ResetOnUpdate();
    scene();
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
  auto  previous    = steady_clock::now();
  bool  running     = true;
  float accumulator = 0.f;

  while (running) {
    // 1. Clear one-frame transition flags from the previous logical tick
    //    BEFORE polling so fresh events fill m_justPressed cleanly.
    m_input->EndFrame();

    const auto  now = steady_clock::now();
    const float raw = static_cast<float>(
        duration<double>(now - previous).count());
    previous = now;

    const float dt = (raw < 0.05f) ? raw : 0.05f;

    // 2. Pump all pending SDL events into InputManager.
    ProcessEvents(running);

    // 3. Run as many fixed-step ticks as the accumulator allows.
    //    m_justPressed survives here because EndFrame() won't fire
    //    again until the top of the next real-frame iteration.
    accumulator += dt;
    while (accumulator >= kFixedDt) {
      Update(kFixedDt);
      accumulator -= kFixedDt;
    }

    Render();
  }
}

void Game::Shutdown() {
  m_scripting.reset();
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
