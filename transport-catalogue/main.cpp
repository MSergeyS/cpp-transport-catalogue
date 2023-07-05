//#include "input_reader.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "request_handler.h"

using namespace std;
using namespace transport_catalogue;
//using namespace query;


int main() {

    // using namespace std::literals;

    //{
    //    TransportCatalogue tc;
    //    InputReader ir;
    //    ir.ParseInput();
    //    ir.ParseInput();
    //    ir.Load(tc);
    //    //tests();
    //}

    {
        TransportCatalogue tc;
        renderer::MapRenderer map_render;
        //transport_router::TransportRouter router(tc, route_setting);
        transport_router::TransportRouter transport_router(tc);
        request_handler::RequestHandler handler(tc, map_render, transport_router);
        json_reader::JsonReader join_reader(handler, std::cin, std::cout);
        join_reader.ReadRequests();
    }
    return 0;
}
