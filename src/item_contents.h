#pragma once
#ifndef CATA_SRC_ITEM_CONTENTS_H
#define CATA_SRC_ITEM_CONTENTS_H

#include <cstddef>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "enums.h"
#include "item_pocket.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"

class Character;
class JsonObject;
class JsonOut;
class item;
class item_location;
class iteminfo_query;
class map;
struct iteminfo;


enum class content_newness {
    NEW,
    MIGHT_BE_HIDDEN,
    SEEN
};

class item_contents
{
    public:
        item_contents() = default;

        explicit item_contents( const std::vector<pocket_data> &pockets );














        std::pair<item_location, item_pocket *> best_pocket( const item &it, item_location &this_loc,
                const item *avoid = nullptr, bool allow_sealed = false, bool ignore_settings = false,
                bool nested = false, bool ignore_rigidity = false, bool allow_nested = true );

        units::length max_containable_length( bool unrestricted_pockets_only = false ) const;
        units::length min_containable_length() const;
        units::volume max_containable_volume( bool unrestricted_pockets_only = false ) const;

        std::set<flag_id> magazine_flag_restrictions() const;






        ret_val<void> is_compatible( const item &it ) const;











        ret_val<void> can_contain( const item &it, bool ignore_pkt_settings = true,
                                   bool ignore_non_container_pocket = false,
                                   units::volume remaining_parent_volume = 10000000_ml ) const;
        ret_val<void> can_contain( const item &it, int &copies_remaining, bool ignore_pkt_settings = true,
                                   bool ignore_non_container_pocket = false,
                                   units::volume remaining_parent_volume = 10000000_ml ) const;
        ret_val<void> can_contain_rigid( const item &it, bool ignore_pkt_settings = true,
                                         bool ignore_non_container_pocket = false ) const;
        ret_val<void> can_contain_rigid( const item &it, int &copies_remaining,
                                         bool ignore_pkt_settings = true,
                                         bool ignore_non_container_pocket = false ) const;
        bool can_contain_liquid( bool held_or_ground ) const;

        bool contains_no_solids() const;






        bool can_reload_with( const item &ammo, bool now ) const;


        bool empty() const;

        bool empty_with_no_mods() const;

        bool empty_container() const;

        bool full( bool allow_bucket ) const;

        bool is_magazine_full() const;

        bool bigger_on_the_inside( const units::volume &container_volume ) const;

        size_t size() const;


        std::list<item *> all_items_top( const std::function<bool( item_pocket & )> &filter );

        std::list<const item *> all_items_top( const std::function<bool( const item_pocket & )> &filter )
        const;


        std::list<item *> all_items_top( pocket_type pk_type, bool unloading = false );

        std::list<const item *> all_items_top( pocket_type pk_type ) const;


        std::list<item *> all_items_top();

        std::list<const item *> all_items_top() const;

        std::list<item *> all_items_container_top();

        std::list<const item *> all_items_container_top() const;


        std::list<item *> all_known_contents();
        std::list<const item *> all_known_contents() const;


        std::list<item *> all_holstered_items();
        std::list<const item *> all_holstered_items() const;


        std::list<item *> all_ablative_armor();
        std::list<const item *> all_ablative_armor() const;


        content_newness get_content_newness( const std::set<itype_id> &read_items ) const;


        std::vector<item *> gunmods();

        std::vector<const item *> gunmods() const;

        bool allows_speedloader( const itype_id &speedloader_id ) const;

        std::vector<const item *> mods() const;

        std::vector<const item *> softwares() const;

        std::vector<item *> ebooks();
        std::vector<const item *> ebooks() const;

        std::vector<item *> efiles();
        std::vector<const item *> efiles() const;



        units::ememory occupied_ememory() const;

        std::vector<item *> cables();
        std::vector<const item *> cables() const;

        void update_modified_pockets( std::vector<const pocket_data *> mag_or_mag_wells,
                                      std::vector<const pocket_data *> container_pockets );


