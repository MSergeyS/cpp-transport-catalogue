/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include "request_handler.h"

namespace request_handler {

    // RequestHandler ------------------------------------------------------------------------------------
    
    // конструктор
	RequestHandler::RequestHandler(transport_catalogue::TransportCatalogue& catalogue,
		renderer::MapRenderer& renderer, transport_router::TransportRouter& router,
        serialization::Serialization& serialization) :
		db_(catalogue), renderer_(renderer), router_(router), serialization_(serialization) {
	}


    // TransportCatalogue ----------------------------------------------------------------------------------

	// Возвращает информацию о маршруте (запрос Bus)
	std::optional<const RouteInfo*> RequestHandler::GetRouteInfo(const std::string_view& bus_name) const {
		return db_.GetRouteInfo(bus_name);
	}

	// Возвращает маршруты, проходящие через
	const std::set<std::string_view>* RequestHandler::GetRoutesOnStop(const std::string_view stop_name) const {
		return db_.GetRoutesOnStop(db_.GetStopByName(stop_name));
	}

    //
	bool RequestHandler::StopIs(const std::string_view stop_name) const {
	    return (db_.GetStopByName(stop_name) != nullptr);
	}

    void RequestHandler::AddStop(const std::string& stop_name, const geo::Coordinates coordinate) {
        db_.AddStop(stop_name, coordinate, db_.GetNumberStops());
    }

    // задаёт дистанцию между остановками p_stop1 и p_stop2
    void RequestHandler::SetStopDistance(const std::string_view name_stop1, const std::string_view name_stop2, uint64_t distance) {
        db_.SetStopDistance(db_.GetStopByName(name_stop1), db_.GetStopByName(name_stop2), distance);
    }

    // добавление маршрута в базу
    void RequestHandler::AddRoute(std::string_view name, RouteType type, std::vector<std::string_view> stops) {
        db_.AddRoute(name, type, stops, db_.GetNumberRoutes());
    }

    // поиск остановки по имени
    const std::unordered_map<std::string_view, const Stop*>& RequestHandler::GetAllStops() const {
        return db_.GetAllStops();
    }

    // поиск маршрута по имени
    const std::unordered_map<std::string_view, const Route*>& RequestHandler::GetAllRoutes() const {
        return db_.GetAllRoutes();
    }

    // получение всех расстояний между парами остановок
    std::vector<DistanceBeetweenPairStops> RequestHandler::GetAllDistanceBeetweenPairStops() {
        return db_.GetAllDistanceBeetweenPairStops();
    }

    // поиск имени остановки по ID
    std::string_view RequestHandler::GetStopNameById(uint32_t id) const {
        return db_.GetStopNameById(id);
    }


    // MapRenderer -----------------------------------------------------------------------------------------

    // установка параметров MapRenderer
    void RequestHandler::SetRenderSettings(const renderer::RenderSettings& render_settings) {
        renderer_.SetRenderSettings(render_settings);
    }

    // рисуем карту
	svg::Document RequestHandler::RenderMap() const	{
		return renderer_.CreateMap(db_);
	}


    // TransportRouter -------------------------------------------------------------------------------------

    // задание установок построения маршрута
    void RequestHandler::SetRoutingSettings(const RoutingSettings settings) {
        router_.SetRoutingSettings(settings.bus_wait_time, settings.bus_velocity);
        //router_.InitializeGraph();
    }

    // построение маршрута между двумя остановками
    std::optional<std::vector<RouteData>> RequestHandler::CreateRoute(const std::string_view& from,
            const std::string_view& to) const {
        return router_.CreatRoute(from, to);
    }

    void RequestHandler::RouterInitializeGraph() {
        router_.InitializeGraph();
    }

    // Serialization -----------------------------------------------------------------------------------------

    // установка настроек сериализации
    void RequestHandler::SetSerializationSettings(const serialization::SerializationSettings settings) {
        serialization_.SetSettings(settings);
    }

} // namespace request_handler