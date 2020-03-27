#include "DrawList.h"
#include "SceneNode.h"
#include "SceneMesh.h"
#include "SceneMeshSkin.h"
#include "VertexBuffer.h"

namespace hierarchy
{

std::pair<uint32_t, uint32_t> DrawList::PushCB(const ConstantBuffer *cb, const CBValue *value, int count)
{
    auto offset = (uint32_t)CB.size();
    if (cb)
    {
        CBRanges.push_back({offset, cb->End()});
        CB.resize(CB.size() + CBRanges.back().second);
        auto p = CB.data() + offset;
        for (int i = 0; i < count; ++i, ++value)
        {
            for (auto &var : cb->Variables)
            {
                if (var.Semantic == value->semantic)
                {
                    // copy value
                    memcpy(p + var.Offset, value->p, value->size);
                    break;
                }
            }
        }
    }
    else
    {
        CBRanges.push_back({offset, 256});
    }

    return CBRanges.back();
} // namespace hierarchy

} // namespace hierarchy
