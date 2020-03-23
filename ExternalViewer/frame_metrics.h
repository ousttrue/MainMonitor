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
struct scoped_RAII
{
    bool moved = false;

    scoped_RAII(const char (&section)[N])
    {
        push(section);
    }
    ~scoped_RAII()
    {
        if (moved)
        {
            auto a = 0;
        }
        else
        {
            pop();
        }
    }

    scoped_RAII(const scoped_RAII &) = delete;
    scoped_RAII &operator=(const scoped_RAII &) = delete;

    scoped_RAII(scoped_RAII &&rhs)
    {
        rhs.moved = true;
    }
    scoped_RAII &operator=(scoped_RAII &&rhs)
    {
        rhs.moved = true;
    }
};

template <size_t N>
scoped_RAII<N> scoped(const char (&section)[N])
{
    return std::move(scoped_RAII<N>(section));
}

} // namespace frame_metrics
