#pragma once

#define _USE_MATH_DEFINES
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include <cmath>

// функции для работы с географическими координатами

namespace geo {
    const double Earth_radius = 6371000;

    struct Coordinates {
        double lat;
        double lng;
        bool operator==(const Coordinates& other) const;
        bool operator!=(const Coordinates& other) const;
    };

    double ComputeDistance(Coordinates from, Coordinates to);
}
