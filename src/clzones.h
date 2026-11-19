#pragma once
#ifndef CATA_SRC_CLZONES_H
#define CATA_SRC_CLZONES_H

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "game.h"
#include "map_scale_constants.h"
#include "memory_fast.h"
#include "point.h"
#include "translation.h"
#include "translation_cache.h"
#include "type_id.h"

namespace catacurses
{
class window;
}

class JsonObject;
class JsonOut;
class JsonValue;
class input_context;
class item;
class map;
class talker;
class const_talker;
struct construction;

inline const faction_id your_fac( "your_followers" );
const std::string type_fac_hash_str = "__FAC__";

extern const std::vector<zone_type_id> ignorable_zone_types;

class zone_type
{
    private:
        translation name_;
        translation desc_;
        field_type_str_id field_;
    public:

        zone_type_id id;
        std::vector<std::pair<zone_type_id, mod_id>> src;
        bool was_loaded = false;

        zone_type() = default;
        explicit zone_type( const translation &name, const translation &desc,
                            const field_type_str_id &field ) : name_( name ),
            desc_( desc ), field_( field ) {}

        std::string name() const;
        std::string desc() const;
        field_type_str_id get_field() const;

        bool can_be_personal = false;
        bool hidden = false;

        static void load_zones( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void reset();
        void load( const JsonObject &jo, std::string_view );



        static const std::vector<zone_type> &get_all();
        bool is_valid() const;
};

class zone_options
{
    public:
        virtual ~zone_options() = default;


        static shared_ptr_fast<zone_options> create( const zone_type_id &type );


        static bool is_valid( const zone_type_id &type, const zone_options &options );


        virtual bool has_options() const {
            return false;
        }



        virtual bool query_at_creation() {
            return true;
        }



        virtual bool query() {
            return false;
        }


        virtual std::string get_zone_name_suggestion() const {
            return "";
        }


        virtual std::vector<std::pair<std::string, std::string>> get_descriptions() const {
            return std::vector<std::pair<std::string, std::string>>();
        }

        virtual void serialize( JsonOut & ) const {}
        virtual void deserialize( const JsonObject & ) {}
};


class mark_option
{
    public:
        virtual ~mark_option() = default;

        virtual std::string get_mark() const = 0;
};

class plot_options : public zone_options, public mark_option
{
    private:
        itype_id mark;
        itype_id seed;
        itype_id fertilizer;

        enum query_seed_result {
            canceled,
            successful,
            changed,
        };

        query_seed_result query_seed();
        bool query_fertilizer();

    public:
        std::string get_mark() const override {
            return mark.str();
        }
        itype_id get_seed() const {
            return seed;
        }
        itype_id get_fertilizer() const {
            return fertilizer;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};

class blueprint_options : public zone_options, public mark_option
{
    private:

        std::string mark;
        construction_group_str_id group = construction_group_str_id::NULL_ID();
        construction_id index;

        enum query_con_result {
            canceled,
            successful,
            changed,
        };

        query_con_result query_con();

    public:
        blueprint_options() = default;
        blueprint_options( std::string mark, construction_group_str_id const &group,
                           construction_id const &index )
            : mark( std::move( mark ) ), group( group ), index( index ) {
        }

        std::string get_mark() const override {
            return mark;
        }
        construction_id get_index() const {
            return index;
        }

        bool has_options() const override {
            return true;
        }

        construction_id get_final_construction(
            const std::vector<construction> &list_constructions,
            const construction_id &idx,
            std::set<construction_id> &skip_index );

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};

class ignorable_options : public zone_options
{
    private:
        bool ignore_contents;

        enum query_ignorable_result {
            canceled,
            successful,
            changed,
        };

        query_ignorable_result query_ignorable();

    public:
        bool get_ignore_contents() const {
            return ignore_contents;
        }
        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;

};

class loot_options : public zone_options, public mark_option
{
    private:

        std::string mark;

        enum query_loot_result {
            canceled,
            successful,
            changed,
        };

        query_loot_result query_loot();

    public:
        std::string get_mark() const override {
            return mark;
        }

        void set_mark( std::string const &nmark ) {
            mark = nmark;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};

class unload_options : public zone_options, public mark_option
{
    private:

        std::string mark;
        bool mods;
        bool molle;
        bool sparse_only;
        int sparse_threshold = 20;
        bool always_unload;

        enum query_unload_result {
            canceled,
            successful,
            changed,
        };

        query_unload_result query_unload();

    public:
        std::string get_mark() const override {
            return mark;
        }

        bool unload_mods() const {
            return mods;
        }

        bool unload_molle() const {
            return molle;
        }

        bool unload_sparse_only() const {
            return sparse_only;
        }

