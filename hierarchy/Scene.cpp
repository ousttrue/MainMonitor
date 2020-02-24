#include "Scene.h"
#include "ParseGltf.h"
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <gltfformat/glb.h>
#include <gltfformat/bin.h>

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
    gltfformat::glb glb;
    if (!glb.load(bytes, byteLength))
    {
        return;
    }

    auto gltf = ::ParseGltf(glb.json.p, glb.json.size);

    gltfformat::bin bin(gltf, glb.bin.p, glb.bin.size);

    // build scene
    auto node = SceneNode::Create();
    for (auto &gltfMesh : gltf.meshes)
    {
        for (auto &gltfPrimitive : gltfMesh.primitives)
        {
            auto mesh = SceneMesh::Create();
            node->AddMesh(mesh);

            for (auto [k, v] : gltfPrimitive.attributes)
            {
                if (k == "POSITION")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::Position, ValueType::Float3, p, size);
                }
            }

            {
                auto index = gltfPrimitive.indices.value();
                auto accessor = gltf.accessors[index];
                auto [p, size] = bin.get_bytes(accessor);
                ValueType valueType;
                switch (accessor.componentType.value())
                {
                case gltfformat::AccessorComponentType::UNSIGNED_SHORT:
                case gltfformat::AccessorComponentType::SHORT:
                    valueType = ValueType::UInt16;
                    break;

                case gltfformat::AccessorComponentType::UNSIGNED_INT:
                    valueType = ValueType::UInt32;
                    break;

                default:
                    throw;
                }
                mesh->SetIndices(valueType, p, size);
            }
        }
    }
    m_nodes.push_back(node);

    std::cout << "load" << std::endl;
}

} // namespace hierarchy