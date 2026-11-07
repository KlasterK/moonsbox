#include "fpscounter.hpp"
#include <algorithm>
#include <numeric>
#include <SDL3/SDL_timer.h>

FPSCounter::FPSCounter()
{
    // 16 ms is near to 60 fps
    // Default set to not make values crazy in the beginning
    std::ranges::fill(m_previous_frame_intervals, 16'666'667);
    m_previous_frame_time = 16'666'667;
}

void FPSCounter::tick()
{
    uint64_t new_frame_time = SDL_GetTicksNS();
    uint64_t new_frame_interval = new_frame_time - m_previous_frame_time;
    m_previous_frame_time = new_frame_time;

    std::ranges::shift_left(m_previous_frame_intervals, 1);
    *m_previous_frame_intervals.rbegin() = new_frame_interval;
}

double FPSCounter::avg_fps()
{
    return 1'000'000'000.0
        * double(m_previous_frame_intervals.size())
        / double(std::accumulate(
            m_previous_frame_intervals.begin(), m_previous_frame_intervals.end(),
            uint64_t(0)
        ));
}

double FPSCounter::min_fps()
{
    return 1'000'000'000.0 / double(std::ranges::max(m_previous_frame_intervals));
}

double FPSCounter::max_fps()
{
    return 1'000'000'000.0 / double(std::ranges::min(m_previous_frame_intervals));
}
