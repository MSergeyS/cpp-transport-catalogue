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