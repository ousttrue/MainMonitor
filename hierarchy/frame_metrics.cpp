#include "frame_metrics.h"
#include <array>
#include <algorithm>
#include <Windows.h>
#include <vector>
#include <assert.h>

namespace frame_metrics
{

struct FrameMetric
{
    float duration;
    std::vector<section> sections;
};

class Metrics
{
    int m_pos = 0;
    std::array<FrameMetric, 60> m_delta;
    std::vector<int> m_stack;

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
            m_delta[m_pos++].duration = delta * m_freqInv;
            if (m_pos >= 60)
            {
                m_pos -= 60;
                assert(m_pos == 0);
            }
            m_delta[m_pos].sections.clear();
            m_stack.clear();
        }
        m_last = now;
    }

    float histroy(int index)
    {
        auto wrapped = (m_pos - 60 + index);
        while (wrapped < 0)
        {
            wrapped += 60;
        }

        return m_delta[wrapped].duration;
    }

    float now()
    {
        LARGE_INTEGER value;
        QueryPerformanceCounter(&value);
        auto delta = value.QuadPart - m_last.QuadPart;
        return delta * m_freqInv;
    }

    ///
    /// profiler
    ///
    void push(const char *section)
    {
        auto index = (int)m_delta[m_pos].sections.size();
        m_delta[m_pos].sections.push_back({
            .parent = m_stack.empty() ? -1 : m_stack.back(),
            .name = section,
            .start = now(),
        });
        m_stack.push_back(index);
    }

    void pop()
    {
        auto index = m_stack.back();
        m_stack.pop_back();
        m_delta[m_pos].sections[index].end = now();
    }

    const section *get_sections(int *count)
    {
        auto pos = m_pos == 0 ? 59 : m_pos - 1;
        auto &frame = m_delta[pos];
        *count = (int)frame.sections.size();
        return frame.sections.data();
    }
};
thread_local Metrics tl_metrics;

void new_frame()
{
    tl_metrics.new_frame();
}

float imgui_plot(void *data, int index)
{
    return tl_metrics.histroy(index);
}

void push_internal(const char *section, size_t)
{
    tl_metrics.push(section);
}

void pop()
{
    tl_metrics.pop();
}

const section *get_sections(int *count)
{
    return tl_metrics.get_sections(count);
}

} // namespace frame_metrics
