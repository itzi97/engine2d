# GenerateGameShim.cmake
# Called by CMake at configure time to write:
#   build/generated/game_script_shim.hpp
# which simply includes the correct embedded_<GAME>.hpp and exposes:
#   game_script::source  (const char*)
#   game_script::name    (const char*)

set(SHIM_FILE "${OUT_DIR}/game_script_shim.hpp")
file(WRITE "${SHIM_FILE}"
"#pragma once
#include \"embedded_${GAME_NAME}.hpp\"
namespace game_script {
  inline constexpr const char *source = embedded::${GAME_NAME};
  inline constexpr const char *name   = \"${GAME_NAME}\";
}
")
