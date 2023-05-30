#include <sstream>
#include <fstream>
#include <map>
#include <iostream>

#include "json_reader.h"

namespace json_reader
{
    using namespace std::literals;

    JsonReader::JsonReader(request_handler::RequestHandler& handler,
            std::istream& input, std::ostream& output)
            : handler_(handler), input_(input), output_(output) {
            }

    //------------------input-------------------------

    /* Данные поступают из stdin в формате JSON-объекта. Его верхнеуровневая структура:
    {
        "base_requests": [...] ,
        "render_settings": { ... },
        "stat_requests": [...]
    }
    Это словарь, содержащий ключи:
      - base_requests — массив с описанием автобусных маршрутов и остановок,
      - stat_requests — массив с запросами к транспортному справочнику.
    Внимание, JSON может быть отформатирован произвольным образом: использовать или не использовать
    пробелы для отступов, ключи объектов могут быть расположены в разных строках или в одной.
    Следующие JSON-данные эквивалентны:
    {"base_requests":[],"stat_requests":[]}
    {
      "stat_requests": [],
      "base_requests": []
    }
    {
        "base_requests":     [
    ]
    ,
    "stat_requests":[]
    }
    Иными словами, разделительные пробелы, табуляции и символы перевода строки внутри JSON
    могут располагаться произвольным образом или вообще отсутствовать.
    */
    void JsonReader::ReadRequests() {
    try {
        const json::Dict dict = json::Load(input_).GetRoot().AsMap();
        const auto base_requests = dict.find("base_requests");
        if (base_requests != dict.end()) {
            MakeBase(base_requests->second.AsArray());
        }
        const auto render_settings = dict.find("render_settings");
        if (render_settings != dict.end())
        {
            SetMapRenderer(render_settings->second.AsMap());
        }
        const auto stat_requests = dict.find("stat_requests");
        if (stat_requests != dict.end()) {
            StatRequests(stat_requests->second.AsArray());
        }
    }
    catch (const std::logic_error& err) {
        std::cerr << "Invalid data format: " << err.what() << std::endl;
    }
    catch (const json::ParsingError& err) {
        std::cerr << "ParsingError: " << err.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error" << std::endl;
    }
    }

    //------------------input-------------------------

    void JsonReader::MakeBase(const json::Array& arr) {
        for (const auto& request : arr) {
            const auto request_type = request.AsMap().find("type");
            if (request_type != request.AsMap().end()) {
                if (request_type->second.AsString() == "Stop") {
                    ReadStopData(request.AsMap());
                }
            }
        }

        for (const auto& request : arr) {
            const auto request_type = request.AsMap().find("type");
            if (request_type != request.AsMap().end()) {
                if (request_type->second.AsString() == "Stop") {
                    ReadStopDistance(request.AsMap());
                }
            }
        }

        for (const auto& request : arr) {
            const auto request_type = request.AsMap().find("type");
            if (request_type != request.AsMap().end()) {
                if (request_type->second.AsString() == "Bus") {
                    ReadBusData(request.AsMap());
                }
            }
        }
    }

    /* Описание базы маршрутов
        Массив base_requests содержит элементы двух типов: маршруты и остановки.
        Они перечисляются в произвольном порядке.
       Пример описания остановки:
    {
       "type": "Stop",
       "name" : "Электросети",
       "latitude" : 43.598701,
       "longitude" : 39.730623,
       "road_distances" : {
            "Улица Докучаева": 3000,
            "Улица Лизы Чайкиной" : 4300
        }
    }
     Описание остановки — словарь с ключами:
       - type — строка, равная "Stop". Означает, что словарь описывает остановку;
       - name — название остановки;
       - latitude и longitude — широта и долгота остановки — числа с плавающей запятой;
       - road_distances — словарь, задающий дорожное расстояние от этой остановки до соседних. Каждый ключ
         в этом словаре — название соседней остановки, значение — целочисленное расстояние в метрах.
    */
    void JsonReader::ReadStopData(const json::Dict& dict) {
        const std::string name = dict.at("name").AsString();
        const auto latitude = dict.at("latitude").AsDouble();
        const auto longitude = dict.at("longitude").AsDouble();
        handler_.AddStop(name, { latitude, longitude });
    }

    void JsonReader::ReadStopDistance(const json::Dict& dict) {
        const std::string& from_stop_name = dict.at("name").AsString();
        const json::Dict stops = dict.at("road_distances").AsMap();
        for (const auto& [to_stop_name, distance] : stops) {
            handler_.SetStopDistance(from_stop_name, to_stop_name,
                    static_cast<unsigned long int>(distance.AsInt()));
        }
    }

