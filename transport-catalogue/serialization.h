#pragma once

#include <filesystem>
#include <fstream> 

#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"
#include "map_renderer.h"
#include "map_renderer.pb.h"
#include "svg.pb.h"
#include "transport_router.h"
#include "graph.pb.h"


namespace serialization {

    using Path = std::filesystem::path;

	struct SerializationSettings {
		Path file_name; // Название файла. Именно в этот файл нужно сохранить сериализованную базу.
	};

	class Serialization {
	public:
		Serialization() = default;
		Serialization(transport_catalogue::TransportCatalogue& transport_catalogue, 
	        renderer::MapRenderer& map_renderer, transport_router::TransportRouter& transport_router);
		void SaveTo();
		void LoadFrom();
		void SetSettings(SerializationSettings serialization_settings);

	private:
		SerializationSettings serialization_settings_;
		transport_catalogue::TransportCatalogue& transport_catalogue_;
		renderer::MapRenderer& map_renderer_;
		transport_router::TransportRouter& transport_router_;
        
		// TransportCatalogue ---------------------------------------------------------
		
		transport_catalogue_proto::TransportCatalogue TransportCatalogueToProto();
		void ProtoToTransportCatalogue(
			transport_catalogue_proto::TransportCatalogue& proto_catalogue);

        // MapRenderer ----------------------------------------------------------------

		map_renderer_proto::Color ColorToProto(svg::Color color);
	    map_renderer_proto::RenderSettings RenderSettingsToProto(const renderer::RenderSettings& render_settings);

	    svg::Color ProtoToColor(const map_renderer_proto::Color proto_color);
	    void ProtoToRenderSettings(const map_renderer_proto ::RenderSettings& proto_settings);

        // TransportRouter ------------------------------------------------------------

		transport_router_proto::RouterSettings RouteSettingsToProto(const RoutingSettings& route_settings);
	    transport_router_proto::Graph GraphToProto();

	    void ProtoToRouteSettings(const transport_router_proto::RouterSettings& proto_settings);
	    void ProtoToGraph(const transport_router_proto::Graph& proto_graph);

	}; // class Serialization

}  // namespace serialization