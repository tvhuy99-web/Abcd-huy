#pragma once
#ifndef CATA_SRC_TILESET_LOADER_H
#define CATA_SRC_TILESET_LOADER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "atlas_bake_plan.h"
#include "cata_tiles.h"
#include "point.h"
#include "sdl_wrappers.h"

class JsonObject;
class cata_path;
template <typename T> struct weighted_int_list;

class tileset_cache::loader
{
    public:



        struct tile_value_targets {
            std::vector<texture> *normal = nullptr;
            std::vector<texture> *shadow = nullptr;
            std::vector<texture> *night = nullptr;
            std::vector<texture> *overexposed = nullptr;
            std::vector<texture> *memory = nullptr;
            std::vector<texture> *silhouette = nullptr;
        };

    private:
        tileset &ts;
        const SDL_Renderer_Ptr &renderer;
        std::string memory_map_mode;
        uint64_t filter_fingerprint = 0;
        atlas_bake_plan bake_plan;


        int texture_count = 0;
        uint64_t atlas_payload_bytes = 0;

        point sprite_offset;
        point sprite_offset_retracted;
        float sprite_pixelscale = 1.0;

        int sprite_width = 0;
        int sprite_height = 0;

        int offset = 0;
        int sprite_id_offset = 0;
        int size = 0;

        int R = 0;
        int G = 0;
        int B = 0;

        int tile_atlas_width = 0;



        static std::shared_ptr<SDL_Texture> make_gated_atlas_texture(
            SDL_Texture_Ptr tex, atlas_replay_quarantine::gate abandon_gate );


        void copy_surface_to_texture( const SDL_Surface_Ptr &surf, const point &offset,
                                      std::vector<texture> &target,
                                      const std::vector<SDL_Rect> &opaque_bounds,
                                      const atlas_replay_quarantine::gate &abandon_gate );
        void create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas, const point &offset,
                                              const tile_value_targets &targets,
                                              const atlas_replay_quarantine::gate &abandon_gate );

        void process_variations_after_loading( weighted_int_list<std::vector<int>> &v ) const;

        void add_ascii_subtile( tile_type &curr_tile, const std::string &t_id, int sprite_id,
                                const std::string &s_id );
        void load_ascii_set( const JsonObject &entry );



        tile_type &load_tile( const JsonObject &entry, const std::string &id );

        void load_tile_spritelists( const JsonObject &entry, weighted_int_list<std::vector<int>> &vs,
                                    std::string_view objname ) const;

        void load_ascii( const JsonObject &config );



        void read_image_dimensions( const cata_path &img_path, int kr, int kg, int kb );



        void parse_mappings( const JsonObject &config );





        void parse_atlases( const JsonObject &config, const cata_path &tileset_root,
                            const cata_path &img_path, bool pump_events );


        void load_layers( const JsonObject &config );





        atlas_upload_interrupt upload_one_atlas( const atlas_replay_descriptor &desc,
                const tile_value_targets &targets,
                bool pump_events,
                const atlas_replay_quarantine::gate &abandon_gate,
                const atlas_upload_poll &poll );

    public:
        loader( tileset &ts, const SDL_Renderer_Ptr &r, std::string memory_map_mode,
                const uint64_t filter_fingerprint )
            : ts( ts ), renderer( r ), memory_map_mode( std::move( memory_map_mode ) ),
              filter_fingerprint( filter_fingerprint ) {
        }







        atlas_upload_interrupt load( const std::string &tileset_id, bool precheck,
                                     bool pump_events = false, bool terrain = false,
                                     uint64_t renderer_instance_generation = 0,
                                     uint64_t gpu_textures_generation = 0,
                                     const atlas_upload_poll &poll = {},
                                     atlas_replay_quarantine *quarantine = nullptr );











        static atlas_upload_interrupt upload_atlases( tileset &ts, const SDL_Renderer_Ptr &renderer,
                const std::string &memory_map_mode,
                uint64_t filter_fingerprint,
                const atlas_bake_plan &plan,
                const std::vector<atlas_replay_descriptor> &descriptors,
                uint64_t renderer_instance_generation,
                uint64_t gpu_textures_generation,
                bool pump_events,
                const atlas_upload_poll &poll = {},
                atlas_replay_quarantine *quarantine = nullptr );
};

#endif
