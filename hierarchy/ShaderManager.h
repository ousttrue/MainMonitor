#pragma once
#include "ShaderWatcher.h"
#include <unordered_map>
#include <filesystem>
#include <mutex>

namespace hierarchy
{

class ShaderManager
{
    std::unordered_map<std::wstring, ShaderWatcherPtr> m_shaderMap;
    std::mutex m_mutex;

    class DirectoryWatcher *m_watcher = nullptr;

    // avoid copy
    ShaderManager(const ShaderManager &) = delete;
    ShaderManager &operator=(const ShaderManager &) = delete;

    ShaderManager();
    ~ShaderManager();

public:
    // singleton
    static ShaderManager &Instance();

    void watch(std::filesystem::path &path);
    void stop();

    // default
    ShaderWatcherPtr get(const std::string &shaderName);
    ShaderWatcherPtr getDefault()
    {
        return get("default");
    }

    void onFile(const std::wstring &fileName, int action);
};

} // namespace hierarchy
