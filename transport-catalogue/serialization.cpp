#include "serialization.h"

namespace serialization {

	Serialization::Serialization(transport_catalogue::TransportCatalogue& transport_catalogue, 
	    renderer::MapRenderer& map_renderer, transport_router::TransportRouter& transport_router) :
		transport_catalogue_(transport_catalogue), map_renderer_(map_renderer), 
		transport_router_(transport_router) { };

	void Serialization::SaveTo() {
		std::ofstream output(serialization_settings_.file_name, std::ios::binary);
		
		if (!output) {
			return;
		}

		transport_catalogue_proto::Base base;

		*(base.mutable_transport_catalogue()) = TransportCatalogueToProto();
		*(base.mutable_render_settings()) = RenderSettingsToProto(map_renderer_.GetRenderSettings());
		*(base.mutable_route_settings()) = RouteSettingsToProto(transport_router_.GetRoutingSettings());
	    *(base.mutable_graph()) = GraphToProto();

		base.SerializeToOstream(&output);
	}

	void Serialization::LoadFrom() {
		std::ifstream input(serialization_settings_.file_name, std::ios::binary);
		if (!input) {
			return;
		}

		transport_catalogue_proto::Base base;
		if (!base.ParseFromIstream(&input)) {
			return;
		}

		ProtoToTransportCatalogue(*base.mutable_transport_catalogue());
		ProtoToRenderSettings(*base.mutable_render_settings());
		ProtoToRouteSettings(*base.mutable_route_settings());
		ProtoToGraph(*base.mutable_graph());

	}

	void Serialization::SetSettings(SerializationSettings serialization_settings) {
		serialization_settings_ = serialization_settings;
	}

    // TransportCatalogue ---------------------------------------------------------

	transport_catalogue_proto::TransportCatalogue Serialization::TransportCatalogueToProto() {

		transport_catalogue_proto::TransportCatalogue proto_date;
        
		// SaveStops
		for (const auto& [stop_name, stop_date] : transport_catalogue_.GetAllStops()) {
			transport_catalogue_proto::Coordinates proto_coordinate;
			proto_coordinate.set_lat(stop_date->coordinate.lat);
			proto_coordinate.set_lng(stop_date->coordinate.lng);
			
			transport_catalogue_proto::Stop* proto_stop = proto_date.add_stops();
			proto_stop->set_name(stop_date->name);
            *proto_stop->mutable_coordinate() = std::move(proto_coordinate);
			proto_stop->set_id(stop_date->id);
		}

        // SaveRoutes
		for (const auto& [route_name, route_date] : transport_catalogue_.GetAllRoutes()) {
			transport_catalogue_proto::Route* proto_route = proto_date.add_routes();

            proto_route->set_name(route_date->name);
			proto_route->set_is_circular(route_date->route_type==RouteType::CIRCLE);
			for (const auto& stop : route_date->stops) {
				proto_route->add_id_stops(stop->id);
			}
			proto_route->set_id(route_date->id);
		}

        // SaveDistances
		for (const auto& pair_stops_distance : transport_catalogue_.GetAllDistanceBeetweenPairStops()) {
			transport_catalogue_proto::Distance* proto_distance = proto_date.add_distances();
			proto_distance->set_id_stop_from(pair_stops_distance.id_stop_from);
			proto_distance->set_id_stop_to(pair_stops_distance.id_stop_to);
			proto_distance->set_distance(pair_stops_distance.distance);
		}

		return proto_date;
	}

