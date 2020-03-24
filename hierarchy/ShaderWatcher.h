#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace hierarchy
{

class ShaderWatcher
{
    std::string m_name;
    std::string m_source;
    int m_generation = 1;

public:
    ShaderWatcher(const std::string &name);
    const std::string &name() const { return m_name; }
    void source(const std::string &source)
    {
        if (m_source == source)
        {
            return;
        }
        m_source = source;
        m_generation++;
    }
    std::pair<std::string, int> source() const { return std::make_pair(m_source, m_generation); }
    void clear() { m_source = ""; };
};
using ShaderWatcherPtr = std::shared_ptr<ShaderWatcher>;

} // namespace hierarchy
