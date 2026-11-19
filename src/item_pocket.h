#pragma once
#ifndef CATA_SRC_ITEM_POCKET_H
#define CATA_SRC_ITEM_POCKET_H

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "enums.h"
#include "flat_set.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"

class Character;
class JsonArray;
class JsonObject;
class JsonOut;
class item;
class item_location;
class map;
class pocket_data;
struct iteminfo;

class item_pocket
{
    public:
        enum class contain_code : int {
            SUCCESS,

            ERR_MOD,

            ERR_LIQUID,

            ERR_GAS,

            ERR_TOO_BIG,

            ERR_TOO_HEAVY,

            ERR_TOO_SMALL,

            ERR_NO_SPACE,

            ERR_CANNOT_SUPPORT,

            ERR_FLAG,

            ERR_AMMO
        };

        class favorite_settings
        {
            public:
                void clear();

                void set_priority( int priority );
                int priority() const {
                    return priority_rating;
                }


                bool is_null() const;

                void whitelist_item( const itype_id &id );
                void blacklist_item( const itype_id &id );
                void clear_item( const itype_id &id );

                const cata::flat_set<itype_id> &get_item_whitelist() const;
                const cata::flat_set<itype_id> &get_item_blacklist() const;
                const cata::flat_set<item_category_id> &get_category_whitelist() const;
                const cata::flat_set<item_category_id> &get_category_blacklist() const;

                void whitelist_category( const item_category_id &id );
                void blacklist_category( const item_category_id &id );
                void clear_category( const item_category_id &id );


                bool accepts_item( const item &it ) const;


                bool is_collapsed() const;

                void set_collapse( bool );


                bool is_disabled() const;
                void set_disabled( bool );


                bool is_unloadable() const;
                void set_unloadable( bool );

                const std::optional<std::string> &get_preset_name() const;
                void set_preset_name( const std::string & );

                void set_was_edited();
                bool was_edited() const;

                void info( std::vector<iteminfo> &info ) const;

                void serialize( JsonOut &json ) const;
                void deserialize( const JsonObject &data );
            private:
                std::optional<std::string> preset_name;
                int priority_rating = 0;
                cata::flat_set<itype_id> item_whitelist;
                cata::flat_set<itype_id> item_blacklist;
                cata::flat_set<item_category_id> category_whitelist;
                cata::flat_set<item_category_id> category_blacklist;
                bool collapsed = false;
                bool disabled = false;
                bool unload = true;
                bool player_edited = false;
        };

        item_pocket() = default;
        explicit item_pocket( const pocket_data *data ) : data( data ) {}

        bool stacks_with( const item_pocket &rhs, int depth = 0, int maxdepth = 2 ) const;
        bool is_funnel_container( units::volume &bigger_than ) const;
        bool is_restricted() const;
        bool has_any_with( const std::function<bool( const item & )> &filter ) const;

        bool is_type( pocket_type ptype ) const;
        bool is_type( const std::set<pocket_type> &ptype ) const;
        bool is_ablative() const;
        bool is_holster() const;

        bool holster_full() const;
        bool empty() const;
        bool full( bool allow_bucket ) const;


        bool rigid() const;
        bool watertight() const;
        bool airtight() const;
        bool inherits_flags() const;

        bool transparent() const;

        bool allows_speedloader( const itype_id &speedloader_id ) const;



        bool is_standard_type() const;

        bool is_container_like_type() const;

        bool is_forbidden() const;

        const translation &get_description() const;
        const translation &get_name() const;

        const pocket_data *get_pocket_data() const;

        std::list<item *> all_items_top();
        std::list<const item *> all_items_top() const;
        std::list<item *> all_items_ptr( pocket_type pk_type );
        std::list<const item *> all_items_ptr( pocket_type pk_type ) const;



        units::ememory occupied_ememory() const;

        item &back();
        const item &back() const;
        item &front();
        const item &front() const;

        size_t size() const;
        void pop_back();






        ret_val<contain_code> is_compatible( const item &it ) const;







        ret_val<contain_code> can_contain( const item &it, bool ignore_contents = false ) const;
        ret_val<contain_code> can_contain( const item &it, int &copies_remaining,
                                           bool ignore_contents = false ) const;

        bool can_contain_liquid( bool held_or_ground ) const;
        bool contains_phase( phase_id phase ) const;






        bool can_reload_with( const item &ammo, bool now ) const;

        units::length max_containable_length() const;
        units::length min_containable_length() const;

        units::volume volume_capacity() const;

        units::volume contents_volume() const;
        units::volume remaining_volume() const;






        void begin_bulk_fill();
        void end_bulk_fill();

        int remaining_capacity_for_item( const item &it ) const;

        units::volume magazine_well() const;

        units::mass weight_capacity() const;

