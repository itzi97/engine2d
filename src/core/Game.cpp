#include "core/Game.hpp"

#include "ecs/World.hpp"
#include "input/InputManager.hpp"
#include "scripting/ScriptingEngine.hpp"

#include <SDL3/SDL.h>

// EmbedScript.cmake generates:
//   build/generated/embedded_<GAME>.hpp
// containing:
//   namespace embedded { inline constexpr char <GAME>[] = "...lua source..."; }
//
// GAME_SCRIPT_NAME is set by CMake as a compile definition (e.g. "snake").
// We use the X-macro trick to build the include path and symbol at compile time.
#define EMBED_HEADER2(name) #name
#define EMBED_HEADER(name)  EMBED_HEADER2(embedded_ ## name ## .hpp)
#include EMBED_HEADER(GAME_SCRIPT_NAME)

#define EMBED_SOURCE2(name) embedded::name
#define EMBED_SOURCE(name)  EMBED_SOURCE2(name)

Game::Game() = default;
Game::~Game() = default;

bool Game::Init() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }

  m_window.reset(SDL_CreateWindow("2D Engine", 1280, 720, 0));
  if (!m_window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return false;
  }

  m_renderer.reset(SDL_CreateRenderer(m_window.get(), nullptr));
  if (!m_renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    return false;
  }

  m_world     = std::make_unique<World>();
  m_input     = std::make_unique<InputManager>();
  m_scripting = std::make_unique<ScriptingEngine>();

  m_scripting->BindWorld(m_world.get());
  m_scripting->BindInput(m_input.get());

  if (!m_scripting->RunString(EMBED_SOURCE(GAME_SCRIPT_NAME), GAME_SCRIPT_NAME)) {
    SDL_Log("Failed to load game script: %s", GAME_SCRIPT_NAME);
    return false;
  }

  return true;
}

void Game::Run() {
  using namespace std::chrono;
  auto previous = steady_clock::now();
  constexpr double kMaxDt = 1.0 / 20.0;

  while (m_running) {
    const auto   now = steady_clock::now();
    const double dt  = std::min(duration<double>(now - previous).count(), kMaxDt);
    previous         = now;

    // 1. Input
    m_input->BeginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) m_running = false;
      m_input->ProcessEvent(event);
    }

    // 2. Physics (integrates velocity → position)
    m_world->Update(static_cast<float>(dt));

    // 3. Lua on_update (may call set_position, destroy entities, etc.)
    m_scripting->CallOnUpdate(static_cast<float>(dt));

    // 4. Collision detection on fully-committed positions
    m_world->RunCollision();

    // 5. Render
    SDL_SetRenderDrawColor(m_renderer.get(), 15, 15, 15, 255);
    SDL_RenderClear(m_renderer.get());
    m_world->Render(m_renderer.get());
    SDL_RenderPresent(m_renderer.get());
  }
}

void Game::Shutdown() {
  m_scripting.reset();
  m_world.reset();
  m_input.reset();
  m_renderer.reset();
  m_window.reset();
  SDL_Quit();
}
