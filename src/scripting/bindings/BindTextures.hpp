#pragma once

// Forward-declare sol::state without pulling in sol/sol.hpp.
namespace sol { class state; }
class TextureManager;

// Extends the global "engine" Lua table with engine.load_texture.
// Must be called after BindEngine so the "engine" table already exists.
void BindTextures(sol::state &lua, TextureManager *textures);
