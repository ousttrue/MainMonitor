#pragma once
#include "SceneImage.h"
#include "SceneMaterial.h"
#include "SceneMesh.h"
#include "SceneMeshSkin.h"
#include "SceneNode.h"
#include <vector>

namespace hierarchy
{

struct SceneModel
{
    std::string name;
    std::vector<SceneImagePtr> images;
    std::vector<SceneMaterialPtr> materials;
    std::vector<SceneMeshPtr> meshes;
    std::vector<SceneMeshSkinPtr> skins;
    std::vector<SceneNodePtr> nodes;

    SceneNodePtr root;

    static std::shared_ptr<SceneModel> LoadFromPath(const std::filesystem::path &path);
    static std::shared_ptr<SceneModel> LoadGlbBytes(const uint8_t *p, int size);
};
using SceneModelPtr = std::shared_ptr<SceneModel>;

} // namespace hierarchy
