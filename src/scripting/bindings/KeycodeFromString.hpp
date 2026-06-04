#pragma once
#include <string>
#include <SDL3/SDL.h>

// Converts an engine-facing uppercase key name ("SPACE", "UP", "F", …)
// to the SDL_Keycode used by SDL_KeyboardEvent.key.
// Special cases handle names that differ from SDL's own canonical names;
// everything else is lowercased and resolved via SDL_GetKeyFromName so the
// mapping stays in sync with the runtime SDL build.
SDL_Keycode KeycodeFromString(const std::string &key);
