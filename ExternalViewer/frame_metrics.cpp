#include "frame_metrics.h"
#include <array>
#include <algorithm>

thread_local int tl_pos = 60;
thread_local std::array<float, 120> tl_delta;

namespace frame_metrics
{

void new_frame_delta_seconds(float seconds)
{
    tl_delta[tl_pos++] = seconds;
    if (tl_pos >= 120)
    {
        tl_pos = 60;
        std::copy(tl_delta.begin() + 60, tl_delta.begin() + 120, tl_delta.begin());
    }
}

float imgui_plot(void *data, int index)
{
    return tl_delta[tl_pos - 60 + index];
}

void push_internal(const char *section, size_t n)
{
}

void pop()
{
}

} // namespace frame_metrics
