#include "core/HotReload.hpp"
#include <iostream>

HotReload::HotReload(float intervalSeconds)
    : m_interval(intervalSeconds) {}

void HotReload::Watch(const std::filesystem::path &path,
                      std::function<void()> callback) {
  std::filesystem::file_time_type lastWrite{};
  std::error_code ec;
  if (std::filesystem::exists(path, ec))
    lastWrite = std::filesystem::last_write_time(path, ec);

  m_entries.push_back({path, std::move(callback), lastWrite});
}

void HotReload::Clear() {
  m_entries.clear();
  m_accumulator = 0.f;
}

void HotReload::Poll() {
  // dt is not passed in — Poll() is called every frame and we accumulate
  // a fixed wall-clock interval using steady_clock instead.
  using Clock = std::chrono::steady_clock;
  static auto last = Clock::now();
  const auto  now  = Clock::now();
  const float elapsed = std::chrono::duration<float>(now - last).count();

  if (elapsed < m_interval) return;
  last = now;

  for (auto &entry : m_entries) {
    std::error_code ec;
    if (!std::filesystem::exists(entry.path, ec)) continue;
    const auto mtime = std::filesystem::last_write_time(entry.path, ec);
    if (ec) continue;
    if (mtime != entry.lastWrite) {
      entry.lastWrite = mtime;
      std::cout << "[HotReload] Changed: " << entry.path << '\n';
      entry.callback();
    }
  }
}