        int unload_sparse_threshold() const {
            return sparse_threshold;
        }

        bool unload_always() const {
            return always_unload;
        }

        void set_mark( std::string const &nmark ) {
            mark = nmark;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};

class study_zone_options : public zone_options
{
    private:

        std::map<std::string, std::set<skill_id>> npc_skill_preferences;

        enum query_study_result {
            canceled,
            successful,
            changed,
        };

        query_study_result query_study_skills();

    public:

        const std::set<skill_id> *get_skill_preferences( const std::string &npc_name ) const;

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};




class zone_data
{
    private:
        std::string name;
        zone_type_id type;
        faction_id faction;
        bool invert;
        bool enabled;

        bool temporarily_disabled; // NOLINT(cata-serialize)
        bool is_vehicle;
        tripoint_abs_ms start;
        tripoint_abs_ms end;

        bool is_personal;
        tripoint_rel_ms personal_start;
        tripoint_rel_ms personal_end;

        tripoint_abs_ms cached_shift;
        shared_ptr_fast<zone_options> options;
        bool is_displayed;

    public:
        zone_data() {
            type = zone_type_id( "" );
            invert = false;
            enabled = false;
            temporarily_disabled = false;
            is_vehicle = false;
            is_personal = false;
            start = tripoint_abs_ms::zero;
            end = tripoint_abs_ms::zero;
            personal_start = tripoint_rel_ms::zero;
            personal_end = tripoint_rel_ms::zero;
            cached_shift = tripoint_abs_ms::zero;
            options = nullptr;
            is_displayed = false;
        }

        zone_data( const std::string &_name, const zone_type_id &_type, const faction_id &_faction,
                   bool _invert, const bool _enabled,
                   const shared_ptr_fast<zone_options> &_options = nullptr,
                   bool _is_displayed = false ) {
            name = _name;
            type = _type;
            faction = _faction;
            invert = _invert;
            enabled = _enabled;
            temporarily_disabled = false;
            is_vehicle = false;
            is_displayed = _is_displayed;


            if( _options == nullptr || !zone_options::is_valid( type, *_options ) ) {
                options = zone_options::create( type );
            } else {
                options = _options;
            }
        }

        zone_data( const std::string &_name, const zone_type_id &_type, const faction_id &_faction,
                   bool _invert, const bool _enabled,
                   const tripoint_abs_ms &_start, const tripoint_abs_ms &_end,
                   const shared_ptr_fast<zone_options> &_options = nullptr,
                   bool _is_displayed = false )
            : zone_data( _name, _type, _faction, _invert, _enabled, _options, _is_displayed ) {
            is_personal = false;
            start = _start;
            end = _end;
        }

        zone_data( const std::string &_name, const zone_type_id &_type, const faction_id &_faction,
                   bool _invert, const bool _enabled,
                   const tripoint_rel_ms &_start, const tripoint_rel_ms &_end,
                   const shared_ptr_fast<zone_options> &_options = nullptr,
                   bool _is_displayed = false )
            : zone_data( _name, _type, _faction, _invert, _enabled, _options, _is_displayed ) {
            is_personal = true;
            personal_start = _start;
            personal_end = _end;
        }


        bool set_name();

        bool set_type();



        void set_position( const std::pair<tripoint_abs_ms, tripoint_abs_ms> &position, bool manual = true,
                           bool update_avatar = true, bool skip_cache_update = false, bool suppress_display_update = false );
        void set_position( const std::pair<tripoint_rel_ms, tripoint_rel_ms> &position, bool manual = true,
                           bool update_avatar = true, bool skip_cache_update = false, bool suppress_display_update = false );
        void set_enabled( bool enabled_arg );
        void set_temporary_disabled( bool enabled_arg );



        void refresh_display() const;
        void toggle_display();
        void set_is_vehicle( bool is_vehicle_arg );

        static std::string make_type_hash( const zone_type_id &_type, const faction_id &_fac ) {
            return _type.c_str() + type_fac_hash_str + _fac.c_str();
        }
        static zone_type_id unhash_type( const std::string_view hash_type ) {
            size_t end = hash_type.find( type_fac_hash_str );
            if( end != std::string::npos && end < hash_type.size() ) {
                return zone_type_id( hash_type.substr( 0, end ) );
            }
            return zone_type_id( "" );
        }
        static faction_id unhash_fac( const std::string_view hash_type ) {
            size_t start = hash_type.find( type_fac_hash_str ) + type_fac_hash_str.size();
            if( start != std::string::npos ) {
                return faction_id( hash_type.substr( start ) );
            }
            return faction_id( "" );
        }
        std::string get_name() const {
            return name;
        }
        const faction_id &get_faction() const {
            return faction;
        }
        std::string get_type_hash() const {
            return make_type_hash( type, faction );
        }
        const zone_type_id &get_type() const {
            return type;
        }
        bool get_invert() const {
            return invert;
        }
        bool get_enabled() const {
            return enabled;
        }
        bool get_temporarily_disabled() const {
            return temporarily_disabled;
        }

