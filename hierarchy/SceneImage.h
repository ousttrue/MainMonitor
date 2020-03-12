#pragma once
#include <memory>
#include <vector>
#include <stdint.h>

namespace hierarchy
{

enum class ImageType
{
    Unknown,
    Raw,
};

class SceneImage
{
public:
    static std::shared_ptr<SceneImage> Create();

    std::vector<uint8_t> buffer;
    ImageType type = ImageType::Unknown;
    int width = 0;
    int height = 0;

    uint32_t size() const
    {
        return width * height * 4;
    }

    void SetRawBytes(const uint8_t *p, int w, int h)
    {
        type = ImageType::Raw;
        width = w;
        height = h;
        buffer.assign(p, p + w * h);
    }
};
using SceneImagePtr = std::shared_ptr<SceneImage>;

} // namespace hierarchy