	void Serialization::ProtoToTransportCatalogue(
		transport_catalogue_proto::TransportCatalogue& proto_catalogue) {

        // LoadStops
		for (int n = 0; n < proto_catalogue.stops_size(); ++n) {
			const geo::Coordinates coordinate = 
			  { proto_catalogue.stops(n).coordinate().lat(),
				proto_catalogue.stops(n).coordinate().lng() };
			transport_catalogue_.AddStop(proto_catalogue.stops(n).name(), coordinate, proto_catalogue.stops(n).id());
		}

        // LoadRoutes
		for (int n = 0; n < proto_catalogue.routes_size(); ++n) {
			const transport_catalogue_proto::Route& proto_route = proto_catalogue.routes(n);

			std::string_view name = proto_route.name();
			RouteType type = (proto_route.is_circular()) ? RouteType::CIRCLE : RouteType::LINEAR;
			std::vector<std::string_view> stops;

			for (int m = 0; m < proto_route.id_stops_size(); ++m) {
				stops.push_back(transport_catalogue_.GetStopNameById(proto_route.id_stops(m)));
			}
			transport_catalogue_.AddRoute(name, type, stops, proto_catalogue.routes(n).id());
		}

        // LoadDistances
		for (int n = 0; n < proto_catalogue.distances_size(); ++n) {
			transport_catalogue_.SetStopDistance(
				transport_catalogue_.GetStopByName(transport_catalogue_.GetStopNameById(proto_catalogue.distances(n).id_stop_from())),
				transport_catalogue_.GetStopByName(transport_catalogue_.GetStopNameById(proto_catalogue.distances(n).id_stop_to())),
			    proto_catalogue.distances(n).distance() );
		}
	}

	// MapRenderer ----------------------------------------------------------------

	map_renderer_proto::Color Serialization::ColorToProto(svg::Color color) {
	    map_renderer_proto::Color proto_color;

	    if (std::holds_alternative<svg::Rgb>(color)) {
		    proto_color.mutable_rgb()->set_red(std::get<svg::Rgb>(color).red);
		    proto_color.mutable_rgb()->set_green(std::get<svg::Rgb>(color).green);
		    proto_color.mutable_rgb()->set_blue(std::get<svg::Rgb>(color).blue);
	    } else if (std::holds_alternative<svg::Rgba>(color)){
		    proto_color.mutable_rgba()->set_red(std::get<svg::Rgba>(color).red);
		    proto_color.mutable_rgba()->set_green(std::get<svg::Rgba>(color).green);
		    proto_color.mutable_rgba()->set_blue(std::get<svg::Rgba>(color).blue);
		    proto_color.mutable_rgba()->set_opacity(std::get<svg::Rgba>(color).opacity);
	    } else if (std::holds_alternative<std::string>(color)){
		    proto_color.set_text(std::get<std::string>(color));
	    }

	    return proto_color;
    }

    map_renderer_proto::RenderSettings Serialization::RenderSettingsToProto(
		const renderer::RenderSettings& render_settings) {
	    map_renderer_proto::RenderSettings proto_settings;

	    proto_settings.set_width(render_settings.width);
	    proto_settings.set_height(render_settings.height);
	    proto_settings.set_padding(render_settings.padding);
	    proto_settings.set_line_width(render_settings.line_width);
	    proto_settings.set_stop_radius(render_settings.stop_radius);
	    proto_settings.set_bus_label_font_size(render_settings.bus_label_font_size);
	    proto_settings.add_bus_label_offset(render_settings.bus_label_offset.x);
	    proto_settings.add_bus_label_offset(render_settings.bus_label_offset.y);
	    proto_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
	    proto_settings.add_stop_label_offset(render_settings.stop_label_offset.x);
	    proto_settings.add_stop_label_offset(render_settings.stop_label_offset.y);
	    proto_settings.set_underlayer_width(render_settings.underlayer_width);

	    *proto_settings.mutable_underlayer_color() = ColorToProto(render_settings.underlayer_color);

	    for (size_t i = 0; i < render_settings.color_palette.size(); ++i){
		    *(proto_settings.add_color_palette()) = ColorToProto(render_settings.color_palette.at(i));
	    }

	    return proto_settings;
    }

    svg::Color Serialization::ProtoToColor(const map_renderer_proto::Color proto_color){
		svg::Color color;
        switch (proto_color.inx_case()) {
            case map_renderer_proto::Color::kText :
                color = proto_color.text();
            break;
            case map_renderer_proto::Color::kRgb :
            {
                auto &p_rgb = proto_color.rgb();
                color = svg::Rgb(p_rgb.red(), p_rgb.green(), p_rgb.blue());
            }
            break;
            case map_renderer_proto::Color::kRgba :
            {
                auto &p_rgba = proto_color.rgba();
                color = svg::Rgba(p_rgba.red(), p_rgba.green(), p_rgba.blue(), p_rgba.opacity());
            }
            break;
            default:
                color = svg::NoneColor;
            break;
        }
        return color;
    }

