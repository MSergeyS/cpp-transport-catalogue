#include "transport_router.h"

/*
Задача поиска оптимального маршрута данного вида сводится к задаче поиска
кратчайшего пути во взвешенном ориентированном графе.
предоставляются две небольшие библиотеки:

    graph.h — класс, реализующий взвешенный ориентированный граф,
    router.h — класс, реализующий поиск кратчайшего пути во взвешенном ориентированном графе.

О классах дополнительно известно следующее:

    - Вершины и рёбра графа нумеруются автоинкрементно беззнаковыми целыми числами,
      хранящимися в типах VertexId и EdgeId: вершины нумеруются от нуля до количества вершин минус один
      в соответствии с пользовательской логикой. Номер очередного ребра выдаётся методом AddEdge;
      он равен нулю для первого вызова метода и при каждом следующем вызове увеличивается на единицу.
    - Память, нужная для хранения графа, линейна относительно суммы количеств вершин и рёбер.
    - Конструктор и деструктор графа имеют линейную сложность, а остальные методы константны или
      амортизированно константны.
    - Маршрутизатор — класс Router — требует квадратичного относительно количества вершин объёма памяти,
      не считая памяти, требуемой для хранения кэша маршрутов.
    - Конструктор маршрутизатора имеет сложность O(V^3+E), где V — количество вершин графа,
      E — количество рёбер.
    - Маршрутизатор не работает с графами, имеющими рёбра отрицательного веса.
    - Построение маршрута на готовом маршрутизаторе линейно относительно количества рёбер в маршруте.
      Таким образом, основная нагрузка построения оптимальных путей ложится на конструктор.
*/

namespace transport_router
{
    // TransportRouter -----------------------------------------------------------------------------
    TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& transport_catalogue) :
        transport_catalogue_(transport_catalogue) {}

    void TransportRouter::SetRoutingSettings(const int bus_wait_time, const double bus_velocity)
    {
        settings_.bus_wait_time = bus_wait_time;
        settings_.bus_velocity = bus_velocity;
    }

    void TransportRouter::InitializeGraph() {
        BuildGraph();
        router_ = std::make_unique<graph::Router<double>>(graph_);
    }

    std::optional<std::vector<RouteData>> TransportRouter::CreatRoute(const std::string_view from, const std::string_view to) {
        std::optional<graph::Router<double>::RouteInfo> route_info = router_->BuildRoute(
                transport_catalogue_.GetStopByName(from)->id, transport_catalogue_.GetStopByName(to)->id);

        if (route_info == std::nullopt) {
            return std::nullopt;
        }
        if (route_info.value().edges.size() == 0) {
            std::vector<RouteData> route_data;
            route_data.push_back(std::move(CreateEmptyAnswer()));
            return route_data;
        }
        return CreateAnswer(route_info);
    }

    void TransportRouter::BuildGraph() {
        Graph graph(transport_catalogue_.GetAllStops().size());
        const std::unordered_map <std::string_view, const Route*>& all_buses = transport_catalogue_.GetAllRoutes();
        for (const auto& [bus_name, struct_bus] : all_buses) {
            CreateEdgesAlongRoute(graph, struct_bus->stops.begin(), struct_bus->stops.end(), bus_name);
            if (struct_bus->route_type == RouteType::LINEAR) {
                CreateEdgesAlongRoute(graph, struct_bus->stops.rbegin(), struct_bus->stops.rend(), bus_name);
            }
        }
        graph_ = std::move(graph);
    }

    std::vector<RouteData> TransportRouter::CreateAnswer(const std::optional<graph::Router<double>::RouteInfo>& route_info) const {
        double total_time = 0.0;
        std::vector<RouteData> route_data;

        for (size_t edge_index : route_info.value().edges) {
            route_data.push_back(std::move(CreateStopAnswer(edge_index)));
            total_time += settings_.bus_wait_time;

            if (edges_[edge_index].first == edges_[edge_index].second) {
                continue;
            }

            double time = (edges_info_[edge_index].time_weight - settings_.bus_wait_time * MIN_TO_SECONDS) / MIN_TO_SECONDS;
            total_time += time;

            route_data.push_back(std::move(CreateBusAnswer(edge_index, time)));
        }
        return route_data;
    }

    RouteData TransportRouter::CreateBusAnswer(size_t edge_index, double time) const {
        RouteData bus_answer;
        bus_answer.type = "bus"sv;
        bus_answer.bus_name = edges_info_[edge_index].bus_name;
        bus_answer.span_count = edges_info_[edge_index].span_count;
        bus_answer.motion_time = time;
        return bus_answer;
    }

    RouteData TransportRouter::CreateStopAnswer(size_t edge_index) const {
        RouteData stop_answer;
        stop_answer.type = "stop"sv;
        stop_answer.stop_name = edges_[edge_index].first;
        stop_answer.bus_wait_time = settings_.bus_wait_time;
        return stop_answer;
    }

    RouteData TransportRouter::CreateEmptyAnswer() const {
        RouteData empty_answer;
        empty_answer.type = "stay_here"sv;
        return empty_answer;
    }

} // namespace transport_router
