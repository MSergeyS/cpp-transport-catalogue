#pragma once

/*
  json_reader — выполняет разбор JSON-данных, построенных в ходе парсинга,
  и формирует массив JSON-ответов;
*/

#pragma once

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "svg.h"

namespace json_reader
{

	void ReadRequests(std::istream& input, transport_catalogue::TransportCatalogue& catalogue);

	//------------------input-------------------------

	void MakeBase(transport_catalogue::TransportCatalogue& trans_cat, const json::Array& arr);
	void ReadStopData(transport_catalogue::TransportCatalogue& trans_cat, const json::Dict& dict);
	void ReadStopDistance(transport_catalogue::TransportCatalogue& trans_cat, const json::Dict& dict);
	void ReadBusData(transport_catalogue::TransportCatalogue& trans_cat, const json::Dict& dict);

	//------------------output-------------------------

	void StatRequests(const request_handler::RequestHandler& handler, const json::Node& node);
	json::Node RequestStop(const request_handler::RequestHandler& handler, const json::Node& value);
	json::Node RequestBus(const request_handler::RequestHandler& handler, const json::Node& value);
	json::Node RequestMap(const request_handler::RequestHandler& handler, const json::Node& value);

	//------------------render-------------------------

	const svg::Color GetColor(const json::Node& color);
	void SetMapRenderer(renderer::MapRenderer& map_renderer, const json::Dict& dict);

	//--------------------------------------------------

	json::Node CreateEmptyAnswer(const json::Node& value);

} // json_reader