    /* Пример описания автобусного маршрута:
    {
       "type": "Bus",
       "name": "14",
       "stops": [
          "Улица Лизы Чайкиной",
          "Электросети",
          "Улица Докучаева",
          "Улица Лизы Чайкиной"
       ],
       "is_roundtrip": true
    }
    Описание автобусного маршрута — словарь с ключами:
       - type — строка "Bus". Означает, что словарь описывает автобусный маршрут;
       - name — название маршрута;
       - stops — массив с названиями остановок, через которые проходит маршрут.
         У кольцевого маршрута название последней остановки дублирует название первой.
         Например: ["stop1", "stop2", "stop3", "stop1"];
       - is_roundtrip — значение типа bool. true, если маршрут кольцевой.
    */
    void JsonReader::ReadBusData(const json::Dict& dict) {
        const std::string_view bus_name = dict.at("name").AsString();
        std::vector<std::string_view> stops;
        for (const auto& stop : dict.at("stops").AsArray()) {
            stops.emplace_back(stop.AsString());
        }
        dict.at("stops").AsArray();
        const auto is_roundtrip = dict.at("is_roundtrip").AsBool();
        RouteType type = is_roundtrip ? RouteType::CIRCLE : RouteType::LINEAR;
        handler_.AddRoute(bus_name, type, stops);
    }

    //------------------output-------------------------

    /* Формат запросов к транспортному справочнику и ответов на них
       Запросы хранятся в массиве stat_requests.
       В ответ на них программа должна вывести в stdout JSON-массив ответов:
       [
         { ответ на первый запрос },
         { ответ на второй запрос },
         ...
         { ответ на последний запрос }
       ]
       Каждый запрос — словарь с обязательными ключами id и type.
       Они задают уникальный числовой идентификатор запроса и его тип.
       В словаре могут быть и другие ключи, специфичные для конкретного типа запроса.
       В выходном JSON-массиве на каждый запрос stat_requests должен быть ответ
       в виде словаря с обязательным ключом request_id.
       Значение ключа должно быть равно id соответствующего запроса.
       В словаре возможны и другие ключи, специфичные для конкретного типа ответа.
       Порядок следования ответов на запросы в выходном массиве должен совпадать
       с порядком запросов в массиве stat_requests.
    */
    void JsonReader::StatRequests(const json::Node& node) {
        json::Array arr_answer;
        for (const auto& request : node.AsArray()) {
            if (request.AsMap().empty()) {
                continue;
            }
            if (request.AsMap().at("type").AsString() == "Stop") {
                json::Node dict_node_stop = RequestStop(request);
                arr_answer.push_back(std::move(dict_node_stop));
                continue;
            }
            else if (request.AsMap().at("type").AsString() == "Bus") {
                json::Node dict_node_bus = RequestBus(request);
                arr_answer.push_back(std::move(dict_node_bus));
                continue;
            }

            /* запрос на получение изображения, который имеет следующий вид:
               {
                  "type": "Map",
                  "id": 11111
               } 
            */
            else if (request.AsMap().at("type").AsString() == "Map") {
                json::Node dict_node_map = RequestMap(request);
                arr_answer.push_back(std::move(dict_node_map));
                continue;
            }
        }
        json::Print(json::Document{ arr_answer }, output_);
    }

    /*
    Получение информации об остановке
      Формат запроса:
      {
        "id": 12345,
        "type": "Stop",
        "name": "Улица Докучаева"
      } 

      Ключ name задаёт название остановки.
      Ответ на запрос:
      {
        "buses": [
            "14", "22к"
        ],
        "request_id": 12345
      } 

      Значение ключей ответа:
        - buses — массив названий маршрутов, которые проходят через эту остановку.
          Названия отсортированы в лексикографическом порядке.
        - request_id — целое число, равное id соответствующего запроса Stop.

      Если в справочнике нет остановки с переданным названием, ответ на запрос должен быть такой:
      {
        "request_id": 12345,
        "error_message": "not found"
      } 
    */ 
    json::Node JsonReader::RequestStop(const json::Node& value) {
        std::string_view name = value.AsMap().at("name"s).AsString();
        if (!handler_.StopIs(name)) {
            return CreateEmptyAnswer(value);
        }
        auto buses = handler_.GetRoutesOnStop(name);
        json::Array arr;
        if (buses != nullptr) {
            for (std::string_view bus : *buses) {
                arr.push_back(std::string(bus));
            }
        }
        return json::Builder{}.StartDict()
                .Key("buses"s).Value(arr)
                .Key("request_id"s).Value(value.AsMap().at("id"s).AsInt())
                .EndDict().Build().AsMap();
    }
    
