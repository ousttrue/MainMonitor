#pragma once

namespace frame_metrics
{

void new_frame_delta_seconds(float seconds);
float imgui_plot(void *data, int index);

void push(const char *section);
void pop();

struct scoped
{
    scoped(const char *section)
    {
        push(section);
    }
    ~scoped()
    {
        pop();
    }
};

} // namespace frame_metrics
