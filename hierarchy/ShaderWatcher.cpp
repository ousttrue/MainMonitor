#include "ShaderWatcher.h"
#include <vector>
#include <stdint.h>
#include <fstream>

static std::string ReadAllText(const std::filesystem::path &path)
{
    std::string result;
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (ifs)
    {
        auto pos = ifs.tellg();
        result.resize(pos);
        ifs.seekg(0, std::ios::beg);
        ifs.read((char *)result.data(), pos);
    }
    return result;
}

namespace hierarchy
{
ShaderWatcher::ShaderWatcher(const std::filesystem::path &path)
    : m_path(path)
{
    // start watch path
    m_source = ReadAllText(m_path);

    // TODO: watch
}

} // namespace hierarchy