        std::set<itype_id> magazine_compatible() const;




        itype_id magazine_default() const;




        std::vector<itype_id> magazines_default() const;





        item &legacy_front();
        const item &legacy_front() const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;
        units::length item_length_modifier() const;


        units::mass total_container_weight_capacity( bool unrestricted_pockets_only = false ) const;


        units::volume volume_capacity( const std::function<bool( const item_pocket & )> &include_pocket =
                                           item_pocket::ok_default_containers ) const;







        units::volume volume_capacity_recursive( const std::function<bool( const item_pocket & )>
                &include_pocket,
                const std::function<bool( const item_pocket & )> &check_pocket_tree,
                units::volume &out_volume_expansion ) const;

        units::volume contents_volume( const std::function<bool( const item_pocket & )> &include_pocket =
                                           item_pocket::ok_default_containers ) const;


        units::volume remaining_volume( const std::function<bool( const item_pocket & )> &include_pocket =
                                            item_pocket::ok_default_containers ) const;







        units::volume remaining_volume_recursive( const std::function<bool( const item_pocket & )>
                &include_pocket,
                const std::function<bool( const item_pocket & )> &check_pocket_tree,
                units::volume &out_volume_expansion ) const;





        units::volume biggest_pocket_capacity() const;
        units::mass remaining_container_capacity_weight( bool unrestricted_pockets_only = false ) const;
        units::mass total_contained_weight( bool unrestricted_pockets_only = false ) const;


        int get_used_holsters() const;
        int get_total_holsters() const;
        units::volume get_total_holster_volume() const;
        units::volume get_used_holster_volume() const;
        units::mass get_total_holster_weight() const;
        units::mass get_used_holster_weight() const;


        std::vector<const item_pocket *> get_container_pockets() const;
        std::vector<item_pocket *> get_container_pockets();
        std::vector<const item_pocket *> get_standard_pockets() const;
        std::vector<item_pocket *> get_standard_pockets();
        std::vector<const item_pocket *> get_ablative_pockets() const;
        std::vector<item_pocket *> get_ablative_pockets();
        std::vector<const item_pocket *> get_container_and_mod_pockets() const;
        std::vector<item_pocket *> get_container_and_mod_pockets();
        std::vector<const item_pocket *>
        get_pockets( const std::function<bool( const item_pocket & )> &filter ) const;
        std::vector<item_pocket *>
        get_pockets( const std::function<bool( const item_pocket & )> &filter );




        void add_pocket( const item &pocket );






        item remove_pocket( int index );


        const item_pocket *get_added_pocket( int index ) const;

        std::vector<const item *> get_added_pockets() const;
        std::vector<item *> get_added_pockets_mutable();

        bool has_additional_pockets() const;

        int get_additional_pocket_encumbrance( float mod ) const;
        int get_additional_space_used() const;
        units::mass get_additional_weight() const;
        units::volume get_additional_volume() const;


        std::vector<const item_pocket *> get_all_reloadable_pockets() const;


        int remaining_capacity_for_liquid( const item &liquid ) const;






        float relative_encumbrance() const;

        bool all_pockets_rigid() const;

        bool container_type_pockets_empty() const;


        int best_quality( const quality_id &id ) const;






        int obtain_cost( const item &it ) const;

        int insert_cost( const item &it ) const;














        ret_val<item *> insert_item( const item &it, pocket_type pk_type,
                                     bool ignore_contents = false, bool unseal_pockets = false );
        void force_insert_item( const item &it, pocket_type pk_type );
        bool can_unload_liquid() const;






        size_t num_item_stacks() const;




        void favorite_settings_menu( item_location il );

        item_pocket *contained_where( const item &contained );
        void on_pickup( Character &guy, item *avoid = nullptr );
        bool spill_contents( const tripoint_bub_ms &pos );
        bool spill_contents( map *here, const tripoint_bub_ms &pos );

