#include <cmath>
#include <chrono>
#include <thread>

namespace py = pybind11;
using namespace std::chrono;
using namespace std::chrono_literals;

// class Clock
// {
// public:
//     double get_fps() const
//     {
//         return m_fps;
//     }

//     void tick(double framerate = 0.0)
//     {
//         auto now = steady_clock::now();
//         auto frame_time = now - m_last_frame;
//         m_last_frame = now;

//         ++m_frame_count;
//         m_accumulated_time += frame_time;
//         if(m_accumulated_time >= 1s)
//         {
//             m_fps = m_frame_count / m_accumulated_time.count();
//             m_frame_count = 0;
//             m_accumulated_time = duration_cast<double>(0s);
//         }

//         if(framerate > 0.0)
//         {
//             auto sleep_time = 1.0 / framerate - frame_time;
//             if(sleep_time > 0s) std::this_thread::sleep_for(sleep_time);
//         }
//     }

// private:
//     steady_clock::time_point m_start_time = high_resulution_clock::now();
//     int m_frame_count = 0;
//     double m_fps = 0.0;
// };



class SimulationManager // : private Clock
{
public:
    SimulationManager(py::object game_map) 
        : m_map(game_map)
    {
        auto config = py::module_::import("src.config");
        m_do_temp_exchange = config.attr("TEMP_IS_EXCHANGING").cast<bool>();

        auto materials = py::module_::import("src.materials");
        m_gas_flag = materials.attr("MaterialTags").attr("GAS").cast<long long>();

        auto pygame_time = py::module_::import("pygame.time");
        m_pygame_clock = pygame_time.attr("Clock")();
    }

    void tick(double framerate = 0.0)
    {   
        py::array_t<py::object> view = m_map.attr("get_view")();
        auto data = view.data();

        auto data_idx = [&](int x, int y)
        { 
            auto strides = view.strides(); 
            auto offset = x * strides[0] + y * strides[1];
            return offset / sizeof(py::object);
        };

        // Double-buffered exchanging of temp
        if(m_do_temp_exchange)
        {
            std::vector<double> buf_temp(view.shape(0) * view.shape(1));
            for(int y = 0; y < view.shape(1); ++y)
            {
                for(int x = 0; x < view.shape(0); ++x)
                {
                    py::object cur_dot = data[data_idx(x, y)];
                    double new_temp = cur_dot.attr("temp").cast<double>();
                    double orig_temp = new_temp;
                    double heat_capacity = cur_dot.attr("heat_capacity").cast<double>();
                    for(int rel_x : {-1, 1})
                    for(int rel_y : {-1, 1})
                    {
                        auto nb = data[data_idx(x + rel_x, y + rel_y)];
                        new_temp += (nb.attr("temp").cast<double>() - orig_temp)
                            * nb.attr("thermal_conductivity").cast<double>()
                            * heat_capacity;
                    }
                    buf_temp[y + x * view.shape(1)] = new_temp;
                }
            }
            // Assign calculated temp to the view
            for(int y = 0; y < view.shape(1); ++y)
                for(int x = 0; x < view.shape(0); ++x)
                    data[data_idx(x, y)].attr("temp") = buf_temp[y + x * view.shape(1)];
        }

        // Updating not gases (going upwards)
        for(int y = 0; y < view.shape(1); ++y)
            for(int x = 0; x < view.shape(0); ++x)
                if(0 == (data[data_idx(x, y)].attr("tags").cast<long long>() & m_gas_flag))
                    data[data_idx(x, y)].attr("update")(m_map, x, y);
        
        // Updating gases (going downwards)
        for(int y = view.shape(1) - 1; y >= 0; --y)
            for(int x = 0; x < view.shape(0); ++x)
                if(data[data_idx(x, y)].attr("tags").cast<long long>() & m_gas_flag)
                    data[data_idx(x, y)].attr("update")(m_map, x, y);

        // Clock::tick(framerate);
        m_pygame_clock.attr("tick")(framerate);
    }

    double get_tps()
    {
        // return Clock::get_fps();
        return m_pygame_clock.attr("get_fps")().cast<double>();
    }

private:
    py::object m_map, m_pygame_clock;
    bool m_do_temp_exchange = true;
    long long m_gas_flag{};
};


