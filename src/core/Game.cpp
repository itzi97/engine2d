#include "core/Game.hpp"

#include "ecs/World.hpp"
#include "input/InputManager.hpp"
#include "scripting/ScriptingEngine.hpp"

#include <SDL3/SDL.h>

// game_script_shim.hpp is generated at CMake configure time.
// It includes the correct embedded_<GAME>.hpp and exposes:
//   game_script::source  (const char*)
//   game_script::name    (const char*)
#include "game_script_shim.hpp"

Game::Game()  = default;
Game::~Game() = default;

bool Game::Initialize() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }

  m_window = SDL_CreateWindow(kTitle.data(), kWidth, kHeight, 0);
  if (!m_window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return false;
  }

  m_renderer = SDL_CreateRenderer(m_window, nullptr);
  if (!m_renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    return false;
  }

  m_world     = std::make_unique<World>();
  m_input     = std::make_unique<InputManager>();
  m_scripting = std::make_unique<ScriptingEngine>();

  m_scripting->BindWorld(m_world.get());
  m_scripting->BindInput(m_input.get());

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
}

void Game::Render() {
  SDL_SetRenderDrawColor(m_renderer, 15, 15, 15, 255);
  SDL_RenderClear(m_renderer);
  m_world->Render(m_renderer);
  SDL_RenderPresent(m_renderer);
}

void Game::Run() {
  using namespace std::chrono;
  auto  previous    = steady_clock::now();
  bool  running     = true;
  float accumulator = 0.f;

  while (running) {
    const auto  now = steady_clock::now();
    const float raw = static_cast<float>(
        duration<double>(now - previous).count());
    previous = now;

    const float dt = (raw < 0.05f) ? raw : 0.05f;  // clamp: avoid spiral-of-death

    ProcessEvents(running);

    accumulator += dt;
    while (accumulator >= kFixedDt) {
      Update(kFixedDt);
      accumulator -= kFixedDt;
    }

    m_input->EndFrame();
    Render();
  }
}

void Game::Shutdown() {
  m_scripting.reset();
  m_world.reset();
  m_input.reset();
  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
  m_renderer = nullptr;
  m_window   = nullptr;
  SDL_Quit();
}
