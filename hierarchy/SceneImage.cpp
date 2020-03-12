#include "SceneImage.h"

namespace hierarchy
{

std::shared_ptr<SceneImage> SceneImage::Create()
{
    return SceneImagePtr(new SceneImage);
}

} // namespace hierarchy