    /*
    Получение информации о маршруте
      Формат запроса:
      {
        "id": 12345678,
        "type": "Bus",
        "name": "14"
      } 

      Ключ type имеет значение “Bus”. По нему можно определить, что это запрос
      на получение информации о маршруте.
      Ключ name задаёт название маршрута, для которого приложение должно вывести
      статистическую информацию.
      Ответ на этот запрос должен быть дан в виде словаря:

      {
        "curvature": 2.18604,
        "request_id": 12345678,
        "route_length": 9300,
        "stop_count": 4,
        "unique_stop_count": 3
      } 

      Ключи словаря:
        - curvature — извилистость маршрута. Она равна отношению длины дорожного расстояния
          маршрута к длине географического расстояния. Число типа double;
        - request_id — должен быть равен id соответствующего запроса Bus. Целое число;
        - route_length — длина дорожного расстояния маршрута в метрах, целое число;
        - stop_count — количество остановок на маршруте;
        - unique_stop_count — количество уникальных остановок на маршруте.

      Например, на кольцевом маршруте с остановками A, B, C, A четыре остановки. Три из них уникальные.
      На некольцевом маршруте с остановками A, B и C пять остановок (A, B, C, B, A). Три из них уникальные.
      Если в справочнике нет маршрута с указанным названием, ответ должен быть таким:

      {
        "request_id": 12345678,
        "error_message": "not found"
      } 
    */
    json::Node JsonReader::RequestBus(const json::Node& value) {
        auto route_info = handler_.GetRouteInfo(value.AsMap().at("name"s).AsString());
        if (route_info == nullptr) {
            return CreateEmptyAnswer(value);
        }
        return json::Builder{}.StartDict()
                .Key("curvature"s).Value((*route_info)->curvature)
                .Key("request_id"s).Value(value.AsMap().at("id"s).AsInt())
                .Key("route_length"s).Value((*route_info)->route_length)
                .Key("stop_count"s).Value((*route_info)->number_of_stops)
                .Key("unique_stop_count"s).Value((*route_info)->number_of_unique_stops)
                .EndDict().Build().AsMap();
    }

    /*Ответ на этот запрос отдаётся в виде словаря с ключами request_id и map:
      {
        "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n  <polyline points=\"100.817,170 30,30 100.817,170\" fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"100.817\" y=\"170\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"green\" x=\"100.817\" y=\"170\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"30\" y=\"30\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"green\" x=\"30\" y=\"30\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <circle cx=\"100.817\" cy=\"170\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"30\" cy=\"30\" r=\"5\" fill=\"white\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"100.817\" y=\"170\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"black\" x=\"100.817\" y=\"170\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"30\" y=\"30\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Ривьерский мост</text>\n  <text fill=\"black\" x=\"30\" y=\"30\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Ривьерский мост</text>\n</svg>",
        "request_id": 11111
      } 

      Ключ map — строка с изображением карты в формате SVG. Следующие спецсимволы при выводе строк
      в JSON нужно экранировать:
        - двойные кавычки";
        - обратный слэш \;
        - символы возврата каретки и перевода строки.
    */
    json::Node JsonReader::RequestMap(const json::Node& value) {
        svg::Document doc = handler_.RenderMap();
        std::ostringstream ostr;
        doc.Render(ostr);
        return json::Builder{}.StartDict()
                .Key("map"s).Value(ostr.str())
                .Key("request_id"s).Value(value.AsMap().at("id"s).AsInt())
                .EndDict().Build();
    }

    //------------------render-------------------------

