#pragma once

#include "transport_catalogue.h"

#include <charconv>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace transport_catalogue {

namespace query {

enum class QueryType {
    StopX,
    BusX
};

struct Command {
    QueryType type;  // тип команды
    std::string name;  // имя
    std::pair<std::string_view, std::string_view> coordinates;  // координаты
    std::vector<std::pair<std::string_view, std::string_view>> distances; // расстояния
    std::vector<std::string_view> route_stops; // остановки маршрута
    RouteType route_type = RouteType::LINEAR; // тип маршрута
    std::string origin_command;  // текущая команда
    std::string direction_command; // директива (команда на изменение базы данных)

    // разбирает координаты остановки
    std::pair<std::string_view, std::string_view> ParseCoordinates(
            std::string_view latitude, std::string_view longitude);
    // разбирает остановки маршрута
    std::vector<std::string_view> ParseBuses(
            std::vector<std::string_view> vec_input);
    // разбирает расстояния между остановками
    std::vector<std::pair<std::string_view, std::string_view>> ParseDistances(
            std::vector<std::string_view> vec_input);

    // разбирает команды
    void ParseCommandString(std::string input);
};

// разделяет строки на слова
inline std::vector<std::string_view> Split(std::string_view string, char delim);

class InputReader {
public:
    // переставляем элементы в очереди запросов
    void Load(TransportCatalogue& tc);
    // выполнение команды
    void LoadCommand(TransportCatalogue& tc, Command com);
    // разбирает ввод
    void ParseInput();

private:
    std::vector<Command> commands_;
}; //class InputReader

} //namespace query
} //namespace transport_catalogue
