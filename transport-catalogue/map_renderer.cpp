#include "map_renderer.h"

namespace renderer
{
    using namespace std::literals;

    // SphereProjector ---------------------------------------------------------------------------------

    bool IsZero(const double& value)
    {
        return std::abs(value) < EPSILON;
    }

    svg::Point SphereProjector::operator()(const geo::Coordinates& coords) const
    {
        return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                 (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
    }

    // MapRenderer -------------------------------------------------------------------------------------

    MapRenderer::MapRenderer() {
    }

    MapRenderer::MapRenderer(const RenderSettings& render_settings) : render_settings_(render_settings) {
        size_palette_ = render_settings.color_palette.size();
    }

    void MapRenderer::SetRenderSettings(const RenderSettings& render_settings)
    {
        render_settings_ = render_settings;
        size_palette_ = render_settings_.color_palette.size();
    }

    const RenderSettings& MapRenderer::GetRenderSettings() const
    {
        return render_settings_;
    }

    void MapRenderer::ResetColorCount() const {
        const_cast<MapRenderer*>(this)->color_count_ = 0;
    }

    void MapRenderer::ChangeCountColor() {
        ++color_count_;
        if (color_count_ < size_palette_) {
            return;
        }
        color_count_ = 0;
    }

    std::pair<svg::Text, svg::Text> MapRenderer::FillingText(std::string route_name, svg::Point point) const {
        svg::Text text_1;
        svg::Text text_2;
        text_1.SetPosition(point).SetOffset(render_settings_.bus_label_offset).
            SetFontSize(render_settings_.bus_label_font_size).SetFontFamily("Verdana"s).
            SetFontWeight("bold"s).SetData(route_name).
            SetFillColor(render_settings_.underlayer_color).SetStrokeColor(render_settings_.underlayer_color).
            SetStrokeWidth(render_settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).
            SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        text_2.SetPosition(point).SetOffset(render_settings_.bus_label_offset).
            SetFontSize(render_settings_.bus_label_font_size).SetFontFamily("Verdana"s).
            SetFontWeight("bold"s).SetData(route_name).SetFillColor(render_settings_.color_palette[color_count_]);
        return { text_1 , text_2 };
    }

    std::optional<svg::Polyline> MapRenderer::CreateRouteLine(const Route* route, const SphereProjector& sphere_proj) const {
        svg::Polyline polyline;
        if (route->stops.empty()) {
            return std::nullopt;
        }
        for (const Stop* stop : route->stops) {
            svg::Point point = sphere_proj(stop->coordinate);
            polyline.AddPoint(point).SetStrokeColor(render_settings_.color_palette[color_count_]).SetFillColor(svg::NoneColor).
                SetStrokeWidth(render_settings_.line_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).
                SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        }
        if (route->route_type == RouteType::CIRCLE) {
            const_cast<MapRenderer*>(this)->ChangeCountColor();
            return polyline;
        }
        for (auto it_back = route->stops.rbegin() + 1; it_back != route->stops.rend(); ++it_back) {
            svg::Point point = sphere_proj((*it_back)->coordinate);
            polyline.AddPoint(point).SetStrokeColor(render_settings_.color_palette[color_count_]).SetFillColor(svg::NoneColor).
                SetStrokeWidth(render_settings_.line_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).
                SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        }
        const_cast<MapRenderer*>(this)->ChangeCountColor();
        return polyline;
    }

    std::vector<std::pair<svg::Text, svg::Text>> MapRenderer::CreateRouteName(const Route* route, const SphereProjector& sphere_proj) const {
        if (route->stops.empty()) {
            return {};
        }
        svg::Point point = sphere_proj(route->stops.front()->coordinate);
        auto [text_1, text_2] = FillingText(route->name, point);
        if ((route->route_type == RouteType::CIRCLE) || (route->stops.size() == 1) || (route->stops.front() == route->stops.back())) {
            const_cast<MapRenderer*>(this)->ChangeCountColor();
            return { { text_1, text_2 } };
        }
        std::vector<std::pair<svg::Text, svg::Text>> result;
        result.push_back({ text_1, text_2 });
        point = sphere_proj(route->stops.back()->coordinate);
        auto [text_1_, text_2_] = FillingText(route->name, point);
        result.push_back({ text_1_, text_2_ });
        const_cast<MapRenderer*>(this)->ChangeCountColor();
        return result;
    }

    svg::Circle MapRenderer::CreateStopsSymbol(const Stop* stop, const SphereProjector& sphere_proj) const {
        svg::Circle circle;
        svg::Point point = sphere_proj(stop->coordinate);
        circle.SetCenter(point).SetRadius(render_settings_.stop_radius).SetFillColor("white");
        return circle;
    }

    std::pair<svg::Text, svg::Text> MapRenderer::CreateStopsName(const Stop* stop, const SphereProjector& sphere_proj) const {
        svg::Text text_1;
        svg::Text text_2;
        svg::Point point = sphere_proj(stop->coordinate);
        text_1.SetPosition(point).SetOffset(render_settings_.stop_label_offset).
            SetFontSize(render_settings_.stop_label_font_size).SetFontFamily("Verdana"s).
            SetData(stop->name).SetFillColor(render_settings_.underlayer_color).SetStrokeColor(render_settings_.underlayer_color).
            SetStrokeWidth(render_settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).
            SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        text_2.SetPosition(point).SetOffset(render_settings_.stop_label_offset).
            SetFontSize(render_settings_.stop_label_font_size).SetFontFamily("Verdana"s).
            SetData(stop->name).SetFillColor("black");
        return { text_1, text_2 };
    }
   
    svg::Document MapRenderer::CreateMap(const transport_catalogue::TransportCatalogue& catalogue) const {
        std::set<std::string_view> stops_names;
        std::set<std::string_view> buses_names;
        std::vector<geo::Coordinates> coordinates_stops;
        std::unordered_map<std::string_view, const Route*> all_buses = catalogue.GetAllRoutes();
        for (const auto& [bus_name, struct_bus] : all_buses) {
            buses_names.insert(bus_name);
            for (auto stop : struct_bus->stops) {
                auto [_, status] = stops_names.insert(stop->name);
                if (status) {
                    coordinates_stops.push_back(stop->coordinate);
                }
            }
        }
        renderer::SphereProjector sphere_proj(coordinates_stops.begin(), coordinates_stops.end(),
                   render_settings_.width, render_settings_.height, render_settings_.padding);
        svg::Document doc;
        for (std::string_view bus : buses_names) {
            std::optional<svg::Polyline> poly = this->CreateRouteLine(all_buses.at(bus), sphere_proj);
            if (poly == std::nullopt) {
                continue;
            }
            doc.Add(*poly);
        }
        this->ResetColorCount();
        for (std::string_view bus : buses_names) {
            auto text = this->CreateRouteName(all_buses.at(bus), sphere_proj);
            if (text.empty()) {
                continue;
            }
            doc.Add(text[0].first);
            doc.Add(text[0].second);
            if (text.size() == 2) {
                doc.Add(text[1].first);
                doc.Add(text[1].second);
            }
        }
        const std::unordered_map <std::string_view, const Stop*>& all_stops = catalogue.GetAllStops();
        for (std::string_view stop : stops_names) {
            svg::Circle circle = this->CreateStopsSymbol(all_stops.at(stop), sphere_proj);
            doc.Add(circle);
        }
        for (std::string_view stop : stops_names) {
            auto text = this->CreateStopsName(all_stops.at(stop), sphere_proj);
            doc.Add(text.first);
            doc.Add(text.second);
        }
        return doc;
    }

} // namespace renderer
