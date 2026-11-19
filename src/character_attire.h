#pragma once
#ifndef CATA_SRC_CHARACTER_ATTIRE_H
#define CATA_SRC_CHARACTER_ATTIRE_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "color.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "ret_val.h"
#include "subbodypart.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"

class Character;
class JsonObject;
class JsonOut;
class advanced_inv_area;
class advanced_inv_listitem;
class advanced_inventory_pane;
class avatar;
class npc;
class player_morale;
struct bodygraph_info;
struct damage_unit;

struct item_penalties {
    std::vector<bodypart_id> body_parts_with_stacking_penalty;
    std::vector<bodypart_id> body_parts_with_out_of_order_penalty;
    std::set<std::string> bad_items_within;

    int badness() const {
        return !body_parts_with_stacking_penalty.empty() +
               !body_parts_with_out_of_order_penalty.empty();
    }

    nc_color color_for_stacking_badness() const;
};

struct layering_item_info {
    item_penalties penalties;
    int encumber;
    std::string name;


    bool operator ==( const layering_item_info &o ) const {





        return this->penalties.badness() == o.penalties.badness() &&
               this->encumber == o.encumber &&
               this->name == o.name;
    }
};

struct dispose_option {
    std::string prompt;
    bool enabled;
    char invlet;
    int moves;
    std::function<bool()> action;
};

class outfit
{
        friend class Character;
    private:
        std::list<item> worn;
    public:
        outfit() = default;
        explicit outfit( const std::list<item> &items ) : worn( items ) {}
        bool is_worn( const item &clothing ) const;
        bool is_worn( const itype_id &clothing ) const;
        bool is_worn_module( const item &thing ) const;
        bool is_wearing_on_bp( const itype_id &clothing, const bodypart_id &bp ) const;
        bool covered_with_flag( const flag_id &f, const body_part_set &parts ) const;
        bool wearing_something_on( const bodypart_id &bp ) const;
        bool wearing_fitting_on( const bodypart_id &bp ) const;
        bool worn_with_flag( const flag_id &f, const bodypart_id &bp ) const;
        bool worn_with_flag( const flag_id &f ) const;
        bool is_worn_item_visible( std::list<item>::const_iterator worn_item,
                                   const body_part_set &worn_item_body_parts ) const;

        bool hands_conductive() const;
        bool can_pickVolume( const item &it, bool ignore_pkt_settings = true,
                             bool ignore_non_container_pocket = false ) const;
        side is_wearing_shoes( const bodypart_id &bp ) const;
        bool is_barefoot() const;
        item item_worn_with_flag( const flag_id &f, const bodypart_id &bp ) const;
        item item_worn_with_flag( const flag_id &f ) const;
        item *item_worn_with_id( const itype_id &i );
        std::optional<const item *> item_worn_with_inv_let( char invlet ) const;

        item *best_shield();

        item *current_unarmed_weapon( const sub_bodypart_str_id &contact_area );
        item_location first_item_covering_bp( Character &guy, bodypart_id bp );
        void inv_dump( std::vector<item *> &ret );
        void inv_dump( std::vector<const item *> &ret ) const;




        void item_encumb( std::map<bodypart_id, encumbrance_data> &vals, const item &new_item,
                          const Character &guy ) const;
        std::list<item_location> get_visible_worn_items( const Character &guy );
        int swim_modifier( int swim_skill ) const;
        bool natural_attack_restricted_on( const bodypart_id &bp ) const;
        bool natural_attack_restricted_on( const sub_bodypart_id &bp ) const;
        units::mass weight_carried_with_tweaks( const std::map<const item *, int> &without ) const;
        units::mass weight() const;
        units::volume remaining_volume_recursive( const std::function<bool( const item_pocket & )>
                &include_pocket,
                const std::function<bool( const item_pocket & )> &check_pocket_tree ) const;
        units::mass free_weight_capacity() const;
        units::volume max_single_item_volume() const;
        units::length max_single_item_length() const;

        units::volume volume_capacity( const std::function<bool( const item_pocket & )> &include_pocket =
                                           item_pocket::ok_default_containers ) const;
        int pocket_warmth() const;
        int hood_warmth() const;
        int collar_warmth() const;

        std::map<bodypart_id, int> warmth( const Character &guy ) const;

        std::map<bodypart_id, int> wind_resistance( const Character &guy ) const;
        int get_env_resist( bodypart_id bp ) const;
        int sum_filthy_cover( bool ranged, bool melee, bodypart_id bp ) const;
        ret_val<void> power_armor_conflicts( const item &clothing ) const;
        bool is_wearing_power_armor( bool *has_helmet = nullptr ) const;
        bool is_wearing_active_power_armor() const;
        bool is_wearing_active_optcloak() const;
        ret_val<void> only_one_conflicts( const item &clothing ) const;
        bool one_per_layer_change_side( item &it, const Character &guy ) const;
        void one_per_layer_sidedness( item &clothing ) const;
        ret_val<void> check_rigid_conflicts( const item &clothing, side s ) const;
        ret_val<void> check_rigid_conflicts( const item &clothing ) const;
        bool check_rigid_change_side( item &it, const Character &guy ) const;
        void check_rigid_sidedness( item &clothing ) const;
        int amount_worn( const itype_id &clothing ) const;
        int worn_guns() const;
        int clatter_sound() const;
        bool adjust_worn( npc &guy );
        float clothing_wetness_mult( const bodypart_id &bp, bool permeability_check = false ) const;
        void damage_mitigate( const bodypart_id &bp, damage_unit &dam ) const;
        float damage_resist( const damage_type_id &dt, const bodypart_id &bp, bool to_self = false ) const;

