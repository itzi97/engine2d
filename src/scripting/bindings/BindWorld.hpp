#pragma once

namespace sol { class state; }
class World;
class TextureManager;

void BindWorld(sol::state &lua, World *world, TextureManager *textures);
