#include <sstream>
#include <fstream>
#include <map>
#include <iostream>

#include "json_builder.h"
#include "json_reader.h"

namespace json_reader
{
    using namespace std::literals;

    // JsonReader : public  -----------------------------------------------------

    JsonReader::JsonReader(request_handler::RequestHandler& handler,
        std::istream& input, std::ostream& output)
        : handler_(handler), input_(input), output_(output) {
    }

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
        const json::Dict dict = json::Load(input_).GetRoot().AsDict();
        const auto base_requests = dict.find("base_requests"s);
        if (base_requests != dict.end()) {
            MakeBase(base_requests->second.AsArray());
        }
        const auto render_settings = dict.find("render_settings"s);
        if (render_settings != dict.end())
        {
            SetMapRenderer(render_settings->second.AsDict());
        }

        const auto routing_settings = dict.find("routing_settings"s);
        if (routing_settings != dict.end())
        {
            SetRoutingSettings(routing_settings->second.AsDict());
        }

        const auto stat_requests = dict.find("stat_requests"s);
        if (stat_requests != dict.end()) {
            StatRequests(stat_requests->second.AsArray());
        }
    }
    catch (const std::logic_error& err) {
        std::cerr << "Invalid data format: "s << err.what() << std::endl;
    }
    catch (const json::ParsingError& err) {
        std::cerr << "ParsingError: "s << err.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error"s << std::endl;
    }
    }

    // input --------------------------------------------------------------------------

    void JsonReader::MakeBase(const json::Array& arr) {
        for (const auto& request : arr) {
            const auto request_type = request.AsDict().find("type");
            if (request_type != request.AsDict().end()) {
                if (request_type->second.AsString() == "Stop") {
                    ReadStopData(request.AsDict());
                }
            }
        }

        for (const auto& request : arr) {
            const auto request_type = request.AsDict().find("type");
            if (request_type != request.AsDict().end()) {
                if (request_type->second.AsString() == "Stop") {
                    ReadStopDistance(request.AsDict());
                }
            }
        }

        for (const auto& request : arr) {
            const auto request_type = request.AsDict().find("type");
            if (request_type != request.AsDict().end()) {
                if (request_type->second.AsString() == "Bus") {
                    ReadBusData(request.AsDict());
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
        const json::Dict stops = dict.at("road_distances").AsDict();
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

    // output -------------------------------------------------------------------------

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
            if (request.AsDict().empty()) {
                continue;
            }
            if (request.AsDict().at("type").AsString() == "Stop") {
                json::Node dict_node_stop = RequestStop(request);
                arr_answer.push_back(std::move(dict_node_stop));
                continue;
            }
            else if (request.AsDict().at("type").AsString() == "Bus") {
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
            else if (request.AsDict().at("type").AsString() == "Map") {
                json::Node dict_node_map = RequestMap(request);
                arr_answer.push_back(std::move(dict_node_map));
                continue;
            }

            else if (request.AsDict().at("type").AsString() == "Route") {
                json::Node dict_node_route = RequestRoute(request);
                arr_answer.push_back(std::move(dict_node_route));
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
        std::string_view name = value.AsDict().at("name"s).AsString();
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
                .Key("request_id"s).Value(value.AsDict().at("id"s).AsInt())
                .EndDict().Build().AsDict();
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
        auto route_info = handler_.GetRouteInfo(value.AsDict().at("name"s).AsString());
        if (route_info == nullptr) {
            return CreateEmptyAnswer(value);
        }
        return json::Builder{}.StartDict()
                .Key("curvature"s).Value((*route_info)->curvature)
                .Key("request_id"s).Value(value.AsDict().at("id"s).AsInt())
                .Key("route_length"s).Value((*route_info)->route_length)
                .Key("stop_count"s).Value((*route_info)->number_of_stops)
                .Key("unique_stop_count"s).Value((*route_info)->number_of_unique_stops)
                .EndDict().Build().AsDict();
    }

    /*
    Ответ на этот запрос отдаётся в виде словаря с ключами request_id и map:
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
                .Key("request_id"s).Value(value.AsDict().at("id"s).AsInt())
                .EndDict().Build();
    }

    /*
    Новый тип запросов к базе — Route
    В список stat_requests добавляются элементы с "type": "Route" — это запросы на построение маршрута
    между двумя остановками.
    Помимо стандартных свойств id и type, они содержат ещё два:
      - from — остановка, где нужно начать маршрут.
      - to — остановка, где нужно закончить маршрут.
    Оба значения — названия существующих в базе остановок.
    Однако они, возможно, не принадлежат ни одному автобусному маршруту.
    Пример
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Universam",
      "id": 4
    }
    Данный запрос означает построение маршрута от остановки “Biryulyovo Zapadnoye” до остановки “Universam”.
    */


    /*
    Новая секция — routing_settings
    Во входной JSON добавляется ключ routing_settings, значение которого — словарь с двумя ключами:
        bus_wait_time — время ожидания автобуса на остановке, в минутах.
            Считайте, что когда бы человек ни пришёл на остановку и какой бы ни была эта остановка,
            он будет ждать любой автобус в точности указанное количество минут.
            Значение — целое число от 1 до 1000.
        bus_velocity — скорость автобуса, в км/ч. Считайте, что скорость любого автобуса постоянна и
            в точности равна указанному числу. Время стоянки на остановках не учитывается,
            время разгона и торможения тоже. Значение — вещественное число от 1 до 1000.
    Пример
    "routing_settings": {
        "bus_wait_time": 6,
        "bus_velocity": 40
    }
    Данная конфигурация задаёт время ожидания, равным 6 минутам, и скорость автобусов, равной 40 километрам в час.

    На маршруте человек может использовать несколько автобусов.
    Один автобус даже можно использовать несколько раз, если на некоторых участках он делает большой крюк и проще
    срезать на другом автобусе.
    Маршрут должен быть наиболее оптимален по времени. Если маршрутов с минимально возможным суммарным временем
    несколько, допускается вывести любой из них: тестирующая система проверяет лишь совпадение времени маршрута
    с оптимальным и корректность самого маршрута.
    При прохождении маршрута время расходуется на два типа активностей:
      - Ожидание автобуса. Всегда длится bus_wait_time минут.
      - Поездка на автобусе. Всегда длится ровно такое количество времени, которое требуется для преодоления
        данного расстояния со скоростью bus_velocity. Расстояние между остановками вычисляется по дорогам,
        то есть с использованием road_distances.

    Ходить пешком, выпрыгивать из автобуса между остановками и использовать другие виды транспорта запрещается.
    На конечных остановках все автобусы высаживают пассажиров и уезжают в парк.
    Даже если человек едет на кольцевом — "is_roundtrip": true — маршруте и хочет проехать мимо конечной,
    он будет вынужден выйти и подождать тот же самый автобус ровно bus_wait_time минут.
    Этот и другие случаи разобраны в примерах.
    Ответ на запрос Route устроен следующим образом:
    {
        "request_id": <id запроса>,
        "total_time": <суммарное время>,
        "items": [
            <элементы маршрута>
         ]
    }
    total_time — суммарное время в минутах, которое требуется для прохождения маршрута, выведенное в виде вещественного числа.
    Обратите внимание, что расстояние от остановки A до остановки B может быть не равно расстоянию от B до A!
    items — список элементов маршрута, каждый из которых описывает непрерывную активность пассажира, требующую временных затрат.
    А именно элементы маршрута бывают двух типов.

    Wait — подождать нужное количество минут (в нашем случае всегда bus_wait_time) на указанной остановке:
    {
        "type": "Wait",
        "stop_name": "Biryulyovo",
        "time": 6
    }
    Bus — проехать span_count остановок (перегонов между остановками) на автобусе bus, потратив указанное количество минут:
    {
         "type": "Bus",
         "bus": "297",
         "span_count": 2,
         "time": 5.235
    }
    Если маршрута между указанными остановками нет, выведите результат в следующем формате:
    {
         "request_id": <id запроса>,
         "error_message": "not found"
    }
    */
    json::Node CreateNodeBus(const RouteData& data) {
        json::Node node_route{ json::Builder{}.StartDict()
                         .Key("bus"s).Value(std::string(data.bus_name))
                         .Key("span_count"s).Value(data.span_count)
                         .Key("time"s).Value(data.motion_time)
                         .Key("type"s).Value("Bus"s)
                         .EndDict().Build() };
        return node_route;
    }

    json::Node CreateNodeStop(const RouteData& data) {
        json::Node node_stop{ json::Builder{}.StartDict()
                                            .Key("stop_name"s).Value(std::string(data.stop_name))
                                            .Key("time"s).Value(data.bus_wait_time)
                                            .Key("type"s).Value("Wait"s)
                                        .EndDict().Build() };
        return node_stop;
    }

    json::Node CreateNodeRoute(const std::vector<RouteData>& route_data) {
        json::Node data_route{ json::Builder{}.StartArray().EndArray().Build() };
        for (const RouteData& data : route_data) {
            if (data.type == "bus"sv) {
                const_cast<json::Array&>(data_route.AsArray()).push_back(std::move(CreateNodeBus(data)));
            }
            else if (data.type == "stop"sv) {
                const_cast<json::Array&>(data_route.AsArray()).push_back(std::move(CreateNodeStop(data)));
            }
            else if (data.type == "stay_here"sv) {
                return data_route;
            }
        }
        return data_route;
    }

    double CalcTotalTime(const std::vector<RouteData>& route_data) {
            double total_time = 0.0;
            for (const RouteData& data : route_data) {
                if (data.type == "bus"sv) {
                    total_time += data.motion_time;
                }
                else if (data.type == "stop"sv) {
                    total_time += data.bus_wait_time;
                }
            }
            return total_time;
        }

    json::Node JsonReader::RequestRoute(const json::Node& value) {
    std::optional < std::vector < RouteData >> route_data =
            handler_.CreateRoute(value.AsDict().at("from"s).AsString(),
                    value.AsDict().at("to"s).AsString());
        if (route_data == std::nullopt) {
            return json::Builder{}.StartDict()
                    .Key("error_message"s).Value("not found"s)
                    .Key("request_id"s).Value(value.AsDict().at("id"s).AsInt())
                    .EndDict().Build();
        }

        return json::Builder{}.StartDict()
                .Key("items"s).Value(CreateNodeRoute(route_data.value()).AsArray())
                .Key("request_id"s).Value(value.AsDict().at("id"s).AsInt())
                .Key("total_time"s).Value(CalcTotalTime(route_data.value()))
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
        settings.width = dict.at("width"s).AsDouble();
        settings.height = dict.at("height"s).AsDouble();
        settings.padding = dict.at("padding"s).AsDouble();
        settings.line_width = dict.at("line_width"s).AsDouble();
        settings.stop_radius = dict.at("stop_radius"s).AsDouble();
        settings.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
        settings.bus_label_offset = { dict.at("bus_label_offset"s).AsArray()[0].AsDouble(),
                 dict.at("bus_label_offset").AsArray()[1].AsDouble() };
        settings.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
        settings.stop_label_offset = { dict.at("stop_label_offset"s).AsArray()[0].AsDouble(),
                dict.at("stop_label_offset").AsArray()[1].AsDouble() };
        settings.underlayer_color = GetColor(dict.at("underlayer_color"s));
        settings.underlayer_width = dict.at("underlayer_width"s).AsDouble();
        for (const auto& color : dict.at("color_palette"s).AsArray()) {
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

    // transport router ------------------------------------------------------------------------

    void JsonReader::SetRoutingSettings(const json::Dict& dict) {
        handler_.SetRoutingSettings({dict.at("bus_wait_time"s).AsInt(),
                                     dict.at("bus_velocity"s).AsDouble()});
    }

    //------------------------------------------------------------------------------------------

    json::Node JsonReader::CreateEmptyAnswer(const json::Node& value) {
        return json::Builder{}.StartDict()
                .Key("request_id"s).Value(value.AsDict().at("id"s).AsInt())
                .Key("error_message"s).Value("not found"s)
                .EndDict().Build();
    }

} // namespace json_reader
