#include "Scene.h"
#include "GltfLoader.h"
#include <vector>
#include <fstream>
#include <string>
#include <iostream>

template <class T>
static std::vector<uint8_t> read_allbytes(T path)
{
    std::vector<uint8_t> buffer;

    // open the file for binary reading
    std::ifstream file;
    file.open(path, std::ios_base::binary);
    if (file.is_open())
    {
        // get the length of the file
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // read the file
        buffer.resize(fileSize);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    }

    return buffer;
}

namespace hierarchy
{
Scene::Scene()
{
}

void Scene::LoadFromPath(const std::string &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        return;
    }

    LoadGlbBytes(bytes.data(), (int)bytes.size());
}

void Scene::LoadFromPath(const std::wstring &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        return;
    }

    LoadGlbBytes(bytes.data(), (int)bytes.size());
}

void Scene::LoadGlbBytes(const uint8_t *bytes, int byteLength)
{
    auto gltf = ::LoadGlb(bytes, byteLength);

    // build scene
    for (auto &mesh : gltf.meshes)
    {
        auto a = 0;
    }

    std::cout << "load" << std::endl;
}

} // namespace hierarchy