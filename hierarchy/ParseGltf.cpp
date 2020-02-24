#include "ParseGltf.h"
#include <gltfformat/glb.h>
#include <gltfformat/gltf_nlohmann_json.h>

gltfformat::glTF ParseGltf(const uint8_t *p, int size)
{
    // parse
    auto json = nlohmann::json::parse(p, p + size);
    gltfformat::glTF gltf;
    // deserialize
    gltf = json;
    return gltf;
}
