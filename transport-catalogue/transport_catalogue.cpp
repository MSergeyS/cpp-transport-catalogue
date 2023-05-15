#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_set>

#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue {

// добавление остановки в базу
void TransportCatalogue::AddStop(const std::string &stop_name,
        geo::Coordinates coordinate) {
    Stop stop;
    stop.name = stop_name;
    stop.coordinate = coordinate;
    stops_.push_back(move(stop));
    stops_by_names_.insert({stops_.back().name, &stops_.back()});
}

// добавление маршрута в базу
void TransportCatalogue::AddRoute(string_view name, RouteType type,
        std::vector<std::string_view> stops) {
    Route route;
    route.name = name;
    route.route_type = type;

    for (auto& stop : stops) {
        auto found_stop = GetStopByName(stop);

        if (found_stop != nullptr) {
            route.stops.push_back(found_stop);
        }
    }

    routes_.push_back(move(route));
    routes_by_names_[routes_.back().name] = &routes_.back();

    for (auto& stop : stops) {
        auto found_stop = GetStopByName(stop);
        if (found_stop != nullptr) {
            routes_on_stops_[found_stop].insert(routes_.back().name);
        }
    }
}

const Stop* TransportCatalogue::GetStopByName(
        string_view stop_name) const {
    if (stops_by_names_.count(stop_name) == 0) {
        return nullptr;
    }
    return stops_by_names_.at(stop_name);
}

const Route* TransportCatalogue::GetRouteByName(
        string_view route_name) const {
    if (routes_by_names_.count(route_name) == 0) {
        return nullptr;
    }
    return routes_by_names_.at(route_name);
}

const std::unordered_map<string_view, const Route*>
&TransportCatalogue::GetAllRoutes() const {
    return routes_by_names_;
}

const std::unordered_map<string_view, const Stop*>
&TransportCatalogue::GetAllStops() const {
    return stops_by_names_;
}

int CalculateStops(const Route *route) noexcept {
    int result = 0;
    if (route != nullptr) {
        result = static_cast<int>(route->stops.size());
        if (route->route_type == RouteType::LINEAR) {
            result = result  * 2 - 1;
        }
    }
    return result;
}

int CalculateUniqueStops(const Route *route) noexcept {
    int result = 0;
    if (route != nullptr) {
        unordered_set<string_view> uniques;
        for (auto stop : route->stops) {
            uniques.insert(stop->name);
        }
        result = static_cast<int>(uniques.size());
    }
    return result;
}

double CalculateRouteLength(const Route *route) noexcept {
    double result = 0.0;
    if (route != nullptr) {
        for (auto iter1 = route->stops.begin(), iter2 = iter1+1;
             iter2 < route->stops.end();
             ++iter1, ++iter2) {
            result += ComputeDistance((*iter1)->coordinate, (*iter2)->coordinate);
        }
        if (route->route_type == RouteType::LINEAR) {
            result *= 2;
        }
    }
    return result;
}

RouteInfo TransportCatalogue::GetRouteInfo(
        std::string_view route_name) const {
    RouteInfo result;
    auto route = GetRouteByName(route_name);
    result.name = route->name;
    result.route_type = route->route_type;
    result.number_of_stops = CalculateStops(route);
    result.number_of_unique_stops = CalculateUniqueStops(route);
    result.route_length = this->CalculateRealRouteLength(route);
    // извилистость, то есть отношение фактической длины маршрута к географическому расстоянию
    result.curvature = result.route_length / CalculateRouteLength(route);
    return result;
}

const std::set<std::string_view>
TransportCatalogue::GetRoutesOnStop(const Stop* stop) const {
    auto found = routes_on_stops_.find(stop);
    if (found == routes_on_stops_.end()) {
        return {};
    } else {
        return found->second;
    }
}

void TransportCatalogue::SetStopDistance(const Stop* p_stop1,
        const Stop* p_stop2, uint64_t distance) {
    if (p_stop1 != nullptr && p_stop2 != nullptr) {
        distances_[{ p_stop1, p_stop2 }] = distance;
    }
}

uint64_t TransportCatalogue::GetStopDistance(const Stop*  p_stop1, const Stop*  p_stop2) const {
    if (p_stop1 != nullptr && p_stop2 != nullptr) {
        // Если для двух остановок расстояние было задано только один раз,
        // то оно считается одинаковым в обоих направлениях.
        // Если вы не нашли расстояние от остановки x до остановки y,
        // то поищите расстояние от y до x.
        return (distances_.count({ p_stop1, p_stop2 })) ? 
            distances_.at({ p_stop1, p_stop2 }) : 
            ((distances_.count({ p_stop2, p_stop1 })) ? distances_.at({ p_stop2, p_stop1 }) : 0);
    }
    return 0; // static_cast<uint64_t>(ComputeDistance(p_stop1->coordinate, p_stop2->coordinate));
}

uint64_t  TransportCatalogue::CalculateRealRouteLength(const Route* route) const {
    uint64_t length = 0;
    if (route != nullptr) {
        // проходим по маршруту вперед
        for (size_t n = 0;  n < (route->stops.size() - 1); ++n) {
            const Stop* p_stop1 = route->stops.at(n);
            const Stop* p_stop2 = route->stops.at(n + 1);
            length += this->GetStopDistance(p_stop1, p_stop2);
        }
        // проходим по маршруту назад;
        if (route->route_type == RouteType::LINEAR) {
            for (size_t n = 0; n < (route->stops.size() - 1); ++n) {
                const Stop* p_stop1 = route->stops.at(route->stops.size() - 1 - n);
                const Stop* p_stop2 = route->stops.at(route->stops.size() - 2 - n);
                length += this->GetStopDistance(p_stop1, p_stop2);
            }
        }
    }
    return length;
}

}//namespace transport_catalogue
