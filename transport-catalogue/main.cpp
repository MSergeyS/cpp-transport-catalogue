//#include "input_reader.h"
#include "json_reader.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;
//using namespace query;


int main() {

    using namespace std::literals;

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
        json_reader::ReadRequests(std::cin, tc);
    }
    return 0;
}
