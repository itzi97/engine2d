#pragma once

#include <filesystem>
#include <memory>

/// Scripting facade -- sol2 includes are confined to ScriptingEngine.cpp only.
class ScriptingEngine
{
public:
    ScriptingEngine();
    ~ScriptingEngine();

    ScriptingEngine(const ScriptingEngine&)            = delete;
    ScriptingEngine& operator=(const ScriptingEngine&) = delete;

    /// Loads and executes a Lua script. Returns false and logs on error.
    [[nodiscard]] bool RunScript(const std::filesystem::path& path);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