    void Serialization::ProtoToRenderSettings(const map_renderer_proto::RenderSettings& proto_settings) {
	    renderer::RenderSettings render_settings;

	    render_settings.width = proto_settings.width();
	    render_settings.height = proto_settings.height();
	    render_settings.padding = proto_settings.padding();
	    render_settings.line_width = proto_settings.line_width();
	    render_settings.stop_radius = proto_settings.stop_radius();
	    render_settings.bus_label_font_size = proto_settings.bus_label_font_size();
	    render_settings.bus_label_offset.x = proto_settings.bus_label_offset(0);
	    render_settings.bus_label_offset.y = proto_settings.bus_label_offset(1);
	    render_settings.stop_label_font_size = proto_settings.stop_label_font_size();
	    render_settings.stop_label_offset.x = proto_settings.stop_label_offset(0);
	    render_settings.stop_label_offset.y = proto_settings.stop_label_offset(1);
	    render_settings.underlayer_width = proto_settings.underlayer_width();
	
	    render_settings.underlayer_color = ProtoToColor(proto_settings.underlayer_color());
	
	    render_settings.color_palette.clear();
	    for (int i = 0; i < proto_settings.color_palette_size(); ++i){
		    render_settings.color_palette.push_back(ProtoToColor(proto_settings.color_palette(i)));
	    }

	    map_renderer_.SetRenderSettings(render_settings);
    }

	// TransportRouter ------------------------------------------------------------

	transport_router_proto::RouterSettings Serialization::RouteSettingsToProto(const RoutingSettings& route_settings) {
	    transport_router_proto::RouterSettings proto_settings;

	    proto_settings.set_bus_wait_time(route_settings.bus_wait_time);
	    proto_settings.set_bus_velocity(route_settings.bus_velocity);

	    return proto_settings;
    }

    transport_router_proto::Graph Serialization::GraphToProto() {
	    transport_router_proto::Graph proto_graph;
	    std::shared_ptr<transport_router::Graph> orig_graph = transport_router_.GetGraph();
        
        auto& edges = orig_graph.get()->GetEdges();
        for (size_t n = 0; n < edges.size(); ++n) {
            transport_router_proto::Edge* s_edge = proto_graph.add_edges();
            s_edge->set_from(edges.at(n).from);
            s_edge->set_to(edges.at(n).to);
            s_edge->set_weight(edges.at(n).weight);
			s_edge->set_span_count(edges.at(n).span_count);
			s_edge->set_bus_name_id(edges.at(n).bus_name_id);
        }
		auto& incidence_lists = orig_graph.get()->GetIncidenceLists();
        for (size_t n = 0; n < incidence_lists.size(); ++n) {
            transport_router_proto::IncidenceList* new_list = proto_graph.add_incidence_lists();
            for (size_t m = 0; m < incidence_lists.at(n).size(); ++m) {
                new_list->add_edge_id(incidence_lists.at(n).at(m));;
            }
        }
		
        return proto_graph;
    }

    void Serialization::ProtoToRouteSettings(const transport_router_proto::RouterSettings& proto_settings) {
        transport_router_.SetRoutingSettings(proto_settings.bus_wait_time(),
		    proto_settings.bus_velocity());
		transport_router_.InitializeGraph();
    }

    void Serialization::ProtoToGraph(const transport_router_proto::Graph& proto_graph) {
	
        std::vector<graph::Edge<double>> edges(proto_graph.edges_size());
        std::vector<std::vector<graph::EdgeId>> incidence_lists(proto_graph.incidence_lists_size());
        for (size_t n = 0; n < edges.size(); ++n) {
            const transport_router_proto::Edge& e = proto_graph.edges(n);
            edges[n] = { static_cast<size_t>(e.from()), static_cast<size_t>(e.to()), e.weight(),
			e.span_count(), e.bus_name_id()};
        }
        for (size_t n = 0; n < incidence_lists.size(); ++n) {
            const transport_router_proto::IncidenceList& v = proto_graph.incidence_lists(n);
            incidence_lists[n].reserve(v.edge_id_size());
            for (const auto& id : v.edge_id()) {
                incidence_lists[n].push_back(id);
            }
        }
        transport_router_.SetGraph(graph::DirectedWeightedGraph<double>(edges, incidence_lists));
    }

} // namespace serialization