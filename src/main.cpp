#include "core/Game.hpp"

#include <filesystem>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#  include <direct.h>
#  define CHDIR(p) _chdir(p)
#else
#  include <unistd.h>
#  define CHDIR(p) chdir(p)
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Resolve the project root from the binary path and chdir into it so that
// all relative paths (assets/, scripts/) work regardless of how the binary
// is launched.  argv[0] is typically  <root>/build/2d-engine, so the project
// root is two levels up.  We verify the guess by checking for CMakeLists.txt.
// ---------------------------------------------------------------------------
static void SetWorkingDir(const char *argv0) {
    std::error_code ec;
    fs::path bin = fs::canonical(fs::path(argv0), ec);
    if (ec) return;                          // can't resolve — leave CWD alone

    // Walk upward from the binary until we find CMakeLists.txt (max 4 hops).
    fs::path candidate = bin.parent_path();
    for (int i = 0; i < 4; ++i) {
        if (fs::exists(candidate / "CMakeLists.txt", ec)) {
            CHDIR(candidate.string().c_str());
            return;
        }
        fs::path parent = candidate.parent_path();
        if (parent == candidate) break;      // filesystem root
        candidate = parent;
    }
    // Didn't find CMakeLists.txt — leave CWD unchanged.
}

int main(int argc, char *argv[]) {
    (void)argc;
    SetWorkingDir(argv[0]);

    Game game;

    if (!game.Initialize()) {
        std::cerr << "[main] Game::Initialize() failed. Exiting.\n";
        return EXIT_FAILURE;
    }

    game.Run();
    game.Shutdown();

    return EXIT_SUCCESS;
}
