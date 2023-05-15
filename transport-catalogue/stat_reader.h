#pragma once

#include <iostream>
#include <string_view>

#include "input_reader.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

namespace stat_reader {

void OutputRouteAbout(TransportCatalogue& tc, std::string_view route, std::ostream& output);

void OutputStopAbout(TransportCatalogue& tc, std::string_view name, std::ostream& output);

}//namespace stat_reader
}//transport_catalogue
