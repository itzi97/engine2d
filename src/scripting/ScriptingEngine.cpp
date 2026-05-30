// -- All sol2 includes MUST stay in this translation unit --------------------
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
// ---------------------------------------------------------------------------

#include "scripting/ScriptingEngine.hpp"
#include <iostream>

struct ScriptingEngine::Impl
{
    sol::state lua;

    Impl()
    {
        lua.open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table,
            sol::lib::io
        );
        RegisterBindings();
    }

    void RegisterBindings()
    {
        lua.set_function("log", [](const std::string& msg) {
            std::cout << "[Lua] " << msg << '\n';
        });
        // TODO: bind World, EntityId, GLM types, SDL input helpers here.
    }
};

ScriptingEngine::ScriptingEngine()
    : m_impl(std::make_unique<Impl>())
{}

ScriptingEngine::~ScriptingEngine() = default;

bool ScriptingEngine::RunScript(const std::filesystem::path& path)
{
    auto result = m_impl->lua.safe_script_file(
        path.string(), sol::script_pass_on_error);

    if (!result.valid()) {
        const sol::error err = result;
        std::cerr << "[ScriptingEngine] Error in '" << path
                  << "': " << err.what() << '\n';
        return false;
    }
    return true;
}
