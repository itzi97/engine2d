#pragma once
#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <unordered_map>

// ---------------------------------------------------------------------------
// AudioManager
// ---------------------------------------------------------------------------
// Two-tier audio:
//   SFX   — short one-shot clips loaded as Mix_Chunk via SDL3_mixer.
//            Multiple chunks can play simultaneously on mixer channels.
//   Music — single looping (or one-shot) track loaded as Mix_Music.
//            Only one music track plays at a time.
//
// Handles are plain ints returned to Lua.  Internally they key into
// m_sfx / m_music maps so the Lua side never holds raw pointers.
//
// Lua API (engine table, bound in ScriptingEngine::BindAudio):
//   engine.load_sfx(path)              -> handle  (or -1 on error)
//   engine.play_sfx(handle [,volume])  -> void    (volume 0.0-1.0, default 1.0)
//   engine.load_music(path)            -> handle  (or -1 on error)
//   engine.play_music(handle [,loop])  -> void    (loop default true)
//   engine.pause_music()               -> void
//   engine.resume_music()              -> void
//   engine.stop_music()                -> void
//   engine.set_music_volume(0.0-1.0)   -> void
// ---------------------------------------------------------------------------
class AudioManager {
public:
  AudioManager();
  ~AudioManager();

  AudioManager(const AudioManager &)            = delete;
  AudioManager &operator=(const AudioManager &) = delete;

  [[nodiscard]] bool IsReady() const { return m_ready; }

  // SFX
  [[nodiscard]] int  LoadSfx(const std::string &path);
  void               PlaySfx(int handle, float volume = 1.f);

  // Music
  [[nodiscard]] int  LoadMusic(const std::string &path);
  void               PlayMusic(int handle, bool loop = true);
  void               PauseMusic();
  void               ResumeMusic();
  void               StopMusic();
  void               SetMusicVolume(float volume); // 0.0 – 1.0

private:
  bool m_ready{false};

  std::unordered_map<int, Mix_Chunk *> m_sfx;
  std::unordered_map<int, Mix_Music *> m_music;
  int m_nextSfxId{0};
  int m_nextMusicId{0};
  int m_currentMusic{-1}; // handle of currently loaded music track
};
