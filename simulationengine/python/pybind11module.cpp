#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <simulationengine/core/gamemap.hpp>
#include <simulationengine/core/materialregistry.hpp>
#include <simulationengine/core/materialdefs.hpp>
#include <simulationengine/core/materialcontroller.hpp>
#include <simulationengine/materials/allmaterials.hpp>
#include <simulationengine/algorithms/drawing.hpp>
#include <simulationengine/algorithms/fastprng.hpp>
#include <simulationengine/algorithms/cksum.hpp>
#include <simulationengine/simulation/simulationmanager.hpp>
#include <simulationengine/serialization/savecontainer.hpp>
#include <simulationengine/serialization/saving.hpp>
#include <string_view>

#ifdef SIMULATIONENGINE_BUILD_MINIZIPSAVECONTAINER
    #include <simulationengine/serialization/minizipsavecontainer.hpp>
#endif

namespace py = pybind11;

struct DotCopy
{
    float temp{}, heat_capacity{}, thermal_conductivity{};
    uint32_t color{};
    MaterialTags tags{};
    MaterialPhysicalBehavior physical_behavior{};
    MaterialController &material_controller;
};

AllMaterialsT<std::tuple> g_materials_tuple{};
MaterialRegistry g_material_registry;

drawing::MaterialFactory _make_pyfunc_material_factory(py::function func)
{
    return [func](size_t x, size_t y)
    {
        py::gil_scoped_acquire acquire;
        func(x, y);
    };
}

