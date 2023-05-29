#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>

#include <algorithm>
#include <unordered_map>

using namespace std;

namespace transport_catalogue {

namespace query {

void InputReader::ParseInput() {
    int query_count;
    cin >> query_count;
    cin.ignore();
    string command;

    for (int i = 0; i < query_count; ++i) {
        getline(cin, command);
        Command cur_command;
        cur_command.ParseCommandString(move(command));
        commands_.push_back(move(cur_command));
    }
}

inline std::vector<std::string_view> Split(std::string_view str, char delim) {
    std::vector<std::string_view> result;
    int64_t pos = 0;
    const int64_t pos_end = str.npos;

    while (true) {
        int64_t delimiter = str.find(delim, pos);
        result.push_back(delimiter == pos_end ? str.substr(pos) : str.substr(pos, delimiter - pos));

        if (delimiter == pos_end) {
            break;
        } else {
            pos = delimiter + 1;
        }
    }

    return result;
}

pair<string_view, string_view> Command::ParseCoordinates(string_view latitude,
        string_view longitude) {
    while (longitude.front() == ' ') {
        longitude.remove_prefix(1);
    }

    while (latitude.front() == ' ') {
        latitude.remove_prefix(1);
    }

    while (longitude.back() == ' ') {
        longitude.remove_suffix(1);
    }

    while (latitude.back() == ' ') {
        latitude.remove_suffix(1);
    }

    return make_pair(latitude, longitude);
}

vector<pair<string_view, string_view>> Command::ParseDistances(
        vector<string_view> vec_input) {
    vector<pair<string_view, string_view>> result;
    size_t i = 2;

    while (i < vec_input.size()) {
        while (vec_input[i].front() == ' ') {
            vec_input[i].remove_prefix(1);
        }

        auto pos_m = vec_input[i].find('m');
        string_view dist, stop;
        dist = vec_input[i].substr(0, pos_m);
        auto pos_t = vec_input[i].find('t');
        pos_t += 2;
        stop = vec_input[i].substr(pos_t, vec_input[i].length());

        while (stop.front() == ' ') {
            stop.remove_prefix(1);
        }

        while (stop.back() == ' ') {
            stop.remove_suffix(1);
        }

        result.push_back({dist, stop});
        ++i;
    }
    return result;
}

vector<string_view> Command::ParseBuses(vector<string_view> vec_input) {
    vector<string_view> result;
    vector<string_view> parsed_buses;

    if (vec_input.size() > 3) {
        if (direction_command.find('-') != string::npos) {
            route_type = RouteType::LINEAR;
            parsed_buses = Split(direction_command, '-');
        }

        if (direction_command.find('>') != string::npos) {
            route_type = RouteType::CIRCLE;
            parsed_buses = Split(direction_command, '>');
        }
    }
    for (size_t i = 0; i < parsed_buses.size(); ++i) {
        while (parsed_buses[i].front() == ' ') {
            parsed_buses[i].remove_prefix(1);
        }

        while (parsed_buses[i].back() == ' ') {
            parsed_buses[i].remove_suffix(1);
        }

        result.push_back(parsed_buses[i]);
    }

    return result;
}

void Command::ParseCommandString(string input) {
    static std::unordered_map<std::string, QueryType> const table = {
        {"Stop", QueryType::StopX}, {"Bus", QueryType::BusX}
    };
    origin_command = move(input);
    auto vec_input = Split(origin_command, ' ');
    auto pos_start = origin_command.find_first_not_of(' ');
    auto pos_end_of_command = origin_command.find(' ', pos_start);
    string temp_type = {origin_command.begin() + pos_start, origin_command.begin() + pos_end_of_command};

    switch (table.at(temp_type)) {
        case QueryType::StopX:
            type = QueryType::StopX;
            if (auto pos = origin_command.find(':'); pos != string::npos) {
                //Stop stop1:
                direction_command = origin_command.substr(pos + 2, origin_command.length() - pos - 1);
                auto temp = Split(direction_command, ',');

                for (size_t i = pos_end_of_command; i < pos; ++i) {
                    name += origin_command[i];
                }

                while (name.front() == ' ') {
                    name.erase(name.begin());
                }

                coordinates = ParseCoordinates(temp[0], temp[1]);

                if (temp.size() > 2) {
                    distances = ParseDistances(temp);
                }
            }
            else {
                //Stop stop1
                for (size_t i = pos_end_of_command; i < origin_command.size(); ++i) {
                    name += origin_command[i];
                }

                while (name.front() == ' ') {
                    name.erase(name.begin());
                }

                while (name.back() == ' ') {
                    name.erase(name.end() - 1);
                }
                break;
            }
            break;
        case QueryType::BusX:
            type = QueryType::BusX;
            if (auto pos = origin_command.find(':'); pos != string::npos) {
                direction_command = origin_command.substr(pos + 2, origin_command.length() - pos - 1);
                //Bus bus1:
                for (size_t i = pos_end_of_command; i < pos; ++i) {
                    name += origin_command[i];
                }

                while (name.front() == ' ') {
                    name.erase(name.begin());
                }
                route_stops = ParseBuses(vec_input);
            }
            else {
                //Bus bus1
                for (size_t i = pos_end_of_command; i < origin_command.size(); ++i) {
                    name += origin_command[i];
                }

                while (name.front() == ' ') {
                    name.erase(name.begin());
                }

                while (name.back() == ' ') {
                    name.erase(name.end() - 1);
                }
                break;
            }
            break;
    }
}

void InputReader::Load(TransportCatalogue& tc) {
    // сортируем запросы (переставляем элементы в очереди запросов)
    // сначала команды (direction_command или description)
    auto it_desc = partition(commands_.begin(), commands_.end(), [](Command com) {
        return !com.direction_command.empty();
    });
    // сначала остановки (stop или bus)
    auto it_stops = partition(commands_.begin(), it_desc, [](Command com) {
        return com.type == QueryType::StopX;
    });
    
    //Stop X:
    // Cпецификация остановки будет описывать расстояния до остановок,
    // которые будут определены позже. Для решения проблемы можете обрабатывать
    // ввод в два прохода
    // — на первом добавить остановку с координатами,
    for (auto cur_it = commands_.begin(); cur_it != it_stops; ++cur_it) {
        InputReader::LoadCommand(tc, *cur_it);
    }
    // на последующем — расстояния до соседних остановок.
    for (auto cur_it = commands_.begin(); cur_it != it_stops; ++cur_it) {
        InputReader::LoadCommand(tc, *cur_it);
    }
    //Bus X:
    for (auto cur_it = it_stops; cur_it != it_desc; ++cur_it) {
        InputReader::LoadCommand(tc, *cur_it);
    }
    //output
    for (auto cur_it = it_desc; cur_it != commands_.end(); ++cur_it) {
        InputReader::LoadCommand(tc, *cur_it);
    }
}

void InputReader::LoadCommand(TransportCatalogue& tc, Command com) {
    // выполнение команды
    switch (com.type) {
        case QueryType::StopX:
            if (com.coordinates != pair<string_view, string_view>()) {
                string lat = {com.coordinates.first.begin(), com.coordinates.first.end()};
                string lon = {com.coordinates.second.begin(), com.coordinates.second.end()};
                tc.AddStop(com.name, {stod(lat), stod(lon)});
                if (!com.distances.empty()) {
                    for (auto& [dist, stop] : com.distances) {
                        string dist_str = { dist.begin(), dist.end() };
                        const Stop* p_stop1 = tc.GetStopByName(com.name);
                        const Stop* p_stop2 = tc.GetStopByName(stop);
                        tc.SetStopDistance(p_stop1, p_stop2, stoull(dist_str));
                    }
                }
            }
            else {
                output::OutputStopAbout(tc, com.name);
            }

            break;
        case QueryType::BusX:
            if (!com.route_stops.empty()) {
                tc.AddRoute(com.name, com.route_type, com.route_stops);
             }
            else {
                output::OutputRouteAbout(tc, com.name);
            }
            break;
    }
}

}//namespace query
}//namespace transport_catalogue
