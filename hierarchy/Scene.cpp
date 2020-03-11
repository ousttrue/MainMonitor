#include "Scene.h"
#include "ParseGltf.h"
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <gltfformat/glb.h>
#include <gltfformat/bin.h>

std::string g_shaderSource =
#include "OpenVRRenderModel.hlsl"
    ;

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
    std::vector<SceneImagePtr> images;
    images.reserve(gltf.images.size());
    for (auto &gltfImage : gltf.images)
    {
        auto &bufferView = gltf.bufferViews[gltfImage.bufferView.value()];
        auto bytes = bin.get_bytes(bufferView);
        auto image = SceneImage::Create(bytes.p, bytes.size);
        images.push_back(image);
    }

    std::vector<SceneMaterialPtr> materials;
    materials.reserve(gltf.materials.size());
    for (auto &gltfMaterial : gltf.materials)
    {
        auto material = SceneMaterial::Create();
        if (gltfMaterial.pbrMetallicRoughness.has_value())
        {
            auto &pbr = gltfMaterial.pbrMetallicRoughness.value();
            if (pbr.baseColorTexture.has_value())
            {
                auto &gltfTexture = gltf.textures[pbr.baseColorTexture.value().index.value()];
                auto image = images[gltfTexture.source.value()];
                material->colorImage = image;
            }

            //material->SetColor(pbr.baseColorFactor.value());
        }
        material->name = gltfMaterial.name;

        material->shader = g_shaderSource;

        materials.push_back(material);
    }

    auto node = SceneNode::Create();
    node->EnableGizmo(true);
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
                else if (k == "NORMAL")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::Normal, ValueType::Float3, p, size);
                }
                else if (k == "TEXCOORD_0")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::TexCoord, ValueType::Float2, p, size);
                }
                else
                {
                    auto a = 0;
                }
            }

            if (gltfPrimitive.material.has_value())
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

                auto material = materials[gltfPrimitive.material.value()];                
                mesh->submeshes.push_back({
                    .draw_offset = 0,
                    .draw_count = (uint32_t)accessor.count.value(),
                    .material = material,
                });
            }
            else
            {
                throw "not indices";
            }
        }
    }
    m_nodes.push_back(node);

    std::cout << "load" << std::endl;
}

} // namespace hierarchy