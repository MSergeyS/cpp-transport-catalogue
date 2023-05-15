#include "stat_reader.h"

#include <iomanip>
#include <set>

using namespace std;

namespace transport_catalogue {

namespace stat_reader {

void OutputRouteAbout(TransportCatalogue& tc, std::string_view route_name,
        std::ostream& output) {
    const Route* route = tc.GetRouteByName(route_name);
    if (route == nullptr) {
        output << "Bus "s << route_name << ": not found"s << endl;
    }
    else {
        RouteInfo bus_info = tc.GetRouteInfo(route->name);

        output << "Bus "s << route->name << ": "s << bus_info.number_of_stops
               << " stops on route, "s << bus_info.number_of_unique_stops
               << " unique stops, "s << std::setprecision(6)
               << bus_info.route_length << " route length, "s
               << bus_info.curvature << " curvature"s
               << endl;
    }
}

void OutputStopAbout(TransportCatalogue& tc, string_view name,
        std::ostream& output) {
    const Stop* stop = tc.GetStopByName(name);
    if (stop != nullptr) {
        set<string_view> buses = tc.GetRoutesOnStop(stop);
        if (buses.size() != 0) {
            output << "Stop "s << name << ": buses"s;
            for (string_view bus : buses) {
                output << " "s << bus ;
            }
        } else {
            output << "Stop "s << name << ": no buses"s;
        }
    }
    else {
        output << "Stop "s << name << ": not found";
    }
    output << endl;
}

}//namespace stat_reader
}//namespace transport_catalogue
