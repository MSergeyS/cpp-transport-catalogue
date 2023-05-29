#pragma once

#include <vector>
#include <algorithm>

#include "domain.h"
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace renderer
{
    //----------------RenderSettings----------------------------

    struct RenderSettings
    {
        double width = 0.0;
        double height = 0.0;
        double padding = 0.0;
        double line_width = 0.0;
        double stop_radius = 0.0;
        int bus_label_font_size = 0;
        svg::Point bus_label_offset{ 0.0, 0.0 };
        int stop_label_font_size = 0;
        svg::Point stop_label_offset{ 0.0, 0.0 };
        svg::Color underlayer_color;
        double underlayer_width = 0.0;
        std::vector<svg::Color> color_palette;
    };

    inline const double EPSILON = 1e-6;

    bool IsZero(const double& value);

    //----------------SphereProjector----------------------------

    //класс, который проецирует точки на карту
    class SphereProjector
    {
    public:
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template <typename PointInputIt>
        SphereProjector(const PointInputIt& points_begin, const PointInputIt& points_end,
                const double& max_width, const double& max_height, const double& padding);

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(const geo::Coordinates& coords) const;

    private:

        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    template<typename PointInputIt>
    inline SphereProjector::SphereProjector(const PointInputIt& points_begin, const PointInputIt& points_end,
            const double& max_width, const double& max_height, const double& padding)
        : padding_(padding) {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lng < rhs.lng;
            });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lat < rhs.lat;
            });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }

        else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        }


        else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

 
    //---------------MapRenderer-------------------------

    class MapRenderer
    {
    public:
        MapRenderer();
        MapRenderer(const RenderSettings& render_settings);

        void SetRenderSettings(const RenderSettings& render_settings);
        const RenderSettings& GetRenderSettings() const;

        std::optional<svg::Polyline> CreateRouteLine(const Route* route, const SphereProjector& sphere_proj) const;
        std::vector<std::pair<svg::Text, svg::Text>> CreateRouteName(const Route* route, const SphereProjector& sphere_proj) const;
        svg::Circle CreateStopsSymbol(const Stop* stop, const SphereProjector& sphere_proj) const;
        std::pair<svg::Text, svg::Text> CreateStopsName(const Stop* stop, const SphereProjector& sphere_proj) const;

        svg::Document CreateMap(const transport_catalogue::TransportCatalogue& catalogue) const;
        void ResetColorCount() const;
    private:
        void ChangeCountColor();
        std::pair<svg::Text, svg::Text> FillingText(std::string route_name, svg::Point point) const;

        RenderSettings render_settings_;
        size_t color_count_ = 0;
        size_t size_palette_ = 0;
    };

} // namespace renderer

//program.exe < input.json
