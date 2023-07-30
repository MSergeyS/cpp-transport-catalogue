#include <fstream>
#include <iostream>
#include <string_view>

#include "json_reader.h"
#include "request_handler.h"
#include "serialization.h"

using namespace std::literals;
using namespace transport_catalogue;

void PrintUsage(std::ostream& stream = std::cerr) {
	stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		PrintUsage();
		return 1;
	}

	const std::string_view mode(argv[1]);

	TransportCatalogue tc;
	renderer::MapRenderer map_render;
	transport_router::TransportRouter transport_router(tc);
	serialization::Serialization serialization(tc, map_render, transport_router);

	request_handler::RequestHandler handler(tc, map_render, transport_router, serialization);
	json_reader::JsonReader json_reader(handler, std::cin, std::cout);

    json_reader.ReadRequests();

	if (mode == "make_base"sv) {

		// инициализируем router (строим graph)
		handler.RouterInitializeGraph();
		// сохраняем в файл
		serialization.SaveTo();

	}
	else if (mode == "process_requests"sv) {
        
		// загружаем из файла
		serialization.LoadFrom();
		// обрабатываем stat_requests
        json_reader.HandleStatRequests();
		
	}
	else {
		PrintUsage();
		return 1;
	}	
	return 0;
}