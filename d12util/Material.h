#include "Helper.h"

namespace d12u
{

class Shader;
class Material : NonCopyable
{

public:
    std::shared_ptr<Shader> m_shader;

};

} // namespace d12u
