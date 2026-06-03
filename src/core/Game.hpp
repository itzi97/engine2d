#pragma once

#include "ecs/World.hpp"
#include "scripting/ScriptingEngine.hpp"

#include <SDL3/SDL.h>
#include <deque>
#include <memory>
#include <string_view>

// TODO: Move to Lua
static const float cellSize = 32.0f;

class Game {
public:
  Game();
  ~Game();

  Game(const Game &) = delete;
  Game &operator=(const Game &) = delete;
  Game(Game &&) = delete;
  Game &operator=(Game &&) = delete;

  [[nodiscard]] bool Initialize();
  void Run();
  void Shutdown();

private:
  void ProcessEvents(bool &running);
  void Update(float deltaTime);
  void Render();

  static constexpr std::string_view kTitle = "2D Engine";
  static constexpr int kWidth = 1280;
  static constexpr int kHeight = 720;
  static constexpr float kFixedDt = 1.0f / 60.0f;

  SDL_Window *m_window = nullptr;
  SDL_Renderer *m_renderer = nullptr;

  std::unique_ptr<World> m_world;
  std::unique_ptr<ScriptingEngine> m_scripting;

  // TODO: Move to Lua
  EntityId m_snakeHead{};
  EntityId m_food{};
  std::deque<EntityId> m_snakeBody;
  float m_snakeAccumulator = 0.0f;
  static constexpr float kSnakeStepTime = 1.0f / 10.0f;

  void ResetGame();
  void UpdateSnakeStep();
  void GrowSnake();
  void SpawnFood();
};
