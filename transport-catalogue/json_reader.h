#pragma once

/*
  json_reader — выполняет разбор JSON-данных, построенных в ходе парсинга,
  и формирует массив JSON-ответов;
*/

#pragma once

#include "json.h"
#include "request_handler.h"
#include "svg.h"
#include <stdexcept>
#include <string>

namespace json_reader
{
class JsonReader {
public:
    // JsonReader : public  -----------------------------------------------------

    JsonReader(request_handler::RequestHandler& handler, std::istream& input, std::ostream& output);

	void ReadRequests();

private:
	request_handler::RequestHandler& handler_;
	std::istream& input_;
	std::ostream& output_;

    // input --------------------------------------------------------------------

    void MakeBase(const json::Array& arr);
    void ReadStopData(const json::Dict& dict);
    void ReadStopDistance(const json::Dict& dict);
    void ReadBusData(const json::Dict& dict);

    // output -------------------------------------------------------------------

    void StatRequests(const json::Node& node);
    json::Node RequestStop(const json::Node& value);
    json::Node RequestBus(const json::Node& value);
    json::Node RequestMap(const json::Node& value);

    // render -------------------------------------------------------------------

    const svg::Color GetColor(const json::Node& color);
    void SetMapRenderer(const json::Dict& dict);

    //---------------------------------------------------------------------------

	json::Node CreateEmptyAnswer(const json::Node& value);

}; // class JsonReader

} // json_reader