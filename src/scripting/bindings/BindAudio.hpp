#pragma once

// Forward-declare sol::state without pulling in sol/sol.hpp.
namespace sol { class state; }
class AudioManager;

// Extends the global "engine" Lua table with audio bindings:
//   load_sfx, play_sfx, load_music, play_music,
//   pause_music, resume_music, stop_music, set_music_volume
//
// Must be called after BindEngine so the "engine" table already exists.
void BindAudio(sol::state &lua, AudioManager *audio);
