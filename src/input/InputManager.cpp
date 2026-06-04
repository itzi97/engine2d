#include "input/InputManager.hpp"
#include <SDL3/SDL.h>

void InputManager::ProcessEvent(const SDL_Event &event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      if (!event.key.repeat)
        m_justPressed.insert(event.key.key);
      m_curr[event.key.key] = true;
      break;

    case SDL_EVENT_KEY_UP:
      m_justReleased.insert(event.key.key);
      m_curr[event.key.key] = false;
      break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      m_mouseJustPressed.insert(event.button.button);
      m_mouseCurr[event.button.button] = true;
      break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
      m_mouseJustReleased.insert(event.button.button);
      m_mouseCurr[event.button.button] = false;
      break;

    case SDL_EVENT_MOUSE_MOTION:
      m_mouseX = event.motion.x;
      m_mouseY = event.motion.y;
      break;

    default:
      break;
  }
}

void InputManager::EndFrame() {
  m_justPressed.clear();
  m_justReleased.clear();
  m_mouseJustPressed.clear();
  m_mouseJustReleased.clear();
}

bool InputManager::IsKeyPressed(SDL_Keycode key) const {
  const auto it = m_curr.find(key);
  return it != m_curr.end() && it->second;
}

bool InputManager::IsKeyJustPressed(SDL_Keycode key) const {
  return m_justPressed.count(key) > 0;
}

bool InputManager::IsKeyJustReleased(SDL_Keycode key) const {
  return m_justReleased.count(key) > 0;
}

bool InputManager::IsMousePressed(int button) const {
  const auto it = m_mouseCurr.find(button);
  return it != m_mouseCurr.end() && it->second;
}

bool InputManager::IsMouseJustPressed(int button) const {
  return m_mouseJustPressed.count(button) > 0;
}

bool InputManager::IsMouseJustReleased(int button) const {
  return m_mouseJustReleased.count(button) > 0;
}
