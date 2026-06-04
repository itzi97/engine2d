#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindAudio.hpp"
#include "audio/AudioManager.hpp"

void BindAudio(sol::state &lua, AudioManager *audio) {
  // Extend the existing engine table — must NOT use create_named_table here
  // as that would replace the table BindEngine already populated.
  sol::table eng = lua["engine"];

  eng.set_function("load_sfx",
      [audio](const std::string &path) -> int { return audio->LoadSfx(path); });
  eng.set_function("play_sfx",
      [audio](int handle, sol::optional<float> vol) {
        audio->PlaySfx(handle, vol.value_or(1.f));
      });
  eng.set_function("load_music",
      [audio](const std::string &path) -> int { return audio->LoadMusic(path); });
  eng.set_function("play_music",
      [audio](int handle, sol::optional<bool> loop) {
        audio->PlayMusic(handle, loop.value_or(true));
      });
  eng.set_function("pause_music",      [audio]() { audio->PauseMusic(); });
  eng.set_function("resume_music",     [audio]() { audio->ResumeMusic(); });
  eng.set_function("stop_music",       [audio]() { audio->StopMusic(); });
  eng.set_function("set_music_volume", [audio](float v) { audio->SetMusicVolume(v); });
}
