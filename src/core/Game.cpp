#include "core/Game.hpp"

#include <SDL3/SDL.h>
#include <iostream>

Game::Game()  = default;
Game::~Game() = default;

bool Game::Initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "[Game] SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }

    m_window = SDL_CreateWindow(
        kTitle.data(),
        kWidth, kHeight,
        SDL_WINDOW_RESIZABLE
    );
    if (!m_window) {
        std::cerr << "[Game] SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        std::cerr << "[Game] SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }

    m_world     = std::make_unique<World>();
    m_scripting = std::make_unique<ScriptingEngine>();

    std::cout << "[Game] Initialized OK (" << kWidth << 'x' << kHeight << ")\n";
    return true;
}

void Game::Run()
{
    Uint64 previous    = SDL_GetTicks();
    float  accumulator = 0.0f;
    bool   running     = true;

    while (running) {
        const Uint64 current = SDL_GetTicks();
        const float  elapsed = static_cast<float>(current - previous) / 1000.0f;
        previous             = current;

        accumulator += (elapsed < 0.25f ? elapsed : 0.25f);

        ProcessEvents(running);

        while (accumulator >= kFixedDt) {
            Update(kFixedDt);
            accumulator -= kFixedDt;
        }

        Render();
    }
}

void Game::Shutdown()
{
    m_scripting.reset();
    m_world.reset();

    if (m_renderer) { SDL_DestroyRenderer(m_renderer); m_renderer = nullptr; }
    if (m_window)   { SDL_DestroyWindow(m_window);     m_window   = nullptr; }
    SDL_Quit();
    std::cout << "[Game] Shutdown complete.\n";
}

void Game::ProcessEvents(bool& running)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)                        running = false;
        if (event.type == SDL_EVENT_KEY_DOWN &&
            event.key.key == SDLK_ESCAPE)                       running = false;
    }
}

void Game::Update(float deltaTime) { m_world->Update(deltaTime); }

void Game::Render()
{
    SDL_SetRenderDrawColor(m_renderer, 18, 18, 18, 255);
    SDL_RenderClear(m_renderer);
    m_world->Render();
    SDL_RenderPresent(m_renderer);
}