        units::mass contains_weight() const;
        units::mass remaining_weight() const;



        int charges_per_remaining_volume( const item &it ) const;
        int charges_per_remaining_weight( const item &it ) const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;
        units::length item_length_modifier() const;


        float spoil_multiplier() const;

        float insulation() const;

        int moves() const;

        int best_quality( const quality_id &id ) const;


        void heat_up();

        std::vector<item *> gunmods();

        std::vector<const item *> gunmods() const;
        cata::flat_set<itype_id> item_type_restrictions() const;
        item *magazine_current();
        const item *magazine_current() const;

        itype_id magazine_default() const;

        int ammo_consume( int qty );

        std::set<ammotype> ammo_types() const;
        int ammo_capacity( const ammotype &ammo ) const;
        int remaining_ammo_capacity( const ammotype &ammo ) const;
        void casings_handle( const std::function<bool( item & )> &func );
        bool use_amount( item_location &parent, const itype_id &it, int &quantity, std::list<item> &used );
        bool will_explode_in_a_fire() const;
        bool item_has_uses_recursive() const;

        bool will_spill() const;
        bool will_spill_if_unsealed() const;

        bool seal();

        void unseal();




        bool sealable() const;





        bool sealed() const;

        std::string translated_sealed_prefix() const;
        bool detonate( const tripoint_bub_ms &p, std::vector<item> &drops );

        void remove_all_ammo( Character &guy );
        void remove_all_mods( Character &guy );


        std::optional<item> remove_item( const item &it );
        std::optional<item> remove_item( const item_location &it );

        void overflow( map &here, const tripoint_bub_ms &pos, const item_location &loc );
        bool spill_contents( const tripoint_bub_ms &pos );
        bool spill_contents( map *here, const tripoint_bub_ms &pos );
        void on_pickup( Character &guy, item *avoid = nullptr );
        void on_contents_changed();
        void handle_liquid_or_spill( Character &guy, const item *avoid = nullptr );
        void clear_items();
        bool has_item( const item &it ) const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        const item *get_item_with( const std::function<bool( const item & )> &filter ) const;
        void remove_items_if( const std::function<bool( item & )> &filter );




        void process( map &here, Character *carrier, const tripoint_bub_ms &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f,
                      bool watertight_container = false );

        void leak( map &here, Character *carrier, const tripoint_bub_ms &pos,
                   item_pocket *pocke = nullptr );

        pocket_type saved_type() const {
            return _saved_type;
        }

        bool saved_sealed() const {
            return _saved_sealed;
        }

        float get_capacity_mult() const {
            return capacity_mult;
        }
        void set_capacity_mult( float m ) {
            capacity_mult = m;
        }


        ret_val<item *> insert_item( const item &it, bool into_bottom = false,
                                     bool restack_charges = true, bool ignore_contents = false );




        void add( const item &it, item **ret = nullptr );
        void add( const item &it, int copies, std::vector<item *> &added );
        bool can_unload_liquid() const;

        int fill_with( const item &contained, Character &guy, int amount = 0,
                       bool allow_unseal = false, bool ignore_settings = false );











        std::pair<item_location, item_pocket *> best_pocket_in_contents(
            item_location &this_loc, const item &it, const item *avoid,
            bool allow_sealed, bool ignore_settings );


        std::list<item> &edit_contents();



        int obtain_cost( const item &it ) const;


        bool remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );


        VisitResponse visit_contents( const std::function<VisitResponse( const item_location & )> &func,
                                      const item_location &parent, const std::set<pocket_type> &allowed_pockets );
        VisitResponse visit_contents_legacy( const std::function<VisitResponse( item *, item * )> &func,
                                             item *parent, const std::set<pocket_type> &allowed_pockets = { pocket_type::CONTAINER } );

        void general_info( std::vector<iteminfo> &info, int pocket_number, bool disp_pocket_number ) const;
        void contents_info( std::vector<iteminfo> &info, int pocket_number, bool disp_pocket_number ) const;
        void favorite_info( std::vector<iteminfo> &info ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );


        bool is_default_state() const;

        bool same_contents( const item_pocket &rhs ) const;

        void restack();

        item *restack( item *it );
        bool has_item_stacks_with( const item &it ) const;






        bool better_pocket( const item_pocket &rhs, const item &it, bool nested = false ) const;

        bool operator==( const item_pocket &rhs ) const;

        favorite_settings settings;


        static void serialize_presets( JsonOut &json );
        static void deserialize_presets( const JsonArray &ja );
        static void load_presets();
        static void add_preset( const item_pocket::favorite_settings &preset );
        static void save_presets();
        static std::vector<item_pocket::favorite_settings>::iterator find_preset( const std::string &s );
        static bool has_preset( const std::string &s );
        static void delete_preset( std::vector<item_pocket::favorite_settings>::iterator iter );
        static std::vector<item_pocket::favorite_settings> pocket_presets;


        void set_no_rigid( const std::set<sub_bodypart_id> &is_no_rigid );


        bool name_as_description = false; // NOLINT(cata-serialize)



        static bool ok_default_containers( const item_pocket &pocket );


        static bool ok_all_containers( const item_pocket & );


        static bool ok_for_solids( const item_pocket &pocket );
    private:

        pocket_type _saved_type = pocket_type::LAST; // NOLINT(cata-serialize)
        bool _saved_sealed = false; // NOLINT(cata-serialize)
        const pocket_data *data = nullptr; // NOLINT(cata-serialize)

        std::list<item> contents;
        bool _sealed = false;

        std::optional<units::volume> bulk_fill_volume; // NOLINT(cata-serialize)
        std::optional<units::mass> bulk_fill_weight; // NOLINT(cata-serialize)

        float capacity_mult = 1.0f; // NOLINT(cata-serialize)

        std::set<sub_bodypart_id> no_rigid;

        ret_val<contain_code> _can_contain( const item &it, int &copies_remaining,
                                            bool ignore_contents ) const;
};









