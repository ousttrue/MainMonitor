#include "ShaderWatcher.h"
#include <vector>
#include <stdint.h>
#include <fstream>
#include "Shader.h"

namespace hierarchy
{
ShaderWatcher::ShaderWatcher(const std::string &name)
    : m_name(name), m_compiled(new Shader(name))
{
}

void ShaderWatcher::source(const std::string &source)
{
    if (m_source == source)
    {
        return;
    }

    m_source = source;
    m_generation++;

    m_compiled->Initialize(nullptr, source, m_generation);
}

} // namespace hierarchy