        int coverage_with_flags_exclude( const bodypart_id &bp, const std::vector<flag_id> &flags ) const;
        int get_coverage( bodypart_id bp,
                          item::cover_type cover_type = item::cover_type::COVER_DEFAULT ) const;
        void bodypart_exposure( std::map<bodypart_id, float> &bp_exposure,
                                const std::vector<bodypart_id> &all_body_parts ) const;
        void prepare_bodymap_info( bodygraph_info &info, const bodypart_id &bp,
                                   const std::set<sub_bodypart_id> &sub_parts, const Character &person ) const;

        void get_overlay_ids( std::vector<std::pair<std::string, std::string>> &overlay_ids ) const;
        body_part_set exclusive_flag_coverage( body_part_set bps, const flag_id &flag ) const;
        bool check_item_encumbrance_flag( bool update_required );

        void add_dependent_item( std::list<item *> &dependent, const item &it );
        std::list<item> remove_worn_items_with( const std::function<bool( const item & )> &filter,
                                                Character &guy );




        bool takeoff( item_location loc, std::list<item> *res, Character &guy );
        std::list<item> use_amount(
            const itype_id &it, int quantity, std::list<item> &used,
            const std::function<bool( const item & )> &filter, Character &wearer );
        std::list<item>::iterator position_to_wear_new_item( const item &new_item );
        std::optional<std::list<item>::iterator> wear_item( Character &guy, const item &to_wear,
                bool interactive, bool do_calc_encumbrance, bool do_sort_items = true, bool quiet = false );

        void recalc_ablative_blocking( const Character *guy );

        std::unordered_set<bodypart_id> where_discomfort( const Character &guy ) const;

        void insert_item_at_index( const item &clothing, int index );
        void check_and_recover_morale( player_morale &test_morale ) const;
        void absorb_damage( Character &guy, damage_unit &elem, bodypart_id bp,
                            std::list<item> &worn_remains, bool &armor_destroyed );

        void sort_armor( Character &guy );




        std::vector<item_pocket *> grab_drop_pockets();
        std::vector<item_pocket *> grab_drop_pockets( const bodypart_id &bp );
        std::vector<layering_item_info> items_cover_bp( const Character &c, const bodypart_id &bp );
        item_penalties get_item_penalties( std::list<item>::const_iterator worn_item_it,
                                           const Character &c, const bodypart_id &_bp );
        std::vector<advanced_inv_listitem> get_AIM_inventory( size_t &item_index, avatar &you,
                const advanced_inventory_pane &pane, advanced_inv_area &square );
        void add_AIM_items_from_area( avatar &you, advanced_inv_area &square,
                                      advanced_inventory_pane &pane );
        void fire_options( Character &guy, std::vector<std::string> &options,
                           std::vector<std::function<void()>> &actions );

        void best_pocket( Character &guy, const item &it, const item *avoid,
                          std::pair<item_location, item_pocket *> &current_best,
                          bool ignore_settings = false );
        void overflow( Character &guy );
        void holster_opts( std::vector<dispose_option> &opts, item_location obj, Character &guy );
        void get_eligible_containers_for_crafting( std::vector<const item *> &conts ) const;

        void on_takeoff( Character &guy );

        void on_item_wear( Character &guy );

        void pickup_stash( const item &newit, int &remaining_charges, bool ignore_pkt_settings = false );
        void add_stash( Character &guy, const item &newit, int &remaining_charges,
                        bool ignore_pkt_settings = false );

        void set_fitted();
        std::vector<item> available_pockets() const;
        void write_text_memorial( std::ostream &file, const std::string &indent, const char *eol ) const;
        std::string get_armor_display( bodypart_id bp ) const;
        void activate_combat_items( npc &guy );
        void deactivate_combat_items( npc &guy );

        bool empty() const;
        item &front();
        size_t size() const;

        std::vector<item_location> top_items_loc( Character &guy );
        std::vector<item_location> all_items_loc( Character &guy );


        std::optional<int> get_item_position( const item &it ) const;

        const item &i_at( int position ) const;

        VisitResponse visit_items( const Character &wearer,
                                   const std::function<VisitResponse( const item_location & )> &func ) const;
        std::list<item> remove_items_with( Character &guy,
                                           const std::function<bool( const item & )> &filter, int &count );

        void organize_items_menu( Character &guy );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
};

units::mass get_selected_stack_weight( const item *i, const std::map<const item *, int> &without );
void destroyed_armor_msg( Character &who, const std::string &pre_damage_name );

#endif