    /* Структура словаря render_settings:
      {
        "width": 1200.0,
        "height": 1200.0,

        "padding": 50.0,

        "line_width": 14.0,
        "stop_radius": 5.0,

        "bus_label_font_size": 20,
        "bus_label_offset": [7.0, 15.0],

        "stop_label_font_size": 20,
        "stop_label_offset": [7.0, -3.0],

        "underlayer_color": [255, 255, 255, 0.85],
        "underlayer_width": 3.0,

        "color_palette": [
          "green",
          [255, 160, 0],
          "red"
        ]
    } 

      - width и height — ширина и высота изображения в пикселях. Вещественное число
        в диапазоне от 0 до 100000.
      - padding — отступ краёв карты от границ SVG-документа. Вещественное число не меньше 0
        и меньше min(width, height)/2.
      - line_width — толщина линий, которыми рисуются автобусные маршруты. Вещественное число
        в диапазоне от 0 до 100000.
      - stop_radius — радиус окружностей, которыми обозначаются остановки. Вещественное число
        в диапазоне от 0 до 100000.
      - bus_label_font_size — размер текста, которым написаны названия автобусных маршрутов.
        Целое число в диапазоне от 0 до 100000.
      - bus_label_offset — смещение надписи с названием маршрута относительно координат
        конечной остановки на карте. Массив из двух элементов типа double. Задаёт значения
        свойств dx и dy SVG-элемента <text>. Элементы массива — числа в диапазоне от –100000 до 100000.
      - stop_label_font_size — размер текста, которым отображаются названия остановок.
        Целое число в диапазоне от 0 до 100000.
      - stop_label_offset — смещение названия остановки относительно её координат на карте.
        Массив из двух элементов типа double. Задаёт значения свойств dx и dy SVG-элемента <text>.
        Числа в диапазоне от –100000 до 100000.
      - underlayer_color — цвет подложки под названиями остановок и маршрутов. Формат хранения цвета будет ниже.
      - underlayer_width — толщина подложки под названиями остановок и маршрутов.
        Задаёт значение атрибута stroke-width элемента <text>. Вещественное число в диапазоне от 0 до 100000.
      - color_palette — цветовая палитра. Непустой массив.
    */
    void JsonReader::SetMapRenderer(const json::Dict& dict) {
        renderer::RenderSettings settings;
        settings.width = dict.at("width").AsDouble();
        settings.height = dict.at("height").AsDouble();
        settings.padding = dict.at("padding").AsDouble();
        settings.line_width = dict.at("line_width").AsDouble();
        settings.stop_radius = dict.at("stop_radius").AsDouble();
        settings.bus_label_font_size = dict.at("bus_label_font_size").AsInt();
        settings.bus_label_offset = { dict.at("bus_label_offset").AsArray()[0].AsDouble(),
                 dict.at("bus_label_offset").AsArray()[1].AsDouble() };
        settings.stop_label_font_size = dict.at("stop_label_font_size").AsInt();
        settings.stop_label_offset = { dict.at("stop_label_offset").AsArray()[0].AsDouble(),
                dict.at("stop_label_offset").AsArray()[1].AsDouble() };
        settings.underlayer_color = GetColor(dict.at("underlayer_color"));
        settings.underlayer_width = dict.at("underlayer_width").AsDouble();
        for (const auto& color : dict.at("color_palette").AsArray()) {
            settings.color_palette.emplace_back(GetColor(color));
        }
        handler_.SetRenderSettings(settings);
    }

    /*
    Цвет можно указать в одном из следующих форматов:

    в виде строки, например, "red" или "black";
    в массиве из трёх целых чисел диапазона [0, 255]. Они определяют r, g и b компоненты цвета
    в формате svg::Rgb. Цвет [255, 16, 12] нужно вывести в SVG как rgb(255,16,12);
    в массиве из четырёх элементов: три целых числа в диапазоне от [0, 255] и одно вещественное число
    в диапазоне от [0.0, 1.0]. Они задают составляющие red, green, blue и opacity цвета формата svg::Rgba.
    Цвет, заданный как [255, 200, 23, 0.85], должен быть выведен в SVG как rgba(255,200,23,0.85).
    */
    const svg::Color JsonReader::GetColor(const json::Node& color) {
        if (color.IsString()) {
            return svg::Color{ color.AsString() };
        }
        else if (color.IsArray()) {
            if (color.AsArray().size() == 3) {
                return svg::Rgb {
                    static_cast<uint8_t>(color.AsArray()[0].AsInt()),
                    static_cast<uint8_t>(color.AsArray()[1].AsInt()),
                    static_cast<uint8_t>(color.AsArray()[2].AsInt())
                };
            }
            else if (color.AsArray().size() == 4) {
                return svg::Rgba {
                    static_cast<uint8_t>(color.AsArray()[0].AsInt()),
                    static_cast<uint8_t>(color.AsArray()[1].AsInt()),
                    static_cast<uint8_t>(color.AsArray()[2].AsInt()),
                    color.AsArray()[3].AsDouble()
                };
            }
        }
        return svg::Color();
    }

    //--------------------------------------------------

    json::Node JsonReader::CreateEmptyAnswer(const json::Node& value) {
        return json::Builder{}.StartDict()
                .Key("request_id"s).Value(value.AsMap().at("id"s).AsInt())
                .Key("error_message"s).Value("not found"s)
                .EndDict().Build();
    }

} // namespace json_reader
