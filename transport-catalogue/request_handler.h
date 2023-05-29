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

#include "map_renderer.h"
#include "transport_catalogue.h"

 // Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
 // с другими подсистемами приложения.
 // См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
 
namespace request_handler {
    class RequestHandler {
    public:
        RequestHandler(const transport_catalogue::TransportCatalogue& db,
            const renderer::MapRenderer& renderer);

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<const RouteInfo*> GetRouteInfo(const std::string_view& bus_name) const;

        // Возвращает маршруты, проходящие через остановку
        const std::set<std::string_view>* GetRoutesOnStop(const std::string_view stop_name) const;

        bool StopIs(const std::string_view stop_name) const;

        // Рисуем карту
        svg::Document RenderMap() const;

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const transport_catalogue::TransportCatalogue& db_;
        const renderer::MapRenderer& renderer_;
    };
} // namespace request_handler