PYBIND11_MODULE(pysimulationengine, m)
{
    register_all_materials(g_materials_tuple, g_material_registry);

    py::class_<DotCopy>(m, "DotCopy")
        .def_readwrite("temp", &DotCopy::temp)
        .def_readwrite("heat_capacity", &DotCopy::heat_capacity)
        .def_readwrite("thermal_conductivity", &DotCopy::thermal_conductivity)
        .def_readwrite("color", &DotCopy::color)
        .def_readwrite("tags", &DotCopy::tags)
        .def_readwrite("physical_behavior", &DotCopy::physical_behavior)
        .def_property_readonly(
            "material_controller", 
            [](DotCopy &self) { return self.material_controller; },
            py::return_value_policy::reference
        )
    ;

    py::class_<GameMap>(m, "GameMap")
        .def(py::init([](std::array<size_t, 2> size) 
        { 
            GameMap map(size[0], size[1]);
            drawing::fill(map, drawing::make_controller_init_point_factory(
                map, std::get<Space>(g_materials_tuple))
            );
            return map;
        }))
        .def_property_readonly("size", [](GameMap &map) 
        {
            return py::make_tuple(map.width(), map.height());
        })
        .def("__getitem__", [](GameMap &map, std::array<int, 2> pos) -> py::object
        {
            if(!map.in_bounds(pos[0], pos[1]))
                return py::none();

            return py::cast(DotCopy{
                .temp=map.temps(pos[0], pos[1]),
                .heat_capacity=map.heat_capacities(pos[0], pos[1]),
                .thermal_conductivity=map.thermal_conductivities(pos[0], pos[1]),
                .color=map.colors(pos[0], pos[1]),
                .tags=map.tags(pos[0], pos[1]),
                .physical_behavior=map.physical_behaviors(pos[0], pos[1]),
                .material_controller=*map.material_ctls(pos[0], pos[1]),
            });
        })
        .def("__setitem__", [](GameMap &map, std::array<int, 2> pos, DotCopy copy)
        {
            if(!map.in_bounds(pos[0], pos[1]))
                return;

            map.temps(pos[0], pos[1]) = copy.temp;
            map.heat_capacities(pos[0], pos[1]) = copy.heat_capacity;
            map.thermal_conductivities(pos[0], pos[1]) = copy.thermal_conductivity;
            map.colors(pos[0], pos[1]) = copy.color;
            map.tags(pos[0], pos[1]) = copy.tags;
            map.physical_behaviors(pos[0], pos[1]) = copy.physical_behavior;
            map.material_ctls(pos[0], pos[1]) = &copy.material_controller;
        })
        .def("in_bounds", [](GameMap &map, std::array<int, 2> pos) 
        { 
            return map.in_bounds(pos[0], pos[1]); 
        })
    ;

    py::class_<SimulationManager>(m, "SimulationManager")
        .def(py::init([](GameMap &map)
        {
            return SimulationManager(map, g_material_registry);
        }))
        .def("tick", &SimulationManager::tick)
        .def("is_paused", &SimulationManager::is_paused)
        .def("set_paused", &SimulationManager::set_paused)
    ;

    py::class_<MaterialController>(m, "MaterialController")
        .def("init_point", &MaterialController::init_point)
        .def("static_update", &MaterialController::static_update)
        .def("dynamic_update", &MaterialController::dynamic_update)
        .def("is_placeable_on", &MaterialController::is_placeable_on)
        .def("set_play_sound_callback", [](MaterialController &self, py::function func)
        {
            self.set_play_sound_callback([func](        
                std::string_view name,
                std::optional<std::string_view> category,
                bool do_override
            )
            {
                py::gil_scoped_acquire acquire;
                func(name, category.has_value() ? py::cast(*category) : py::none(), do_override);
            });
        })
        .def("play_place_sound", &MaterialController::play_place_sound)
        .def("serialize", &MaterialController::serialize)
        .def("deserialize", &MaterialController::deserialize)
    ;

    py::class_<SemanticVersion>(m, "SemanticVersion")
        .def_readwrite("major", &SemanticVersion::major)
        .def_readwrite("minor", &SemanticVersion::minor)
        .def_readwrite("patch", &SemanticVersion::patch)
        .def_property(
            "revision",
            [](SemanticVersion &self) { return py::str(std::string_view(&self.revision, 1)); },
            [](SemanticVersion &self, py::object rev) 
            {
                if(rev.is_none())
                {
                    self.revision = '\0';
                    return;
                }
                if(!py::isinstance<py::str>(rev))
                    throw py::value_error("rev is not str or None");
                auto view = py::cast<std::string_view>(rev);
                if(view.empty())
                {
                    self.revision = '\0';
                    return;
                }
                if(view.size() > 1)
                    throw py::value_error("rev length is more than 1");
                self.revision = view[0];
            }
        )
    ;

    m.def_submodule("drawing")
        .def("swap", &drawing::swap)
        .def("fill", [](GameMap &map, py::function func) 
        {
            drawing::fill(map, _make_pyfunc_material_factory(func)); 
        })
        .def("rect", [](GameMap &map, drawing::Rect area, py::function func) 
        {
            drawing::rect(map, area, _make_pyfunc_material_factory(func)); 
        })
        .def("ellipse", [](GameMap &map, drawing::Rect area, py::function func) 
        {
            drawing::ellipse(map, area, _make_pyfunc_material_factory(func)); 
        })
        .def("line", [](
            GameMap &map,
            drawing::SignedPoint begin,
            drawing::SignedPoint end,
            int width,
            py::function func,
            std::string_view line_ends
        )
        {
            drawing::LineEnds ends;
            if(line_ends == "square")
                ends = drawing::LineEnds::Square;
            else if(line_ends == "round")
                ends = drawing::LineEnds::Round;
            else // ends == "none" or anything else
                ends = drawing::LineEnds::None;

            drawing::line(map, begin, end, width, _make_pyfunc_material_factory(func), ends); 
        })
    ;

    m.def_submodule("saving")
        .def("serialize", [](WriteSaveContainer &container, GameMap &map)
        {
            auto result = saving::serialize(container, map, g_material_registry);
            if(!result)
                throw std::runtime_error(result.error());
        })
        .def("deserialize", [](ReadSaveContainer &container)
        {
            auto result = saving::deserialize(container, g_material_registry);
            if(!result)
                throw std::runtime_error(result.error());
            return std::move(result.value());
        })
    ;

    py::class_<ReadSaveContainer>(m, "ReadSaveContainer")
        .def("__adasdsadsadadsadasdsa", []{})
    ;
    py::class_<WriteSaveContainer>(m, "WriteSaveContainer")
        .def("close", &WriteSaveContainer::close)
    ;

    #ifdef SIMULATIONENGINE_BUILD_MINIZIPSAVECONTAINER
        py::class_<MinizipReadSaveContainer, ReadSaveContainer>(m, "MinizipReadSaveContainer")
            .def(py::init<std::filesystem::path>())
        ;

        py::class_<MinizipWriteSaveContainer, WriteSaveContainer>(m, "MinizipWriteSaveContainer")
            .def(py::init<std::filesystem::path>())
        ;
    #endif
}
