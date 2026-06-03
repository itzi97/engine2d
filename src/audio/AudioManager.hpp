#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// AudioManager — SDL3 native audio streams, no SDL3_mixer dependency.
//
// OGG files are decoded with stb_vorbis (vendored header-only lib).
// WAV files are decoded with SDL_LoadWAV.
//
// Two categories:
//   SFX   — decoded fully into RAM, replayed by pushing PCM onto a
//            dedicated SDL_AudioStream each play() call. Multiple SFX
//            can overlap because each has its own stream.
//   Music — decoded fully into RAM, looped (or one-shot) by an
//            SDL_AudioStream that refills itself via an audio callback.
//
// Lua API (engine table, bound via ScriptingEngine::BindAudio):
//   engine.load_sfx(path)              -> handle  (-1 on error)
//   engine.play_sfx(handle [,volume])  -> void    (volume 0.0-1.0)
//   engine.load_music(path)            -> handle  (-1 on error)
//   engine.play_music(handle [,loop])  -> void    (loop default true)
//   engine.pause_music()               -> void
//   engine.resume_music()              -> void
//   engine.stop_music()                -> void
//   engine.set_music_volume(v)         -> void    (0.0-1.0)
// ---------------------------------------------------------------------------

struct AudioClip {
  std::vector<float> samples;   // interleaved float PCM
  int                channels{1};
  int                sampleRate{44100};
};

class AudioManager {
public:
  AudioManager();
  ~AudioManager();

  AudioManager(const AudioManager &)            = delete;
  AudioManager &operator=(const AudioManager &) = delete;

  [[nodiscard]] bool IsReady() const { return m_deviceId != 0; }

  // SFX
  [[nodiscard]] int  LoadSfx(const std::string &path);
  void               PlaySfx(int handle, float volume = 1.f);

  // Music
  [[nodiscard]] int  LoadMusic(const std::string &path);
  void               PlayMusic(int handle, bool loop = true);
  void               PauseMusic();
  void               ResumeMusic();
  void               StopMusic();
  void               SetMusicVolume(float volume);

private:
  [[nodiscard]] bool LoadOgg(const std::string &path, AudioClip &out);
  [[nodiscard]] bool LoadWav(const std::string &path, AudioClip &out);
  [[nodiscard]] bool LoadClip(const std::string &path, AudioClip &out);

  SDL_AudioDeviceID m_deviceId{0};
  SDL_AudioSpec     m_spec{};

  // SFX: one SDL_AudioStream per handle (allows overlap)
  std::unordered_map<int, AudioClip>       m_sfxClips;
  int                                      m_nextSfxId{0};

  // Music: single stream that we manage manually
  std::unordered_map<int, AudioClip>       m_musicClips;
  int                                      m_nextMusicId{0};

  SDL_AudioStream *m_musicStream{nullptr};
  int              m_currentMusic{-1};
  bool             m_musicLoop{true};
  float            m_musicVolume{1.f};
  size_t           m_musicPos{0};   // sample index into current clip

  // Called from the SDL audio callback to keep the music stream fed.
  static void SDLCALL AudioCallback(void *userdata, SDL_AudioStream *stream,
                                    int additionalAmount, int totalAmount);
  void FeedMusic(SDL_AudioStream *stream, int additionalAmount);
};
