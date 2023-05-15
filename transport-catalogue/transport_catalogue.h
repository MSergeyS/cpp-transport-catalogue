#pragma once

#include <deque>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

// типы данных -------------------------------------------------------------
// тип маршрута
enum class RouteType {
    UNKNOWN,
    LINEAR,  // линейный
    CIRCLE,  // кольцевой
};

template<typename Type>
class StopHasher {
public:
    size_t operator()(std::pair<const Type*, const Type*> stop) const {
        return hasher_(stop.first) + 37 * hasher_(stop.second);
    }

private:
    std::hash<const Type*> hasher_;
};

// остановка состоит из имени и координат. Считаем, что имена уникальны.
struct Stop {
    std::string name;  // название остановки
    geo::Coordinates coordinate; // координаты
    friend bool operator==(const Stop &lhs, const Stop &rhs);
};

// Маршрут состоит из имени (номера автобуса), типа и списка остановок. Считаем, что имена уникальны.
struct Route {
    std::string name;  // название маршрута
    RouteType route_type = RouteType::UNKNOWN;
    std::vector<const Stop*> stops; // указатели на остановки
    friend bool operator==(const Route &lhs, const Route &rhs);
};

// информация о маршруте
struct RouteInfo {
    std::string_view name;  // название маршрута
    RouteType route_type = RouteType::UNKNOWN; // тип маршрута
    int number_of_stops = 0; // количество остановок (все)
    int number_of_unique_stops = 0; //количество остановок (уникальные)
    uint64_t route_length = 0; // длина маршрута
    double curvature = 0.0; // извилистость
};

// TransportCatalogue - класс транспортного справочника
class TransportCatalogue {

public:
    // конструкторы ------------------------------------------------------------
    TransportCatalogue() = default;

    // методы ------------------------------------------------------------------

    // добавление остановки в базу
    void AddStop(const std::string &stop_name, geo::Coordinates coordinate);
    // добавление маршрута в базу
    void AddRoute(std::string_view number, RouteType type, std::vector<std::string_view> stops);

    // поиск остановки по имени
    const Stop* GetStopByName(std::string_view stop_name) const;
    // поиск маршрута по имени
    const Route* GetRouteByName(std::string_view route_name) const;

    // поиск остановки по имени
    const std::unordered_map<std::string_view, const Stop*>& GetAllStops() const;
    // поиск маршрута по имени
    const std::unordered_map<std::string_view, const Route*>& GetAllRoutes() const;

    // получение информации о маршруте
    RouteInfo GetRouteInfo(std::string_view route_name) const;

    // возвращает список автобусов, проходящих через остановку
    const std::set<std::string_view> GetRoutesOnStop(const Stop* stop) const;

    // задаёт дистанцию между остановками p_stop1 и p_stop2
    void SetStopDistance(const Stop* p_stop1, const Stop* p_stop2, uint64_t distance);

    // получение дистанцию между остановками p_stop1 и p_stop2
    uint64_t GetStopDistance(const Stop* p_stop1, const Stop* p_stop2) const;

private:
    // типы данных -------------------------------------------------------
    // остановки
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stops_by_names_;
    // маршруты
    std::deque<Route> routes_;
    std::unordered_map<std::string_view, const Route*> routes_by_names_;
    // маршруты через каждую остановку
    std::unordered_map<const Stop*, std::set<std::string_view>> routes_on_stops_;
    // длина пути между остановками
    std::unordered_map<std::pair<const Stop*, const Stop*>, uint64_t, StopHasher<Stop>> distances_;

    // считает общее расстояние по маршруту
    uint64_t CalculateRealRouteLength(const Route* route) const;

}; //class TransportCatalogue

// считает общее колисечтво остановок
int CalculateStops(const Route *route) noexcept;
// считает колисечтво уникальных остановок
int CalculateUniqueStops(const Route *route) noexcept;
// считает расстояние между остановками
double CalculateRouteLength(const Route *route) noexcept;

}//namespace transport_catalogue