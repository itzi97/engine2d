#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>
#include <unordered_set>

/// Tracks per-frame keyboard and mouse button state.
/// Call ProcessEvent() for every SDL event, then EndFrame() after Update().
class InputManager {
public:
  // ── Keyboard ──────────────────────────────────────────────────────────────────────────

  /// Feed every SDL_Event here during ProcessEvents.
  void ProcessEvent(const SDL_Event &event);

  /// Clear just-pressed/released sets. Call once per frame after Update.
  void EndFrame();

  /// True while the key is held down.
  [[nodiscard]] bool IsKeyPressed(SDL_Keycode key)     const;

  /// True only on the frame the key first went down (event-driven, not diff).
  [[nodiscard]] bool IsKeyJustPressed(SDL_Keycode key)  const;

  /// True only on the frame the key was released (event-driven, not diff).
  [[nodiscard]] bool IsKeyJustReleased(SDL_Keycode key) const;

  // ── Mouse ─────────────────────────────────────────────────────────────────────────────

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
  // Held-down state (bool map, updated on every KEY_DOWN/KEY_UP)
  std::unordered_map<SDL_Keycode, bool> m_curr;

  // Event-driven single-frame sets — populated in ProcessEvent, cleared in EndFrame
  std::unordered_set<SDL_Keycode> m_justPressed;
  std::unordered_set<SDL_Keycode> m_justReleased;

  std::unordered_map<int, bool> m_mouseCurr;
  std::unordered_set<int>       m_mouseJustPressed;
  std::unordered_set<int>       m_mouseJustReleased;

  float m_mouseX{0.f};
  float m_mouseY{0.f};
};