        void overflow( map &here, const tripoint_bub_ms &pos, const item_location &loc );
        void clear_items();

        void begin_bulk_fill();
        void end_bulk_fill();

        void clear_magazines();
        void clear_pockets_if( const std::function<bool( item_pocket const & )> &filter );
        void update_open_pockets();


        bool seal_all_pockets();
        bool all_pockets_sealed() const;
        bool any_pockets_sealed() const;

        void heat_up();

        int ammo_consume( int qty, const tripoint_bub_ms &pos, float fuel_efficiency = -1.0 );
        int ammo_consume( int qty, map *here, const tripoint_bub_ms &pos, float fuel_efficiency = -1.0 );



        int ammo_consume_in_pocket( const std::string &id, int qty, map *here,
                                    const tripoint_bub_ms &pos );
        item *magazine_current();
        const item *magazine_current() const;
        std::vector<item *> magazines_current();
        std::vector<const item *> magazines_current() const;
        std::set<ammotype> ammo_types() const;
        int ammo_capacity( const ammotype &ammo ) const;
        item &first_ammo();
        const item &first_ammo() const;





        void handle_liquid_or_spill( Character &guy, const item *avoid = nullptr );

        bool will_spill() const;
        bool will_spill_if_unsealed() const;
        bool spill_open_pockets( Character &guy, const item *avoid = nullptr );
        void casings_handle( const std::function<bool( item & )> &func );


        item &only_item();
        item &first_item();
        const item &only_item() const;
        const item &first_item() const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        const item *get_item_with( const std::function<bool( const item & )> &filter ) const;
        void remove_items_if( const std::function<bool( item & )> &filter );


        bool has_pocket_type( pocket_type pk_type ) const;
        bool has_unrestricted_pockets() const;
        bool has_any_with( const std::function<bool( const item & )> &filter,
                           pocket_type pk_type ) const;





        void process( map &here, Character *carrier, const tripoint_bub_ms &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f,
                      bool watertight_container = false );

        void leak( map &here, Character *carrier, const tripoint_bub_ms &pos,
                   item_pocket *pocke = nullptr );

        bool item_has_uses_recursive() const;
        bool stacks_with( const item_contents &rhs, int depth = 0, int maxdepth = 2 ) const;
        bool same_contents( const item_contents &rhs ) const;

        bool is_funnel_container( units::volume &bigger_than ) const;

        bool is_restricted_container() const;
        bool is_single_restricted_container() const;




        VisitResponse visit_contents( const std::function<VisitResponse( const item_location & )> &func,
                                      const item_location &parent, const std::set<pocket_type> &allowed_pockets = { pocket_type::CONTAINER } );
        VisitResponse visit_contents_legacy( const std::function<VisitResponse( item *, item * )> &func,
                                             item *parent,
                                             const std::set<pocket_type> &allowed_pockets = { pocket_type::CONTAINER } );
        void remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );

        void info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const;


        void read_mods( const item_contents &read_input );
        void combine( const item_contents &read_input, bool convert = false, bool into_bottom = false,
                      bool restack_charges = true, bool ignore_contents = false );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
    private:




        ret_val<item_pocket *> find_pocket_for( const item &it,
                                                pocket_type pk_type = pocket_type::CONTAINER );

        ret_val<const item_pocket *> find_pocket_for( const item &it,
                pocket_type pk_type = pocket_type::CONTAINER ) const;

        std::list<item_pocket> contents;


        std::vector<item> additional_pockets;

        units::volume additional_pockets_volume = 0_ml; // NOLINT(cata-serialize)

        units::volume additional_pockets_effective_volume = 0_ml; // NOLINT(cata-serialize)


        int additional_pockets_space_used = 0; // NOLINT(cata-serialize)

        struct item_contents_helper;

        friend struct item_contents_helper;
};

void pocket_management_menu( const std::string &title,
                             const std::vector<item_location> &to_organize );

#endif
