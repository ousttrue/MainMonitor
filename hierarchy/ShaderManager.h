#pragma once
#include "ShaderWatcher.h"
#include <unordered_map>
#include <filesystem>

namespace hierarchy
{

class ShaderManager
{
    std::filesystem::path m_path;
    std::unordered_map<std::string, ShaderWatcherPtr> m_shaderMap;

    // avoid copy
    ShaderManager(const ShaderManager &) = delete;
    ShaderManager &operator=(const ShaderManager &) = delete;

    ShaderManager() = default;

public:
    // singleton
    static ShaderManager &Instance();

    void setPath(std::filesystem::path &path)
    {
        m_path = path;
    }

    // default
    ShaderWatcherPtr get(const std::string &shaderName);
    ShaderWatcherPtr getDefault()
    {
        return get("default");
    }
};

} // namespace hierarchy