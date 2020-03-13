#include "ShaderManager.h"

namespace hierarchy
{

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

// default
ShaderWatcherPtr ShaderManager::get(const std::string &shaderName)
{
    auto found = m_shaderMap.find(shaderName);
    if (found != m_shaderMap.end())
    {
        return found->second;
    }

    auto path = m_path.append(shaderName + ".hlsl");
    auto shader = std::make_shared<ShaderWatcher>(path);
    return shader;
}

} // namespace hierarchy
