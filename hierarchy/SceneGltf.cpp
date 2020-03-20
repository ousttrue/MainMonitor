#include "SceneGltf.h"
#include "ParseGltf.h"
#include "ShaderManager.h"
#include "VertexBuffer.h"
#include "SceneMeshSkin.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <gltfformat/glb.h>
#include <gltfformat/bin.h>
#include <Windows.h>
#include <plog/Log.h>

#define YAP_ENABLE
#define YAP_IMPL //  in one cpp before .h inclusion
// #define YAP_IMGUI // if you want Ocornut's ImGui
#include <YAP.h>

struct GltfVertex
{
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 2> uv;
};
static_assert(sizeof(GltfVertex) == 32, "GltfVertex size");

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

class GltfLoader
{
    const gltfformat::glTF &m_gltf;
    gltfformat::bin m_bin;

    std::vector<SceneImagePtr> m_images;
    std::vector<SceneMaterialPtr> m_materials;
    std::vector<SceneNodePtr> m_nodes;

    struct GltfPrimitive
    {
        SceneMeshPtr mesh;
        std::vector<VertexSkining> skining;
    };
    struct GltfMeshGroup
    {
        std::vector<GltfPrimitive> primitives;
    };
    std::vector<std::shared_ptr<GltfMeshGroup>> m_meshes;

public:
    GltfLoader(const gltfformat::glTF &gltf, const uint8_t *p, int size)
        : m_gltf(gltf), m_bin(gltf, p, size)
    {
    }

    void LoadImages()
    {
        m_images.reserve(m_gltf.images.size());
        for (auto &gltfImage : m_gltf.images)
        {
            auto &bufferView = m_gltf.bufferViews[gltfImage.bufferView.value()];
            auto bytes = m_bin.get_bytes(bufferView);

            // TO_PNG
            auto image = SceneImage::Load(bytes.p, bytes.size);
            image->name = ToUnicode(gltfImage.name, CP_UTF8);
            m_images.push_back(image);
        }
    }

    void LoadMaterials()
    {
        m_materials.reserve(m_gltf.materials.size());
        for (auto &gltfMaterial : m_gltf.materials)
        {
            auto material = SceneMaterial::Create();
            material->shader = ShaderManager::Instance().get("gltf_standard");
            if (gltfMaterial.pbrMetallicRoughness.has_value())
            {
                auto &pbr = gltfMaterial.pbrMetallicRoughness.value();
                if (pbr.baseColorTexture.has_value())
                {
                    auto &gltfTexture = m_gltf.textures[pbr.baseColorTexture.value().index.value()];
                    auto image = m_images[gltfTexture.source.value()];
                    material->colorImage = image;
                }

                //material->SetColor(pbr.baseColorFactor.value());
            }
            material->name = gltfMaterial.name;

            m_materials.push_back(material);
        }
    }

