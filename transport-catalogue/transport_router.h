#pragma once

#include "transport_catalogue.h"
#include "domain.h"
#include "router.h"
#include <memory>

#include <iostream>

namespace transport_router
{
    using namespace std::literals;

    struct EdgeData {
        double time_weight = 0.0;
        int span_count = 0;
        std::string_view bus_name = {};
    };

    class TransportRouter {
    public:
        using Graph = graph::DirectedWeightedGraph<double>;
        using EdgesList = std::vector<std::pair<std::string_view, std::string_view>>;
        using EdgesInfo = std::vector<EdgeData>;

        TransportRouter(const transport_catalogue::TransportCatalogue& transport_catalogue);

        void SetRoutingSettings(const int bus_wait_time, const double bus_velocity);

        void InitializeGraph();

        std::optional<std::vector<RouteData>> CreatRoute(const std::string_view from, const std::string_view to);

    private:

        const transport_catalogue::TransportCatalogue& transport_catalogue_;
        RoutingSettings settings_;
        Graph graph_;
        std::unique_ptr<graph::Router<double>> router_ = nullptr;

        EdgesList edges_;
        EdgesInfo edges_info_;

        void BuildGraph();

        std::vector<RouteData> CreateAnswer(const std::optional<graph::Router<double>::RouteInfo>& route_info) const;

        RouteData CreateBusAnswer(size_t edge_index, double time) const;
        RouteData CreateStopAnswer(size_t edge_index) const;
        RouteData CreateEmptyAnswer() const;

        template <typename Begin, typename End>
        void CreateEdgesAlongRoute(Graph& graph, const Begin begin, const End end, std::string_view bus_name);

    };

    template <typename Begin, typename End>
    void TransportRouter::CreateEdgesAlongRoute(Graph& graph, const Begin begin, const End end, std::string_view bus_name) {
        const double wait = settings_.bus_wait_time * 60.0; // переводим время в секунды
        const double bus_speed = settings_.bus_velocity / 3.6; // переводим скорость в м/с

        for (auto it_stop = begin; it_stop < end; ++it_stop) {
            int span_count = 1;
            double current_lenght = 0.0;
            for (auto from_stop = it_stop, to_stop = from_stop + 1; to_stop != end; ++from_stop, ++to_stop) {
                std::optional<double> lenght = transport_catalogue_.GetStopDistance(*from_stop, *to_stop);
                current_lenght += lenght.value();
                double time_weight = current_lenght / bus_speed + wait;
                graph.AddEdge({ (*it_stop)->id, (*to_stop)->id, time_weight });
                edges_.push_back({ (*it_stop)->name, (*to_stop)->name });
                edges_info_.push_back({ time_weight, span_count, bus_name });
                ++span_count;
            }
        }
    }

} // namespace transport_router
