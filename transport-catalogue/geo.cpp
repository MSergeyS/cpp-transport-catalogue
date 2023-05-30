#include "geo.h"

// geo — объявляет координаты на земной поверхности и вычисляет расстояние между ними

namespace geo {

    bool Coordinates::operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool Coordinates::operator!=(const Coordinates& other) const {
        return !(*this == other);
    }

    // вычисляет расстояние между двумя координатами
    double ComputeDistance(Coordinates from, Coordinates to) {
        using namespace std;
        const double dr = M_PI / 180.0;
        return acos(sin(from.lat * dr) * sin(to.lat * dr)
            + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
            * Earth_radius;
    }

}  // namespace geo
