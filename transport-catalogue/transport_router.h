#pragma once
#include <memory>
#include <iostream>

#include "transport_catalogue.h"
#include "domain.h"
#include "router.h"

#define MIN_TO_SECONDS 60
#define KM_PER_H_TO_M_PER_S 3.6

namespace transport_router
{
    using namespace std::literals;

    using Graph = graph::DirectedWeightedGraph<double>;

    class TransportRouter
    {
    public:
        using EdgesList = std::vector<std::pair<std::string_view, std::string_view>>;

        TransportRouter(const transport_catalogue::TransportCatalogue &transport_catalogue);

        void SetRoutingSettings(const int bus_wait_time, const double bus_velocity);
        const RoutingSettings &GetRoutingSettings() const;

        void InitializeGraph();

        std::optional<std::vector<RouteData>> CreatRoute(const std::string_view from, const std::string_view to);

        void SetGraph(const Graph &graph);
        std::shared_ptr<Graph> GetGraph() const;

    private:
        const transport_catalogue::TransportCatalogue &transport_catalogue_;
        RoutingSettings settings_;
        Graph graph_;
        std::unique_ptr<graph::Router<double>> router_ = nullptr;

        EdgesList edges_;

        void BuildGraph();

        std::vector<RouteData> CreateAnswer(const std::optional<graph::Router<double>::RouteInfo> &route_info) const;

        RouteData CreateBusAnswer(size_t edge_index, double time) const;
        RouteData CreateStopAnswer(size_t edge_index) const;
        RouteData CreateEmptyAnswer() const;

        template <typename Begin, typename End>
        void CreateEdgesAlongRoute(Graph &graph, const Begin begin, const End end, std::string_view bus_name);
    };

    template <typename Begin, typename End>
    void TransportRouter::CreateEdgesAlongRoute(Graph &graph, const Begin begin, const End end, std::string_view bus_name)
    {
        const double wait = settings_.bus_wait_time * MIN_TO_SECONDS;          // переводим время в секунды
        const double bus_speed = settings_.bus_velocity / KM_PER_H_TO_M_PER_S; // переводим скорость в м/с

        for (auto it_stop = begin; it_stop < end; ++it_stop)
        {
            uint32_t span_count = 1;
            double current_lenght = 0.0;
            for (auto from_stop = it_stop, to_stop = from_stop + 1; to_stop != end; ++from_stop, ++to_stop)
            {
                std::optional<double> lenght = transport_catalogue_.GetStopDistance(*from_stop, *to_stop);
                current_lenght += lenght.value();
                double time_weight = current_lenght / bus_speed + wait;
                graph.AddEdge({(*it_stop)->id, (*to_stop)->id, time_weight, span_count, transport_catalogue_.GetRouteByName(bus_name)->id});
                edges_.push_back({(*it_stop)->name, (*to_stop)->name});
                ++span_count;
            }
        }
    }

} // namespace transport_router
