#pragma once
#include <memory>

namespace hierarchy
{
    
class SceneImage
{
public:
    static std::shared_ptr<SceneImage> Create(const uint8_t *p, int length);
};
using SceneImagePtr = std::shared_ptr<SceneImage>;

} // namespace hierarchy
