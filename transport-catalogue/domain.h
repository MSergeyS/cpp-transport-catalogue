#pragma once

/*
 * В этом файле размещены классы/структуры, которые являются частью предметной области (domain)
 * приложения и не зависят от транспортного справочника.
 */

#include <string>
#include <vector>

#include "geo.h"

// тип маршрута
enum class RouteType {
	UNKNOWN,
	LINEAR,  // линейный
	CIRCLE,  // кольцевой
};

// Остановки
// остановка состоит из имени и координат. Считаем, что имена уникальны.
struct Stop {
	std::string name;  // название остановки
	geo::Coordinates coordinate; // координаты
	size_t id = 0;

	friend bool operator==(const Stop& lhs, const Stop& rhs);
};

// Автобусные маршруты
// Маршрут состоит из имени (номера автобуса), типа и списка остановок. Считаем, что имена уникальны.
struct Route {
	std::string name;  // название маршрута
	RouteType route_type = RouteType::UNKNOWN;
	std::vector<const Stop*> stops; // указатели на остановки

	friend bool operator==(const Route& lhs, const Route& rhs);
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

// настройки маршрутов
struct RoutingSettings {
        int bus_wait_time = 0;  // время ожидания автобуса на остановке, в минутах
        double bus_velocity = 0.0;  // скорость автобуса, в км/ч
    };

struct RouteData {
    std::string_view type = { };
    std::string_view bus_name = { };
    std::string_view stop_name = { };
    double motion_time = 0.0;
    int bus_wait_time = 0;
    int span_count = 0;
};
