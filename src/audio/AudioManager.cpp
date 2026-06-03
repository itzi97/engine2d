#include "audio/AudioManager.hpp"
#include <SDL3/SDL.h>
#include <iostream>

static constexpr int   kMixFrequency = MIX_DEFAULT_FREQUENCY;
static constexpr int   kMixChannels  = 8; // simultaneous SFX channels

AudioManager::AudioManager() {
  if (Mix_Init(MIX_INIT_OGG) == 0) {
    std::cerr << "[AudioManager] Mix_Init failed: " << SDL_GetError() << '\n';
    return;
  }
  if (!Mix_OpenAudio(0, nullptr)) {
    std::cerr << "[AudioManager] Mix_OpenAudio failed: " << SDL_GetError() << '\n';
    Mix_Quit();
    return;
  }
  Mix_AllocateChannels(kMixChannels);
  m_ready = true;
}

AudioManager::~AudioManager() {
  for (auto &[id, chunk] : m_sfx)   Mix_FreeChunk(chunk);
  for (auto &[id, music] : m_music) Mix_FreeMusic(music);
  if (m_ready) {
    Mix_CloseAudio();
    Mix_Quit();
  }
}

int AudioManager::LoadSfx(const std::string &path) {
  Mix_Chunk *chunk = Mix_LoadWAV(path.c_str());
  if (!chunk) {
    std::cerr << "[AudioManager] LoadSfx failed ('" << path << "'): "
              << SDL_GetError() << '\n';
    return -1;
  }
  const int id  = m_nextSfxId++;
  m_sfx[id]     = chunk;
  return id;
}

void AudioManager::PlaySfx(int handle, float volume) {
  const auto it = m_sfx.find(handle);
  if (it == m_sfx.end()) return;
  // volume 0.0-1.0 -> 0-128
  Mix_VolumeChunk(it->second,
      static_cast<int>(volume * static_cast<float>(MIX_MAX_VOLUME)));
  Mix_PlayChannel(-1, it->second, 0); // -1 = first free channel
}

int AudioManager::LoadMusic(const std::string &path) {
  Mix_Music *music = Mix_LoadMUS(path.c_str());
  if (!music) {
    std::cerr << "[AudioManager] LoadMusic failed ('" << path << "'): "
              << SDL_GetError() << '\n';
    return -1;
  }
  const int id   = m_nextMusicId++;
  m_music[id]    = music;
  return id;
}

void AudioManager::PlayMusic(int handle, bool loop) {
  const auto it = m_music.find(handle);
  if (it == m_music.end()) return;
  m_currentMusic = handle;
  Mix_PlayMusic(it->second, loop ? -1 : 1); // -1 = loop forever, 1 = play once
}

void AudioManager::PauseMusic()  { Mix_PauseMusic(); }
void AudioManager::ResumeMusic() { Mix_ResumeMusic(); }
void AudioManager::StopMusic()   { Mix_HaltMusic(); }

void AudioManager::SetMusicVolume(float volume) {
  Mix_VolumeMusic(
      static_cast<int>(volume * static_cast<float>(MIX_MAX_VOLUME)));
}
