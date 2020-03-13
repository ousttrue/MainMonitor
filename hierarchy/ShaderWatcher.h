#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace hierarchy
{

class ShaderWatcher
{
    std::filesystem::path m_path;
    std::string m_source;
    int m_generation = 1;

public:
    ShaderWatcher(const std::filesystem::path &path);

    std::pair<std::string, int> source() const { return std::make_pair(m_source, m_generation); }
};
using ShaderWatcherPtr = std::shared_ptr<ShaderWatcher>;

} // namespace hierarchy
