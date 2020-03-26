#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace hierarchy
{

class Shader;
class ShaderWatcher
{
    std::string m_name;
    std::string m_source;
    int m_generation = 1;
    std::shared_ptr<Shader> m_compiled;

public:
    ShaderWatcher(const std::string &name);
    const std::string &name() const { return m_name; }
    void source(const std::string &source);
    std::pair<std::string, int> source() const { return std::make_pair(m_source, m_generation); }
    void clear()
    {
        m_source = "";
        m_compiled = nullptr;
    };
    const std::shared_ptr<Shader> &Compiled() const { return m_compiled; }
};
using ShaderWatcherPtr = std::shared_ptr<ShaderWatcher>;

} // namespace hierarchy
