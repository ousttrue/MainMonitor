#include "GltfLoader.h"
#include <gltfformat/glb.h>
#include <gltfformat/gltf_nlohmann_json.h>

gltfformat::glTF LoadGlb(const uint8_t *p, int size)
{
    gltfformat::glTF gltf;
    gltfformat::glb glb;
    if (glb.load(p, size))
    {
        auto json = nlohmann::json::parse(glb.json.begin(), glb.json.end());
        gltf = json;
    }

    return gltf;
}
