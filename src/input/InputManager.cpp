#include "input/InputManager.hpp"
#include <SDL3/SDL.h>

void InputManager::ProcessEvent(const SDL_Event &event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      SDL_Log("[Input] KEY_DOWN  key=0x%X  scancode=%d  repeat=%d",
              (unsigned)event.key.key, (int)event.key.scancode, (int)event.key.repeat);
      m_curr[event.key.key] = true;
      break;
    case SDL_EVENT_KEY_UP:
      SDL_Log("[Input] KEY_UP    key=0x%X  scancode=%d",
              (unsigned)event.key.key, (int)event.key.scancode);
      m_curr[event.key.key] = false;
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      m_mouseCurr[event.button.button] = true;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
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
  m_prev      = m_curr;
  m_mousePrev = m_mouseCurr;
}

bool InputManager::IsKeyPressed(SDL_Keycode key) const {
  const auto it = m_curr.find(key);
  return it != m_curr.end() && it->second;
}

bool InputManager::IsKeyJustPressed(SDL_Keycode key) const {
  const auto c = m_curr.find(key);
  const auto p = m_prev.find(key);
  const bool curr = c != m_curr.end() && c->second;
  const bool prev = p != m_prev.end() && p->second;
  const bool result = curr && !prev;
  if (result)
    SDL_Log("[Input] IsKeyJustPressed FIRED key=0x%X", (unsigned)key);
  return result;
}

bool InputManager::IsKeyJustReleased(SDL_Keycode key) const {
  const auto c = m_curr.find(key);
  const auto p = m_prev.find(key);
  const bool curr = c != m_curr.end() && c->second;
  const bool prev = p != m_prev.end() && p->second;
  return !curr && prev;
}

bool InputManager::IsMousePressed(int button) const {
  const auto it = m_mouseCurr.find(button);
  return it != m_mouseCurr.end() && it->second;
}

bool InputManager::IsMouseJustPressed(int button) const {
  const bool curr = IsMousePressed(button);
  const auto p    = m_mousePrev.find(button);
  const bool prev = p != m_mousePrev.end() && p->second;
  return curr && !prev;
}

bool InputManager::IsMouseJustReleased(int button) const {
  const bool curr = IsMousePressed(button);
  const auto p    = m_mousePrev.find(button);
  const bool prev = p != m_mousePrev.end() && p->second;
  return !curr && prev;
}
