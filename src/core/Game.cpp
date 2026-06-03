#include "core/Game.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <random>

// Include Components
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

Game::Game() = default;
Game::~Game() = default;

bool Game::Initialize() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cerr << "[Game] SDL_Init failed: " << SDL_GetError() << '\n';
    return false;
  }

  m_window =
      SDL_CreateWindow(kTitle.data(), kWidth, kHeight, SDL_WINDOW_RESIZABLE);
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

  m_world = std::make_unique<World>();
  m_scripting = std::make_unique<ScriptingEngine>();

  std::cout << "[Game] Initialized OK (" << kWidth << 'x' << kHeight << ")\n";

  // Create snake head
  m_snakeHead = m_world->CreateEntity();
  auto &t = m_world->AddComponent<TransformComponent>(m_snakeHead);
  t.position = {10.0f * cellSize, 10.0f * cellSize};
  t.size = {cellSize, cellSize};
  t.velocity = {1.0f, 0.0f};
  auto &headTag = m_world->AddComponent<TagComponent>(m_snakeHead);
  headTag.tag = "snake_head";
  m_world->AddComponent<SpriteComponent>(m_snakeHead, &t,
                                         SDL_Color{0, 255, 0, 255});

  // Spawn food
  SpawnFood();

  return true;
}

void Game::Run() {
  Uint64 previous = SDL_GetTicks();
  float accumulator = 0.0f;
  bool running = true;

  while (running) {
    const Uint64 current = SDL_GetTicks();
    const float elapsed = static_cast<float>(current - previous) / 1000.0f;
    previous = current;

    accumulator += (elapsed < 0.25f ? elapsed : 0.25f);

    ProcessEvents(running);

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

  if (m_renderer) {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }
  if (m_window) {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }
  SDL_Quit();
  std::cout << "[Game] Shutdown complete.\n";
}

void Game::ProcessEvents(bool &running) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT)
      running = false;
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
      running = false;
    if (event.type == SDL_EVENT_KEY_DOWN) {
      if (auto *t = m_world->GetComponent<TransformComponent>(m_snakeHead)) {
        glm::vec2 newDir = t->velocity;

        switch (event.key.key) {
        case SDLK_UP:
          newDir = {0.0f, -1.0f};
          break;
        case SDLK_DOWN:
          newDir = {0.0f, +1.0f};
          break;
        case SDLK_LEFT:
          newDir = {-1.0f, 0.0f};
          break;
        case SDLK_RIGHT:
          newDir = {+1.0f, 0.0f};
          break;
        default:
          break;
        }

        // Reject exact opposite direction
        if (!(newDir.x == -t->velocity.x && newDir.y == -t->velocity.y)) {
          t->velocity = newDir;
        }
      }
    }
  }
}

void Game::Update(float deltaTime) {
  m_world->Update(deltaTime);

  m_snakeAccumulator += deltaTime;
  while (m_snakeAccumulator >= kSnakeStepTime) {
    UpdateSnakeStep();
    m_snakeAccumulator -= kSnakeStepTime;
  }
}

void Game::Render() {
  SDL_SetRenderDrawColor(m_renderer, 18, 18, 18, 255);
  SDL_RenderClear(m_renderer);
  m_world->Render(m_renderer);
  SDL_RenderPresent(m_renderer);
}

