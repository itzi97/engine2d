#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>

/// Tracks per-frame keyboard and mouse button state.
/// Call ProcessEvent() for every SDL event, then EndFrame() after Update().
class InputManager {
public:
  // ── Keyboard ──────────────────────────────────────────────────────────────

  /// Feed every SDL_Event here during ProcessEvents.
  void ProcessEvent(const SDL_Event &event);

  /// Copy current state to previous. Call once per frame after Update.
  void EndFrame();

  /// True while the key is held down.
  [[nodiscard]] bool IsKeyPressed(SDL_Keycode key)     const;

  /// True only on the frame the key first went down.
  [[nodiscard]] bool IsKeyJustPressed(SDL_Keycode key)  const;

  /// True only on the frame the key was released.
  [[nodiscard]] bool IsKeyJustReleased(SDL_Keycode key) const;

  // ── Mouse ─────────────────────────────────────────────────────────────────

  /// True while the mouse button is held (1=left, 2=middle, 3=right).
  [[nodiscard]] bool IsMousePressed(int button)     const;

  /// True only on the frame the button first went down.
  [[nodiscard]] bool IsMouseJustPressed(int button)  const;

  /// True only on the frame the button was released.
  [[nodiscard]] bool IsMouseJustReleased(int button) const;

  /// Current cursor position in window coordinates.
  [[nodiscard]] float MouseX() const { return m_mouseX; }
  [[nodiscard]] float MouseY() const { return m_mouseY; }

private:
  std::unordered_map<SDL_Keycode, bool> m_curr;
  std::unordered_map<SDL_Keycode, bool> m_prev;

  std::unordered_map<int, bool> m_mouseCurr;
  std::unordered_map<int, bool> m_mousePrev;

  float m_mouseX{0.f};
  float m_mouseY{0.f};
};
