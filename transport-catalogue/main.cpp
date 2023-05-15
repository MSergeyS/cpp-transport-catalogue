#include "input_reader.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;
using namespace query;

int main() {
    TransportCatalogue tc;
    InputReader ir;
    ir.ParseInput(std::cin);
    ir.ParseInput(std::cin);
    ir.Load(tc);
    //tests();
    return 0;
}
