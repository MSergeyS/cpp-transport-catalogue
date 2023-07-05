#pragma once

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include <optional>

#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

 // Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
 // с другими подсистемами приложения.
 // См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
 
namespace request_handler {
    class RequestHandler {
    public:
        RequestHandler(transport_catalogue::TransportCatalogue& db,
            renderer::MapRenderer& renderer, transport_router::TransportRouter& router);

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<const RouteInfo*> GetRouteInfo(const std::string_view& bus_name) const;

        // Возвращает маршруты, проходящие через остановку
        const std::set<std::string_view>* GetRoutesOnStop(const std::string_view stop_name) const;

        bool StopIs(const std::string_view stop_name) const;

        // Рисуем карту
        svg::Document RenderMap() const;

        // добавление остановки в базу
        void AddStop(const std::string& stop_name, geo::Coordinates coordinate);

        // задаёт дистанцию между остановками p_stop1 и p_stop2
        void SetStopDistance(const std::string_view name_stop1, const std::string_view name_stop2, uint64_t distance);

        // добавление маршрута в базу
        void AddRoute(std::string_view name, RouteType type, std::vector<std::string_view> stops);

        void SetRenderSettings(const renderer::RenderSettings& render_settings);

        // установка параметров построения маршрута
        void SetRoutingSettings(const RoutingSettings settings);

        // построение маршрута
        std::optional<std::vector<RouteData>> CreateRoute(const std::string_view& from, const std::string_view& to) const;

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        transport_catalogue::TransportCatalogue& db_;
        renderer::MapRenderer& renderer_;
        transport_router::TransportRouter& router_;
    };
} // namespace request_handler
