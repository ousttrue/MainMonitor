#include "SceneImage.h"

namespace hierarchy
{

std::shared_ptr<SceneImage> SceneImage::Create(const uint8_t *p, int length)
{
    return SceneImagePtr(new SceneImage);
}

} // namespace hierarchy