#pragma once
#include "ecs/Component.hpp"
#include <SDL3/SDL.h>
#include <vector>

// Drives a frame-by-frame sprite animation by cycling through a list of
// source rectangles and writing the current one into SpriteComponent::srcRect
// each tick. Requires SpriteComponent on the same entity.
//
// Lua API (world table):
//   world.add_animation(e, frames, frame_duration [, loop])
//     frames         : array-table of {x,y,w,h} rects
//     frame_duration : seconds per frame (e.g. 0.1)
//     loop           : bool, default true
//   world.set_animation_playing(e, bool)
//   world.reset_animation(e)           -- jump back to frame 0
struct AnimationComponent : Component {
  std::vector<SDL_FRect> frames;         // ordered list of atlas src rects
  float  frameDuration{0.1f};            // seconds per frame
  float  timer{0.f};                     // accumulated time within current frame
  int    currentFrame{0};                // index into frames[]
  bool   loop{true};
  bool   playing{true};
};
