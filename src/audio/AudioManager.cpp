#include "audio/AudioManager.hpp"

// stb_vorbis implementation is compiled in stb_vorbis_impl.c;
// include as header-only here to get the declarations.
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool AudioManager::LoadOgg(const std::string &path, AudioClip &out) {
  int   channels = 0, sampleRate = 0;
  short *decoded  = nullptr;
  const int frames = stb_vorbis_decode_filename(
      path.c_str(), &channels, &sampleRate, &decoded);
  if (frames <= 0 || !decoded) {
    std::cerr << "[AudioManager] stb_vorbis failed: '" << path << "'\n";
    return false;
  }
  const size_t total = static_cast<size_t>(frames * channels);
  out.samples.resize(total);
  for (size_t i = 0; i < total; ++i)
    out.samples[i] = static_cast<float>(decoded[i]) / 32768.f;
  out.channels   = channels;
  out.sampleRate = sampleRate;
  free(decoded);
  return true;
}

bool AudioManager::LoadWav(const std::string &path, AudioClip &out) {
  SDL_AudioSpec spec{};
  Uint8 *buf = nullptr;
  Uint32 len = 0;
  if (!SDL_LoadWAV(path.c_str(), &spec, &buf, &len)) {
    std::cerr << "[AudioManager] SDL_LoadWAV failed: '" << path
              << "': " << SDL_GetError() << '\n';
    return false;
  }
  SDL_AudioSpec dst{SDL_AUDIO_F32, spec.channels, spec.freq};
  SDL_AudioStream *cvt = SDL_CreateAudioStream(&spec, &dst);
  SDL_PutAudioStreamData(cvt, buf, static_cast<int>(len));
  SDL_FlushAudioStream(cvt);
  SDL_free(buf);
  const int avail = SDL_GetAudioStreamAvailable(cvt);
  out.samples.resize(static_cast<size_t>(avail) / sizeof(float));
  SDL_GetAudioStreamData(cvt, out.samples.data(), avail);
  SDL_DestroyAudioStream(cvt);
  out.channels   = spec.channels;
  out.sampleRate = spec.freq;
  return true;
}

bool AudioManager::LoadClip(const std::string &path, AudioClip &out) {
  const auto dot = path.rfind('.');
  if (dot != std::string::npos) {
    std::string ext = path.substr(dot + 1);
    for (auto &c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    if (ext == "ogg") return LoadOgg(path, out);
    if (ext == "wav") return LoadWav(path, out);
  }
  return LoadWav(path, out) || LoadOgg(path, out);
}

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------

AudioManager::AudioManager() {
  m_spec     = {SDL_AUDIO_F32, 2, 44100};
  m_deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_spec);
  if (m_deviceId == 0)
    std::cerr << "[AudioManager] SDL_OpenAudioDevice failed: " << SDL_GetError() << '\n';
}

AudioManager::~AudioManager() {
  if (m_musicStream) {
    SDL_DestroyAudioStream(m_musicStream);
    m_musicStream = nullptr;
  }
  if (m_deviceId) {
    SDL_CloseAudioDevice(m_deviceId);
    m_deviceId = 0;
  }
}

// ---------------------------------------------------------------------------
// SFX
// ---------------------------------------------------------------------------

int AudioManager::LoadSfx(const std::string &path) {
  AudioClip clip;
  if (!LoadClip(path, clip)) return -1;
  const int id   = m_nextSfxId++;
  m_sfxClips[id] = std::move(clip);
  return id;
}

