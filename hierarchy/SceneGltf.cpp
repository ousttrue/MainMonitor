#include "SceneGltf.h"
#include "ParseGltf.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <gltfformat/glb.h>
#include <gltfformat/bin.h>
#include <Windows.h>

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

std::wstring ToUnicode(std::string const &src, UINT CP)
{
    auto const dest_size = ::MultiByteToWideChar(CP, 0U, src.data(), -1, nullptr, 0U);
    std::vector<wchar_t> dest(dest_size, L'\0');
    if (::MultiByteToWideChar(CP, 0U, src.data(), -1, dest.data(), (UINT)dest.size()) == 0)
    {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    dest.resize(std::char_traits<wchar_t>::length(dest.data()));
    dest.shrink_to_fit();
    return std::wstring(dest.begin(), dest.end());
}

namespace hierarchy
{

SceneNodePtr SceneGltf::LoadFromPath(const std::string &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        return nullptr;
    }

    return LoadGlbBytes(bytes.data(), (int)bytes.size());
}

SceneNodePtr SceneGltf::LoadFromPath(const std::wstring &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        return nullptr;
    }

    return LoadGlbBytes(bytes.data(), (int)bytes.size());
}

SceneNodePtr SceneGltf::LoadGlbBytes(const uint8_t *bytes, int byteLength)
{
    gltfformat::glb glb;
    if (!glb.load(bytes, byteLength))
    {
        return nullptr;
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

        // TO_PNG
        auto image = SceneImage::Load(bytes.p, bytes.size);
        image->name = ToUnicode(gltfImage.name, CP_UTF8);
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

        materials.push_back(material);
    }

    std::vector<SceneNodePtr> nodes;
    int i = 0;
    for (auto &gltfNode : gltf.nodes)
    {
        auto name = gltfNode.name;
        if (name.empty())
        {
            std::stringstream ss;
            ss << "node#" << i;
            name = ss.str();
        }
        auto node = SceneNode::Create(name);

        if (gltfNode.matrix.size() == 16)
        {
            auto trs = falg::RowMatrixDecompose(falg::vector_cast<falg::float16>(gltfNode.matrix));
            node->Local.translation = trs.translation;
            node->Local.rotation = trs.rotation; // ?

            // throw("not implemented");
            auto length = falg::Length(node->Local.rotation);
            auto delta = abs(1 - length);
            if (delta > 1e-5f)
            {
                throw;
            }
        }
        else
        {
            if (gltfNode.translation.size() == 3)
            {
                node->Local.translation = falg::vector_cast<falg::float3>(gltfNode.translation);
            }
            if (gltfNode.rotation.size() == 4)
            {
                node->Local.rotation = falg::vector_cast<falg::float4>(gltfNode.rotation);
                auto length = falg::Length(node->Local.rotation);
                auto delta = abs(1 - length);
                if (delta > 1e-5f)
                {
                    throw;
                }
            }
            if (gltfNode.scale.size() == 3)
            {
                // node->TRS.scale = falg::vector_cast<falg::float3>(gltfNode.scale);
                // throw("not implemented");
            }
        }
        // node->EnableGizmo(true);
        nodes.push_back(node);
        ++i;
    }

    struct GltfMeshGroup
    {
        std::vector<SceneMeshPtr> primitives;
    };
    std::vector<std::shared_ptr<GltfMeshGroup>> groups;
    for (auto &gltfMesh : gltf.meshes)
    {
        auto group = std::make_shared<GltfMeshGroup>();
        groups.push_back(group);
        for (auto &gltfPrimitive : gltfMesh.primitives)
        {
            auto mesh = SceneMesh::Create();
            mesh->name = ToUnicode(gltfMesh.name, CP_UTF8);
            group->primitives.push_back(mesh);

            for (auto [k, v] : gltfPrimitive.attributes)
            {
                if (k == "POSITION")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::Position, 12, p, size);
                }
                else if (k == "NORMAL")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::Normal, 12, p, size);
                }
                else if (k == "TEXCOORD_0")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::TexCoord, 8, p, size);
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
                int stride = 0;
                switch (accessor.componentType.value())
                {
                case gltfformat::AccessorComponentType::UNSIGNED_SHORT:
                case gltfformat::AccessorComponentType::SHORT:
                    stride = 2;
                    break;

                case gltfformat::AccessorComponentType::UNSIGNED_INT:
                    stride = 4;
                    break;

                default:
                    throw;
                }
                mesh->SetIndices(stride, p, size);

                auto material = materials[gltfPrimitive.material.value()];
                mesh->submeshes.push_back({
                    // .draw_offset = 0,
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

    // build node hierarchy
    for (int i = 0; i < nodes.size(); ++i)
    {
        auto &gltfNode = gltf.nodes[i];
        auto node = nodes[i];
        if (gltfNode.mesh.has_value())
        {
            hierarchy::SceneMeshPtr mesh;
            for (auto primitive : groups[gltfNode.mesh.value()]->primitives)
            {
                if (!mesh)
                {
                    mesh = primitive;
                }
                else
                {
                    mesh->AddSubmesh(primitive);
                }
            }
            node->Mesh(mesh);

            if (gltfNode.skin.has_value())
            {
                auto &gltfSkin = gltf.skins[gltfNode.skin.value()];
                auto a = 0;
            }
        }
        for (auto child : gltfNode.children)
        {
            auto childNode = nodes[child];
            node->AddChild(childNode);
        }
    }

    auto root = SceneNode::Create("gltf");
    for (auto node : nodes)
    {
        if (!node->Parent())
        {
            root->AddChild(node);
        }
    }
    return root;
}

} // namespace hierarchy
