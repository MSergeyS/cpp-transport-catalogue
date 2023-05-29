/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include "request_handler.h"

namespace request_handler {
	RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& catalogue,
		const renderer::MapRenderer& renderer) : db_(catalogue), renderer_(renderer) {
	}

	// Возвращает информацию о маршруте (запрос Bus)
	std::optional<const RouteInfo*> RequestHandler::GetRouteInfo(const std::string_view& bus_name) const {
		return db_.GetRouteInfo(bus_name);
	}

	// Возвращает маршруты, проходящие через
	const std::set<std::string_view>* RequestHandler::GetRoutesOnStop(const std::string_view stop_name) const {
		return db_.GetRoutesOnStop(db_.GetStopByName(stop_name));
	}

	bool RequestHandler::StopIs(const std::string_view stop_name) const {
	    return (db_.GetStopByName(stop_name) != nullptr);
	}

	svg::Document RequestHandler::RenderMap() const	{
		return renderer_.CreateMap(db_);
	}
} // namespace request_handler
