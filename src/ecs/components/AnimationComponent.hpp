#pragma once
#include "ecs/Component.hpp"
#include <vector>

// A single atlas frame — intentionally SDL-free so the ECS layer does not
// depend on the renderer.  AnimationSystem converts to SDL_FRect when writing
// into SpriteComponent::srcRect.
struct Frame {
  float x{0}, y{0}, w{0}, h{0};
};

// Drives a frame-by-frame sprite animation by cycling through a list of
// Frame values and writing the current one into SpriteComponent::srcRect
// each tick.  Requires SpriteComponent on the same entity.
//
// Lua API (world table):
//   world.add_animation(e, frames, frame_duration [, loop])
//     frames         : array-table of {x,y,w,h} rects
//     frame_duration : seconds per frame (e.g. 0.1)
//     loop           : bool, default true
//   world.set_animation_playing(e, bool)
//   world.reset_animation(e)      -- jump back to frame 0, resume playing
struct AnimationComponent : Component {
  std::vector<Frame> frames;
  float frameDuration{0.1f};
  float timer{0.f};
  int   currentFrame{0};
  bool  loop{true};
  bool  playing{true};
};