    void LoadNodes()
    {
        int i = 0;
        m_nodes.reserve(m_gltf.nodes.size());
        for (auto &gltfNode : m_gltf.nodes)
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
                node->Local(trs.transform);

                // throw("not implemented");
                auto length = falg::Length(node->Local().rotation);
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
                    node->Local().translation = falg::vector_cast<falg::float3>(gltfNode.translation);
                }
                if (gltfNode.rotation.size() == 4)
                {
                    node->Local().rotation = falg::vector_cast<falg::float4>(gltfNode.rotation);
                    auto length = falg::Length(node->Local().rotation);
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
            m_nodes.push_back(node);
            ++i;
        }
    }

    void LoadMeshes()
    {
        m_meshes.reserve(m_gltf.meshes.size());
        for (auto &gltfMesh : m_gltf.meshes)
        {
            auto group = std::make_shared<GltfMeshGroup>();
            m_meshes.push_back(group);
            for (auto &gltfPrimitive : gltfMesh.primitives)
            {
                auto mesh = SceneMesh::Create();
                mesh->name = ToUnicode(gltfMesh.name, CP_UTF8);
                group->primitives.push_back({});
                auto &primitive = group->primitives.back();
                primitive.mesh = mesh;

                std::vector<GltfVertex> vertices;
                for (auto [k, v] : gltfPrimitive.attributes)
                {
                    auto accessor = m_gltf.accessors[v];
                    auto [p, size] = m_bin.get_bytes(accessor);
                    auto count = accessor.count.value();
                    vertices.resize(count);
                    if (k == "POSITION")
                    {
                        for (auto i = 0; i < count; ++i, p += 12)
                        {
                            vertices[i].position = *(falg::float3 *)p;
                        }
                    }
                    else if (k == "NORMAL")
                    {
                        for (auto i = 0; i < count; ++i, p += 12)
                        {
                            vertices[i].normal = *(falg::float3 *)p;
                        }
                    }
                    else if (k == "TEXCOORD_0")
                    {
                        for (auto i = 0; i < count; ++i, p += 8)
                        {
                            vertices[i].uv = *(falg::float2 *)p;
                        }
                    }
                    else if (k == "JOINTS_0")
                    {
                        primitive.skining.resize(count);
                        switch (accessor.componentType.value())
                        {
                        case gltfformat::AccessorComponentType::BYTE:
                        {
                            for (auto i = 0; i < count; ++i, p += 4)
                            {
                                auto &joints = *(std::array<uint8_t, 4> *)p;
                                primitive.skining[i].joints[0] = joints[0];
                                primitive.skining[i].joints[1] = joints[1];
                                primitive.skining[i].joints[2] = joints[2];
                                primitive.skining[i].joints[3] = joints[3];
                            }
                            break;
                        }

                        case gltfformat::AccessorComponentType::UNSIGNED_SHORT:
                        {
                            for (auto i = 0; i < count; ++i, p += 8)
                            {
                                primitive.skining[i].joints = *(std::array<uint16_t, 4> *)p;
                            }
                        }
                        break;

                        default:
                            throw;
                        }
                    }
                    else if (k == "WEIGHTS_0")
                    {
                        for (auto i = 0; i < count; ++i, p += 16)
                        {
                            primitive.skining[i].weights = *(std::array<float, 4> *)p;
                        }
                    }
                    else if (k == "TANGENT")
                    {
                        // do nothing
                    }
                    else
                    {
                        auto a = 0;
                    }
                }
                mesh->vertices = VertexBuffer::CreateStatic(
                    Semantics::Vertex,
                    sizeof(GltfVertex), vertices.data(), (uint32_t)(vertices.size() * sizeof(GltfVertex)));

                if (gltfPrimitive.material.has_value())
                {
                    auto index = gltfPrimitive.indices.value();
                    auto accessor = m_gltf.accessors[index];
                    auto [p, size] = m_bin.get_bytes(accessor);
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
                    mesh->indices = VertexBuffer::CreateStatic(
                        Semantics::Index, stride, p, size);

                    auto material = m_materials[gltfPrimitive.material.value()];
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

            LOGD << group->primitives[0].mesh->vertices->Count() << "vertices";
        }
    }

    void BuildHierarchy()
    {
        // build node hierarchy
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            auto &gltfNode = m_gltf.nodes[i];
            auto node = m_nodes[i];
            if (gltfNode.mesh.has_value())
            {
                hierarchy::SceneMeshPtr mesh;
                for (auto &primitive : m_meshes[gltfNode.mesh.value()]->primitives)
                {
                    if (!mesh)
                    {
                        mesh = primitive.mesh;
                    }
                    else
                    {
                        mesh->AddSubmesh(primitive.mesh);
                    }
                }

                if (gltfNode.skin.has_value())
                {
                    auto &gltfSkin = m_gltf.skins[gltfNode.skin.value()];
                    auto skin = std::make_shared<SceneMeshSkin>();
                    for (auto j : gltfSkin.joints)
                    {
                        skin->joints.push_back(m_nodes[j]);
                    }
                    if (gltfSkin.inverseBindMatrices.has_value())
                    {
                        auto accessor = m_gltf.accessors[gltfSkin.inverseBindMatrices.value()];
                        auto [p, size] = m_bin.get_bytes(accessor);
                        skin->inverseBindMatrices.assign((std::array<float, 16> *)p, (std::array<float, 16> *)(p + size));
                    }

                    for (auto &primitive : m_meshes[gltfNode.mesh.value()]->primitives)
                    {
                        std::copy(primitive.skining.begin(), primitive.skining.end(), std::back_inserter(skin->vertexSkiningArray));
                    }
                    if (skin->vertexSkiningArray.size() != mesh->vertices->Count())
                    {
                        throw;
                    }

                    if (gltfSkin.skeleton.has_value())
                    {
                        skin->root = m_nodes[gltfSkin.skeleton.value()]->Parent();
                    }

                    mesh->skin = skin;
                }

                node->Mesh(mesh);
            }
            for (auto child : gltfNode.children)
            {
                auto childNode = m_nodes[child];
                node->AddChild(childNode);
            }
        }
    }

    SceneNodePtr CreateRoot()
    {
        auto root = SceneNode::Create("gltf");
        for (auto node : m_nodes)
        {
            if (!node->Parent())
            {
                root->AddChild(node);
            }
        }
        root->UpdateWorld();

        return root;
    }

    SceneNodePtr Load()
    {
        LoadImages();
        LoadMaterials();
        LoadNodes();
        LoadMeshes();
        BuildHierarchy();
        return CreateRoot();
    }
};

SceneNodePtr SceneGltf::LoadGlbBytes(const uint8_t *bytes, int byteLength)
{
    gltfformat::glb glb;
    if (!glb.load(bytes, byteLength))
    {
        return nullptr;
    }

    auto gltf = ::ParseGltf(glb.json.p, glb.json.size);

    GltfLoader loader(gltf, glb.bin.p, glb.bin.size);

    return loader.Load();
}

} // namespace hierarchy
