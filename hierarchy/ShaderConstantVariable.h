#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include <d3d12.h>
#include <d3dcompiler.h>

namespace hierarchy
{

enum class ConstantSemantics
{
    UNKNOWN,

    RENDERTARGET_SIZE,
    CAMERA_VIEW,
    CAMERA_PROJECTION,
    CAMERA_POSITION,
    CAMERA_FOVY,
    LIGHT_DIRECTION,
    LIGHT_COLOR,
    NODE_WORLD,
};

struct ConstantVariable
{
    std::string Name;
    ConstantSemantics Semantic;
    uint32_t Offset;
    uint32_t Size;

    void GetSemantic(const std::string &src);
};

struct ConstantBuffer
{
    uint32_t reg = (uint32_t)-1;
    std::vector<ConstantVariable> Variables;

    void GetVariables(ID3D12ShaderReflection *pReflection,
                      ID3D12ShaderReflectionConstantBuffer *cb,
                      const std::string &source);

    UINT End() const
    {
        auto end = Variables.back().Offset + Variables.back().Size;
        // 256 alignment
        auto mult = end / 256;
        if (end % 256)
        {
            ++mult;
        }
        return 256 * mult;
    }
};

} // namespace hierarchy
