#include "ShaderConstantVariable.h"

namespace hierarchy
{

static bool IsMatch(const std::string &src, const std::string &name, ConstantSemantics semantic)
{
    auto pos = src.find(name);
    if (pos == std::string::npos)
    {
        // not found
        return false;
    }

    auto tail = src[pos + name.size()];
    if (
        (tail >= 'a' && tail <= 'z')    // a-z
        || (tail >= 'A' && tail <= 'Z') // A-Z
        || (tail >= '0' && tail <= '9') // 0-9
        || tail == '_')
    {
        // continuous
        return false;
    }

    return true;
}

#define MATCH(Symbol)                                     \
    if (IsMatch(src, #Symbol, ConstantSemantics::Symbol)) \
    {                                                     \
        return ConstantSemantics::Symbol;                 \
    }

static ConstantSemantics GetSemanticAfterColon(const std::string &src)
{
    MATCH(RENDERTARGET_SIZE);
    MATCH(CAMERA_VIEW);
    MATCH(CAMERA_PROJECTION);
    MATCH(CAMERA_POSITION);
    MATCH(CAMERA_FOVY);
    MATCH(LIGHT_DIRECTION);
    MATCH(LIGHT_COLOR);
    MATCH(NODE_WORLD);

    return ConstantSemantics::UNKNOWN;
}

#undef MATCH

static ConstantSemantics GetSemanticAfterName(const std::string &src)
{
    // search :
    for (auto it = src.begin(); it != src.end(); ++it)
    {
        if (*it == ';')
        {
            return ConstantSemantics::UNKNOWN;
        }

        if (*it == ':')
        {
            auto start = it;
            ++start;
            for (; start != src.end() && *start == ' '; ++start)
            {
            }

            auto end = start;
            ++end;
            for (; end != src.end() && *end != ';'; ++end)
            {
            }

            return GetSemanticAfterColon(std::string(start, end));
        }
    }

    return ConstantSemantics::UNKNOWN;
}

void ConstantVariable::GetSemantic(const std::string &src)
{
    auto found = src.find(Name);
    if (found == std::string::npos)
    {
        return;
    }

    Semantic = GetSemanticAfterName(src.substr(found + Name.size()));
}

void ConstantBuffer::GetVariables(ID3D12ShaderReflection *pReflection,
                                  ID3D12ShaderReflectionConstantBuffer *cb,
                                  const std::string &source)
{
    D3D12_SHADER_BUFFER_DESC cbDesc;
    cb->GetDesc(&cbDesc);
    for (unsigned j = 0; j < cbDesc.Variables; ++j)
    {
        auto cbVariable = cb->GetVariableByIndex(j);
        D3D12_SHADER_VARIABLE_DESC variableDesc;
        cbVariable->GetDesc(&variableDesc);
        Variables.push_back(ConstantVariable{
            .Name = variableDesc.Name,
            .Offset = variableDesc.StartOffset,
            .Size = variableDesc.Size,
        });
        Variables.back().GetSemantic(source);
    }

    D3D12_SHADER_INPUT_BIND_DESC bindDesc;
    auto binding = pReflection->GetResourceBindingDescByName(cbDesc.Name, &bindDesc);
    reg = bindDesc.BindPoint;
}

} // namespace hierarchy
