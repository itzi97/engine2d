#include "scripting/bindings/KeycodeFromString.hpp"
#include <cctype>

SDL_Keycode KeycodeFromString(const std::string &key) {
  if (key == "SPACE")     return SDLK_SPACE;
  if (key == "RETURN")    return SDLK_RETURN;
  if (key == "ESCAPE")    return SDLK_ESCAPE;
  if (key == "UP")        return SDLK_UP;
  if (key == "DOWN")      return SDLK_DOWN;
  if (key == "LEFT")      return SDLK_LEFT;
  if (key == "RIGHT")     return SDLK_RIGHT;
  if (key == "LSHIFT")    return SDLK_LSHIFT;
  if (key == "RSHIFT")    return SDLK_RSHIFT;
  if (key == "LCTRL")     return SDLK_LCTRL;
  if (key == "RCTRL")     return SDLK_RCTRL;
  if (key == "LALT")      return SDLK_LALT;
  if (key == "RALT")      return SDLK_RALT;
  if (key == "TAB")       return SDLK_TAB;
  if (key == "BACKSPACE") return SDLK_BACKSPACE;
  if (key == "DELETE")    return SDLK_DELETE;

  std::string sdlName = key;
  for (auto &c : sdlName)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return SDL_GetKeyFromName(sdlName.c_str());
}
