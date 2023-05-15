#pragma once

#include "input_reader.h"
#include "transport_catalogue.h"

#include <string_view>

namespace transport_catalogue {

namespace output {

void OutputRouteAbout(TransportCatalogue& tc, std::string_view route);

void OutputStopAbout(TransportCatalogue& tc, std::string_view name);

}//namespace output
}//transport_catalogue
