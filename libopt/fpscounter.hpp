#ifndef MOOX_FPSCOUNTER_HPP
#define MOOX_FPSCOUNTER_HPP

#include <array>
#include <cstdint>

class FPSCounter
{
public:
    FPSCounter();
    void tick();

    double avg_fps();
    double min_fps();
    double max_fps();

private:
    std::array<uint64_t, 60> m_previous_frame_intervals;
    uint64_t m_previous_frame_time{};
};

#endif // MOOX_FPSCOUNTER_HPP