struct sealable_data {

    bool was_loaded = false;

    float spoil_multiplier = 1.0f;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};


struct pocket_noise {

    bool was_loaded = false;

    int volume = 0;
    int chance = 0;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

class pocket_data
{
    public:
        using FlagsSetType = cata::flat_set<flag_id>;

        bool was_loaded = false;

        pocket_data() = default;

        explicit pocket_data( pocket_type pk ) : type( pk ) {
            rigid = true;
        }


        static constexpr units::volume max_volume_for_container = 200000000_ml;
        static constexpr units::mass max_weight_for_container = 2000000_kilogram;

        pocket_type type = pocket_type::CONTAINER;




        units::volume raw_volume_capacity = max_volume_for_container;

        units::volume volume_capacity() const;

        std::optional<units::volume> max_item_volume = std::nullopt;

        units::volume min_item_volume = 0_ml;

        units::length min_item_length = 0_mm;

        units::mass max_contains_weight = max_weight_for_container;


        units::length max_item_length = 0_mm;

        bool holster = false;

        bool ablative = false;

        int extra_encumbrance = 0;

        float volume_encumber_modifier = 1;

        int ripoff = 0;

        pocket_noise activity_noise;

        float spoil_multiplier = 1.0f;

        float insulation = 1.0f;

        float weight_multiplier = 1.0f;

        float volume_multiplier = 1.0f;

        units::volume magazine_well = 0_ml;

        int moves = 100;

        bool fire_protection = false;

        bool watertight = false;

        bool airtight = false;

        bool open_container = false;

        bool inherits_flags = false;

        units::ememory ememory_max = 0_KB;

        bool transparent = false;


        translation description;



        translation name;


        translation pocket_name;


        cata::value_ptr<sealable_data> sealed_data;


        const FlagsSetType &get_flag_restrictions() const;


        void add_flag_restriction( const flag_id &flag );


        std::map<ammotype, int> ammo_restriction;


        cata::flat_set<itype_id> item_id_restriction;

        cata::flat_set<material_id> material_restriction;
        cata::flat_set<itype_id> allowed_speedloaders;

        itype_id default_magazine = itype_id::NULL_ID();

        std::string pocket_id;

        bool rigid = false;

        bool _no_unload = false; // NOLINT(cata-serialize)

        bool _no_reload = false; // NOLINT(cata-serialize)

        bool forbidden = false;

        bool operator==( const pocket_data &rhs ) const;

        std::string check_definition() const;

        void load( const JsonObject &jo );
        void deserialize( const JsonObject &data );
    private:
        FlagsSetType flag_restrictions;
};

class pocket_constraint
{
    public:
        units::mass weight_capacity = pocket_data::max_weight_for_container;
        units::length max_containable_length = 200000_meter;
        units::volume volume_capacity = pocket_data::max_volume_for_container;
        units::volume remaining_volume = pocket_data::max_volume_for_container;
        units::volume max_item_volume = 0_ml;


        bool in_rigid = false;




        bool is_dominated = false;

        pocket_constraint() = default;
        explicit pocket_constraint( const item_pocket *init_pocket );


        bool operator==( const pocket_constraint & ) const;
        bool operator!=( const pocket_constraint & ) const;


        void constrain_by( const item_pocket *outer );


        void constrain_by( const pocket_constraint &outer );
};

template<>
struct ret_val<item_pocket::contain_code>::default_success
    : public std::integral_constant<item_pocket::contain_code,
      item_pocket::contain_code::SUCCESS> {};

#endif
