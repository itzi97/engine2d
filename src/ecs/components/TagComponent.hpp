#pragma once
#include "ecs/Component.hpp"
#include <string>

struct TagComponent : Component {
  std::string tag;
};