        bool get_is_displayed() const {
            return is_displayed;
        }

        bool get_is_vehicle() const {
            return is_vehicle;
        }
        bool get_is_personal() const {
            return is_personal;
        }
        tripoint_abs_ms get_start_point() const {
            if( is_personal ) {
                return cached_shift + personal_start;
            }
            return start;
        }
        tripoint_abs_ms get_end_point() const {
            if( is_personal ) {
                return cached_shift + personal_end;
            }
            return end;
        }
        void update_cached_shift( const tripoint_abs_ms &player_loc ) {
            cached_shift = player_loc;
        }
        tripoint_abs_ms get_center_point() const;
        bool has_options() const {
            return options->has_options();
        }
        const zone_options &get_options() const {
            return *options;
        }
        zone_options &get_options() {
            return *options;
        }




        bool has_inside( const tripoint_abs_ms &p ) const {

            if( is_personal ) {
                return inclusive_cuboid<tripoint_abs_ms>(
                           cached_shift + personal_start, cached_shift + personal_end ).contains( p );
            }
            return inclusive_cuboid<tripoint_abs_ms>( start, end ).contains( p );
        }
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
};

class zone_manager
{
    public:
        using ref_zone_data = std::reference_wrapper<zone_data>;
        using ref_const_zone_data = std::reference_wrapper<const zone_data>;

    private:
        static const int MAX_DISTANCE = MAX_VIEW_DISTANCE;
        std::vector<zone_data> zones;


        std::vector<zone_data *> added_vzones; // NOLINT(cata-serialize)

        std::vector<std::pair<zone_data, zone_data *>> changed_vzones; // NOLINT(cata-serialize)

        std::vector<zone_data> removed_vzones; // NOLINT(cata-serialize)

        std::map<zone_type_id, zone_type> types; // NOLINT(cata-serialize)


