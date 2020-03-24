#pragma once

namespace frame_metrics
{

void new_frame();
float imgui_plot(void *data, int index);

void push_internal(const char *section, size_t n);

template <size_t N>
void push(const char (&section)[N])
{
    push_internal(section, N);
}
void pop();

template <size_t N>
struct scoped
{
    scoped(const char (&section)[N])
    {
        push(section);
    }
    ~scoped()
    {
        pop();
    }

    scoped(const scoped &) = delete;
    scoped &operator=(const scoped &) = delete;
};

struct section
{
    int parent;
    const char *name;
    float start;
    float end;
};
const section *get_sections(int *count);

} // namespace frame_metrics