void Game::UpdateSnakeStep() {
  auto *headT = m_world->GetComponent<TransformComponent>(m_snakeHead);
  if (!headT)
    return;

  // 1. Save head's current position BEFORE moving
  const glm::vec2 prevHeadPos = headT->position;

  // 2. Move head
  headT->position.x += headT->velocity.x * cellSize;
  headT->position.y += headT->velocity.y * cellSize;

  // 3. Cascade body: each segment takes the position of the segment in front of
  // it.
  if (!m_snakeBody.empty()) {
    // Shift positions: segment[i] gets segment[i-1]'s old position
    glm::vec2 prev = prevHeadPos;
    for (EntityId seg : m_snakeBody) {
      auto *segT = m_world->GetComponent<TransformComponent>(seg);
      if (!segT)
        continue;
      glm::vec2 tmp = segT->position;
      segT->position = prev;
      prev = tmp;
    }
  }

  // 4. Collision check with food
  if (headT && m_food != 0) {
    if (auto *foodT = m_world->GetComponent<TransformComponent>(m_food)) {
      if (headT->position == foodT->position) {
        GrowSnake();
        SpawnFood();
      }
    }
  }

  // 5. Bounds collision -> reset
  const int cellsX = static_cast<int>(kWidth / cellSize);
  const int cellsY = static_cast<int>(kHeight / cellSize);
  const bool oob = headT->position.x < 0 || headT->position.y < 0 ||
                   headT->position.x >= kWidth || headT->position.y >= kHeight;
  if (oob) {
    ResetGame();
    return;
  }

  // 6. Self collision
  for (std::size_t i = 1; i < m_snakeBody.size(); ++i) {
    auto *segT = m_world->GetComponent<TransformComponent>(m_snakeBody[i]);
    if (segT && headT->position == segT->position) {
      ResetGame();
      return;
    }
  }
}

void Game::GrowSnake() {
  // Spawn an ew segment at the current tail position
  // (it will be covered by the tail until the next step - invisible flash)
  EntityId seg = m_world->CreateEntity();

  // Find last known position: tail if body exists, else head
  glm::vec2 spawnPos{};
  if (!m_snakeBody.empty()) {
    auto *t = m_world->GetComponent<TransformComponent>(m_snakeBody.back());
    spawnPos = t ? t->position : glm::vec2{};
  } else {
    auto *t = m_world->GetComponent<TransformComponent>(m_snakeHead);
    spawnPos = t ? t->position : glm::vec2{};
  }

  auto &t = m_world->AddComponent<TransformComponent>(seg);
  t.position = spawnPos;
  t.size = {cellSize, cellSize};
  t.velocity = {0.0f, 0.0f};

  m_world->AddComponent<SpriteComponent>(seg, &t, SDL_Color{0, 200, 0, 255});
  m_world->AddComponent<TagComponent>(seg).tag = "snake_body";

  m_snakeBody.push_back(seg);
}

void Game::SpawnFood() {

  if (m_food != 0) {
    m_world->DestroyEntity(m_food);
  }

  m_food = m_world->CreateEntity();

  auto &t = m_world->AddComponent<TransformComponent>(m_food);

  // Static RNG setup
  static std::random_device rd;
  static std::mt19937 gen(rd());

  // Get number of whole cells that fit horizontally and vertically
  const int cellsX = static_cast<int>(kWidth / cellSize);
  const int cellsY = static_cast<int>(kHeight / cellSize);

  std::uniform_int_distribution<int> distX(0, cellsX - 1);
  std::uniform_int_distribution<int> distY(0, cellsY - 1);

  // TODO: Randomize
  const float gridX = distX(gen);
  const float gridY = distY(gen);
  t.position = {gridX * cellSize, gridY * cellSize}; // placeholder
  t.size = {cellSize, cellSize};

  m_world->AddComponent<SpriteComponent>(m_food, &t, SDL_Color{255, 0, 0, 255});

  auto &tag = m_world->AddComponent<TagComponent>(m_food);
  tag.tag = "food";
}

void Game::ResetGame() {
  // Destroy all body segments
  for (EntityId seg : m_snakeBody)
    m_world->DestroyEntity(seg);
  m_snakeBody.clear();

  // Start position and direction for head
  if (auto *t = m_world->GetComponent<TransformComponent>(m_snakeHead)) {
    t->position = {10.0f * cellSize, 10.0f * cellSize};
    t->velocity = {1.0f, 0.0f};
  }

  SpawnFood();
  std::cout << "[Game] Reset" << std::endl;
}
