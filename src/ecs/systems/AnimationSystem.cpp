#include "ecs/systems/AnimationSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/AnimationComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"

void AnimationSystem::Update(World &world, float dt) {
  world.ForEach<AnimationComponent>([&](EntityId e, AnimationComponent &anim) {
    if (!anim.playing || anim.frames.empty()) return;

    anim.timer += dt;
    if (anim.timer >= anim.frameDuration) {
      anim.timer -= anim.frameDuration;
      const int frameCount = static_cast<int>(anim.frames.size());
      anim.currentFrame++;
      if (anim.currentFrame >= frameCount) {
        anim.currentFrame = anim.loop ? 0 : frameCount - 1;
        if (!anim.loop) anim.playing = false;
      }
    }

    // Write current frame rect into SpriteComponent so RenderSystem picks it up.
    if (auto *spr = world.GetComponent<SpriteComponent>(e))
      spr->srcRect = anim.frames[static_cast<std::size_t>(anim.currentFrame)];
  });
}
