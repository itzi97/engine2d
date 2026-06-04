#pragma once
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

// HotReload watches a set of files and invokes a callback when any of them
// is modified on disk. Uses last_write_time polling at a configurable interval.
//
// Usage:
//   HotReload hr;
//   hr.Watch("scripts/scenes/ski.lua", [&]{ scenes.Load("ski"); });
//   // in game loop:
//   hr.Poll();
class HotReload {
public:
  explicit HotReload(float intervalSeconds = 1.0f);

  // Register a file to watch. Callback is called when the file changes.
  void Watch(const std::filesystem::path &path, std::function<void()> callback);

  // Remove all watched files.
  void Clear();

  // Call once per frame. Triggers callbacks for changed files.
  void Poll();

private:
  struct Entry {
    std::filesystem::path     path;
    std::function<void()>     callback;
    std::filesystem::file_time_type lastWrite;
  };

  float                    m_interval;     // seconds between polls
  float                    m_accumulator{0.f};
  std::vector<Entry>       m_entries;
};