        int num_personal_zones = 0; // NOLINT(cata-serialize)

        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map<std::string, std::unordered_set<tripoint_abs_ms>> area_cache;
        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map<std::string, std::unordered_set<tripoint_abs_ms>> vzone_cache;
        std::unordered_set<tripoint_abs_ms> get_point_set( const zone_type_id &type,
                const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint_abs_ms> get_vzone_set( const zone_type_id &type,
                const faction_id &fac = your_fac ) const;




        // NOLINTNEXTLINE(cata-serialize)
        mutable std::unordered_map<std::string, std::function<bool( const item & )>> filter_cache;
        mutable int filter_cache_lang_version = INVALID_LANGUAGE_VERSION; // NOLINT(cata-serialize)

        const std::function<bool( const item & )> &cached_item_filter(
            const std::string &filter ) const;
    public:
        zone_manager();
        ~zone_manager() = default;
        zone_manager( zone_manager && ) = default;
        zone_manager( const zone_manager & ) = default;
        zone_manager &operator=( zone_manager && ) = default;
        zone_manager &operator=( const zone_manager & ) = default;

        static zone_manager &get_manager() {
            static zone_manager manager;
            return manager;
        }

        void clear();


        void add( const std::string &name, const zone_type_id &type, const faction_id &faction,
                  bool invert, bool enabled,
                  const tripoint_abs_ms &start, const tripoint_abs_ms &end,
                  const shared_ptr_fast<zone_options> &options = nullptr,
                  bool silent = false, map *pmap = nullptr );


        void add( const std::string &name, const zone_type_id &type, const faction_id &faction,
                  bool invert, bool enabled,
                  const tripoint_rel_ms &start, const tripoint_rel_ms &end,
                  const shared_ptr_fast<zone_options> &options = nullptr );


        const zone_data *get_zone_at( const tripoint_abs_ms &where, const zone_type_id &type,
                                      const faction_id &fac = your_fac ) const;

        std::vector<zone_data const *> get_zones_at( const tripoint_abs_ms &where, const zone_type_id &type,
                const faction_id &fac = your_fac ) const;
        void create_vehicle_loot_zone( class vehicle &vehicle, const point_rel_ms &mount_point,
                                       zone_data &new_zone, map *pmap = nullptr );

        bool remove( zone_data &zone );

        unsigned int size() const {
            return zones.size();
        }
        const std::map<zone_type_id, zone_type> &get_types() const {
            return types;
        }
        std::string get_name_from_type( const zone_type_id &type ) const;
        bool has_type( const zone_type_id &type ) const;
        bool has_defined( const zone_type_id &type, const faction_id &fac = your_fac ) const;
        void cache_data( bool update_avatar = true );
        void reset_disabled();
        void cache_avatar_location();
        void cache_vzones( map *pmap = nullptr );
        bool has( const zone_type_id &type, const tripoint_abs_ms &where,
                  const faction_id &fac = your_fac ) const;
        bool has_terrain( const zone_type_id &type, const tripoint_abs_ms &where,
                          const faction_id &fac = your_fac ) const;
        bool has_vehicle( const zone_type_id &type, const tripoint_abs_ms &where,
                          const faction_id &fac = your_fac ) const;
        bool has_near( const zone_type_id &type, const tripoint_abs_ms &where,
                       int range = MAX_DISTANCE, const faction_id &fac = your_fac ) const;
        bool has_loot_dest_near( const tripoint_abs_ms &where ) const;


        bool custom_loot_has( const tripoint_abs_ms &where, const item *it,
                              const zone_type_id &ztype, const faction_id &fac = your_fac,
                              std::optional<bool> from_vehicle = std::nullopt,
                              std::unordered_map<const zone_data *, bool> *memo = nullptr ) const;
        std::vector<zone_data const *> get_near_zones( const zone_type_id &type,
                const tripoint_abs_ms &where, int range,
                const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint_abs_ms> get_near(
            const zone_type_id &type, const tripoint_abs_ms &where, int range = MAX_DISTANCE,
            const item *it = nullptr, const faction_id &fac = your_fac ) const;
        std::optional<tripoint_abs_ms> get_nearest(
            const zone_type_id &type, const tripoint_abs_ms &where, int range = MAX_DISTANCE,
            const faction_id &fac = your_fac ) const;
        zone_type_id get_near_zone_type_for_item( const item &it, const tripoint_abs_ms &where,
                int range = MAX_DISTANCE, const faction_id &fac = your_fac ) const;
        std::vector<zone_data> get_zones( const zone_type_id &type, const tripoint_abs_ms &where,
                                          const faction_id &fac = your_fac ) const;
        const zone_data *get_zone_at( const tripoint_abs_ms &where, bool loot_only = false,
                                      const faction_id &fac = your_fac ) const;
        const zone_data *get_bottom_zone( const tripoint_abs_ms &where,
                                          const faction_id &fac = your_fac ) const;
        std::optional<std::string> query_name( const std::string &default_name = "" ) const;
        std::optional<zone_type_id> query_type( bool personal = false ) const;
        void swap( zone_data &a, zone_data &b );
        void rotate_zones( map &target_map, int turns );

        std::unordered_set<tripoint_bub_ms> get_point_set_loot(
            const tripoint_abs_ms &where, int radius, const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint_bub_ms> get_point_set_loot(
            const tripoint_abs_ms &where, int radius, bool npc_search,
            const faction_id &fac = your_fac ) const;


        std::vector<ref_zone_data> get_zones( const faction_id &fac = your_fac );
        std::vector<ref_const_zone_data> get_zones( const faction_id &fac = your_fac ) const;

        bool has_personal_zones() const;


        bool has_nonpersonal( const zone_type_id &type, const tripoint_abs_ms &where,
                              const faction_id &fac = your_fac ) const;

        bool save_zones( std::string const &suffix = {} );
        bool save_world_zones( std::string const &suffix = {} );
        void load_zones( std::string const &suffix = {} );
        void load_world_zones( std::string const &suffix = {} );
        void zone_edited( zone_data &zone );
        void revert_vzones();
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonValue &jv );
};




class zone_manager_ui
{
    public:
        static void display_zone_manager();
        static shared_ptr_fast<game::draw_callback_t> create_zone_callback(
            const std::optional<tripoint_bub_ms> &zone_start,
            const std::optional<tripoint_bub_ms> &zone_end,
            const bool &zone_blink, const bool &zone_cursor, const bool &is_moving_zone = false );
    private:
        static void zones_manager_draw_borders( const catacurses::window &w_border,
                                                const catacurses::window &w_info_border,
                                                int iInfoHeight, int width );
        static void zones_manager_shortcuts( const catacurses::window &w_info, faction_id const &faction,
                                             bool show_all_zones, const input_context &ctxt, int width );
};

void mapgen_place_zone( tripoint_abs_ms const &start, tripoint_abs_ms const &end,
                        zone_type_id const &type,
                        faction_id const &fac = your_fac, std::string const &name = {},
                        std::string const &filter = {}, map *pmap = nullptr );
std::unique_ptr<talker> get_talker_for( zone_data &me );
std::unique_ptr<const_talker> get_talker_for( const zone_data &me );
std::unique_ptr<talker> get_talker_for( zone_data *me );
#endif
