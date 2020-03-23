#include "frame_metrics.h"
#include <array>
#include <algorithm>
#include <Windows.h>

class Metrics
{
    int m_pos = 60;
    std::array<float, 120> m_delta;

    LARGE_INTEGER m_freq;
    float m_freqInv;
    LARGE_INTEGER m_last{};

public:
    Metrics()
    {
        QueryPerformanceFrequency(&m_freq);
        m_freqInv = 1.0f / m_freq.QuadPart;
    }

    void new_frame()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (m_last.QuadPart)
        {
            auto delta = now.QuadPart - m_last.QuadPart;
            m_delta[m_pos++] = delta * m_freqInv;
            if (m_pos >= 120)
            {
                m_pos = 60;
                std::copy(m_delta.begin() + 60, m_delta.begin() + 120, m_delta.begin());
            }
        }
        m_last = now;
    }

    float histroy(int index)
    {
        return m_delta[m_pos - 60 + index];
    }
};
thread_local Metrics tl_metrics;

namespace frame_metrics
{

void new_frame()
{
    tl_metrics.new_frame();
}

float imgui_plot(void *data, int index)
{
    return tl_metrics.histroy(index);
}

void push_internal(const char *section, size_t n)
{
}

void pop()
{
}

} // namespace frame_metrics