void AudioManager::PlaySfx(int handle, float volume) {
  const auto it = m_sfxClips.find(handle);
  if (it == m_sfxClips.end()) return;

  const AudioClip &clip = it->second;
  SDL_AudioSpec src{SDL_AUDIO_F32, clip.channels, clip.sampleRate};
  SDL_AudioStream *stream = SDL_CreateAudioStream(&src, &m_spec);
  if (!stream) return;

  // Apply volume via SDL gain — no copy of the sample buffer needed.
  SDL_SetAudioStreamGain(stream, std::clamp(volume, 0.f, 1.f));

  SDL_PutAudioStreamData(stream, clip.samples.data(),
      static_cast<int>(clip.samples.size() * sizeof(float)));
  SDL_FlushAudioStream(stream);
  SDL_BindAudioStream(m_deviceId, stream);

  // Self-destruct once the stream has been fully consumed.
  SDL_SetAudioStreamPutCallback(stream,
      [](void *, SDL_AudioStream *s, int, int avail) {
        if (avail == 0) {
          SDL_UnbindAudioStream(s);
          SDL_DestroyAudioStream(s);
        }
      }, nullptr);
}

// ---------------------------------------------------------------------------
// Music
// ---------------------------------------------------------------------------

int AudioManager::LoadMusic(const std::string &path) {
  AudioClip clip;
  if (!LoadClip(path, clip)) return -1;
  const int id     = m_nextMusicId++;
  m_musicClips[id] = std::move(clip);
  return id;
}

void AudioManager::AudioCallback(void *userdata, SDL_AudioStream *stream,
                                 int additionalAmount, int /*totalAmount*/) {
  static_cast<AudioManager *>(userdata)->FeedMusic(stream, additionalAmount);
}

void AudioManager::FeedMusic(SDL_AudioStream *stream, int bytes) {
  if (m_currentMusic < 0) return;
  const auto it = m_musicClips.find(m_currentMusic);
  if (it == m_musicClips.end()) return;

  const AudioClip &clip = it->second;
  const size_t    total = clip.samples.size();

  // Use a stack buffer to avoid heap allocation on the audio thread.
  static constexpr size_t kBufFloats = 2048;
  std::array<float, kBufFloats> buf;

  int remaining = bytes;
  while (remaining > 0) {
    const size_t floatsAvail = total - m_musicPos;
    if (floatsAvail == 0) {
      if (!m_musicLoop) break;
      m_musicPos = 0;
      continue;
    }
    const size_t floatsWanted = std::min(
        static_cast<size_t>(remaining) / sizeof(float),
        kBufFloats);
    const size_t chunk = std::min(floatsWanted, floatsAvail);

    for (size_t i = 0; i < chunk; ++i)
      buf[i] = clip.samples[m_musicPos + i] * m_musicVolume;

    SDL_PutAudioStreamData(stream, buf.data(),
        static_cast<int>(chunk * sizeof(float)));
    m_musicPos += chunk;
    remaining  -= static_cast<int>(chunk * sizeof(float));
  }
}

void AudioManager::PlayMusic(int handle, bool loop) {
  if (m_musicStream) {
    SDL_UnbindAudioStream(m_musicStream);
    SDL_DestroyAudioStream(m_musicStream);
    m_musicStream = nullptr;
  }
  const auto it = m_musicClips.find(handle);
  if (it == m_musicClips.end()) return;

  const AudioClip &clip = it->second;
  SDL_AudioSpec src{SDL_AUDIO_F32, clip.channels, clip.sampleRate};
  m_musicStream = SDL_CreateAudioStream(&src, &m_spec);
  if (!m_musicStream) return;

  m_currentMusic = handle;
  m_musicLoop    = loop;
  m_musicPos     = 0;

  SDL_SetAudioStreamGetCallback(m_musicStream, AudioCallback, this);
  SDL_BindAudioStream(m_deviceId, m_musicStream);
}

void AudioManager::PauseMusic() {
  if (m_musicStream) SDL_PauseAudioStreamDevice(m_musicStream);
}

void AudioManager::ResumeMusic() {
  if (m_musicStream) SDL_ResumeAudioStreamDevice(m_musicStream);
}

void AudioManager::StopMusic() {
  if (!m_musicStream) return;
  SDL_UnbindAudioStream(m_musicStream);
  SDL_DestroyAudioStream(m_musicStream);
  m_musicStream  = nullptr;
  m_currentMusic = -1;
  m_musicPos     = 0;
}

void AudioManager::SetMusicVolume(float volume) {
  m_musicVolume = std::clamp(volume, 0.f, 1.f);
}
