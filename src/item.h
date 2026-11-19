#pragma once
#ifndef CATA_SRC_ITEM_H
#define CATA_SRC_ITEM_H

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_lazy.h"
#include "cata_utility.h"
#include "character_id.h"
#include "coordinates.h"
#include "craft_command.h"
#include "craft_reservation.h"
#include "crafting_enums.h"
#include "enums.h"
#include "flat_set.h"
#include "global_vars.h"
#include "gun_mode.h"
#include "io_tags.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_tname.h"
#include "item_uid.h"
#include "material.h"
#include "math_parser_diag_value.h"
#include "pocket_type.h"
#include "point.h"
#include "requirements.h"
#include "rng.h"
#include "safe_reference.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
struct desired_wakeup;
enum class item_wakeup_kind : uint8_t;
class book_proficiency_bonuses;
class enchant_cache;
class enchantment;
class gun_type_type;
class gunmod_location;
class item;
class iteminfo_query;
class map;
class monster;
class nc_color;
class optional_vpart_position;
class recipe;
class relic;
class vehicle;
struct armor_portion_data;
struct islot_comestible;
struct itype;
struct itype_variant_data;
struct pocket_consumption_entry;
struct mtype;
struct part_material;
template<typename T>
class ret_val;
template <typename T> struct enum_traits;

namespace enchant_vals
{
enum class mod : int;
}

using bodytype_id = std::string;
class body_part_set;
class item_category;
enum class side : int;
enum clothing_mod_type : int;
struct damage_instance;
struct damage_unit;
struct fire_data;
struct islot_armor;
struct use_function;

struct light_emission {
    unsigned short luminance;
    short width;
    short direction;
};
extern light_emission nolight;













struct iteminfo {
    public:

        std::string sType;


        std::string sName;


        std::string sFmt;


        std::string sValue;


        double dValue;


        double dUnitAdjustedVal;


        bool is_int;


        bool bNewLine;


        bool bLowerIsBetter;


        bool bDrawName;


        bool bShowPlus;


        bool three_decimal;


        bool bIsArt;


        bool isTable;

        enum flags {
            no_flags = 0,
            is_decimal = 1 << 0,
            is_three_decimal = 1 << 1,
            no_newline = 1 << 2,
            lower_is_better = 1 << 3,
            no_name = 1 << 4,
            show_plus = 1 << 5,
            is_art = 1 << 6,
            is_table = 1 << 7,
        };








        iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt = "",
                  flags Flags = no_flags, double Value = -999, double UnitVal = 0 );
        iteminfo( const std::string &Type, const std::string &Name, flags Flags );
        iteminfo( const std::string &Type, const std::string &Name, double Value, double UnitVal = 0 );
};

template<>
struct enum_traits<iteminfo::flags> {
    static constexpr bool is_flag_enum = true;
};

iteminfo vol_to_info( const std::string &type, const std::string &left,
                      const units::volume &vol, int decimal_places = 2, bool lower_is_better = true );
iteminfo weight_to_info( const std::string &type, const std::string &left,
                         const units::mass &weight, int decimal_places = 2, bool lower_is_better = true );

inline bool is_crafting_component( const item &component );

struct stacking_info {
    tname::segment_bitset bits;
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() const {
        return bits.all();
    }
};

class item
{
    public:
        using FlagsSetType = cata::flat_set<flag_id>;

        item();

        item( item && ) noexcept;
        item( const item & );
        item &operator=( item && ) noexcept;
        item &operator=( const item & );

        explicit item( const itype_id &id, time_point turn = calendar::turn, int qty = -1 );
        explicit item( const itype *type, time_point turn = calendar::turn, int qty = -1 );


        struct default_charges_tag {};
        item( const itype_id &id, time_point turn, default_charges_tag );
        item( const itype *type, time_point turn, default_charges_tag );


        struct solitary_tag {};
        item( const itype_id &id, time_point turn, solitary_tag );
        item( const itype *type, time_point turn, solitary_tag );


        item( const recipe *rec, int qty, item_components items, std::vector<item_comp> selections,
              bool should_add_faults = false );


        item( const recipe *rec, int qty, item &component );

        ~item();



        safe_reference<item> get_safe_reference();



        const item_uid &uid() const {
            return uid_;
        }







        item &convert( const itype_id &new_type, Character *carrier = nullptr );








        item &deactivate( Character *ch = nullptr, bool alert = true );


        item &activate();







        bool activate_thrown( const tripoint_bub_ms &pos );










        units::energy mod_energy( const units::energy &qty );








        item &ammo_set( const itype_id &ammo, int qty = -1 );






        item &ammo_unset();




        void set_damage( int qty );




        void force_set_damage( int qty );




        void set_degradation( int qty );






        item split( int qty );
















        static item make_corpse( const mtype_id &mt = string_id<mtype>::NULL_ID(),
                                 time_point turn = calendar::turn, const std::string &name = "", int upgrade_time = -1 );






        const mtype *get_mtype() const;





        void set_mtype( const mtype *m );





        bool is_corpse() const;



        bool can_revive() const;





        bool ready_to_revive( map &here, const tripoint_bub_ms &pos ) const;

        bool is_money() const;
    private:
        bool is_money( const std::set<ammotype> &ammo ) const;
    public:

        bool is_cash_card() const;

        bool is_estorage() const;

        bool is_estorage_usable( const Character &who ) const;
        bool is_estorable() const;
        bool is_estorable_exclusive() const;
        bool is_browsed() const;
        void set_browsed( bool browsed );

        bool is_ecopiable() const;

        bool efiles_all_browsed() const;

        units::ememory ememory_size() const;

        units::ememory total_ememory() const;

        units::ememory occupied_ememory() const;

        units::ememory remaining_ememory() const;

        item *get_recipe_catalog();
        const item *get_recipe_catalog() const;

        item *get_photo_gallery();
        const item *get_photo_gallery() const;

        int total_photos() const;

        bool is_software() const;





        bool made_of_any_food_components( bool deep_search = false ) const;





        bool is_maybe_melee_weapon() const;




        bool has_edged_damage() const;




        std::string dirt_symbol() const;




        std::string overheat_symbol() const;




        std::string degradation_symbol() const;




        nc_color color() const;





        nc_color color_in_inventory( const Character *ch = nullptr ) const;



        nc_color get_fault_color( nc_color base_color ) const;






        std::string tname( unsigned int quantity = 1,
                           tname::segment_bitset const &segments = tname::default_tname,
                           bool color_faults = false ) const;
        std::string tname( unsigned int quantity, bool with_prefix, bool color_faults = false ) const;
        static std::string tname( const itype_id &id, unsigned int quantity = 1,
                                  tname::segment_bitset const &segments = tname::default_tname );
        std::string display_money( unsigned int quantity, unsigned int total,
                                   const std::optional<unsigned int> &selected = std::nullopt ) const;




        std::string display_name( unsigned int quantity = 1, bool color_faults = false ) const;

        std::vector<iteminfo> get_info( bool showtext ) const;
        std::vector<iteminfo> get_info( bool showtext, int batch ) const;
        std::vector<iteminfo> get_info( const iteminfo_query *parts, int batch ) const;









        std::string info( bool showtext = false ) const;











        std::string info( bool showtext, std::vector<iteminfo> &iteminfo ) const;












        std::string info( bool showtext, std::vector<iteminfo> &iteminfo, int batch ) const;












        std::string info( std::vector<iteminfo> &info, const iteminfo_query *parts = nullptr,
                          int batch = 1 ) const;

        void basic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;
        void debug_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;
        void med_info( const item *med_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                       int batch, bool debug ) const;
        void food_info( const item *food_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                        int batch, bool debug ) const;
        void rot_info( const item *food_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                       int batch, bool debug ) const;
        void magazine_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                            bool debug ) const;
        void ammo_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                        bool debug ) const;
        std::string print_compatible_mags_or_flags() const;
        void gun_info( const item *mod, std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool debug ) const;
        void gunmod_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool debug ) const;
        void armor_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                    bool debug, const sub_bodypart_id &sbp = sub_bodypart_id() ) const;
        void armor_material_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                  bool debug, const sub_bodypart_id &sbp = sub_bodypart_id() ) const;
        void armor_attribute_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                   bool debug, const sub_bodypart_id &sbp = sub_bodypart_id() ) const;
        void pet_armor_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const;
        void armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;
        void animal_armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                bool debug ) const;
        void armor_fit_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void book_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                        bool debug ) const;
        void battery_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                           bool debug ) const;
        void tool_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                        bool debug ) const;
        void actions_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                           bool debug ) const;
        void component_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void enchantment_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                               bool debug ) const;
        void repair_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool debug ) const;
        void disassembly_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                               bool debug ) const;
        void qualities_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void bionic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool debug ) const;
        void melee_combat_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                                bool debug ) const;
        void contents_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                            bool debug ) const;
        void properties_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                              bool debug ) const;
        void ascii_art_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                             bool debug ) const;
        void final_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                         bool debug ) const;




        static std::string layer_to_string( layer_level data );






        float simulate_burn( fire_data &frd ) const;

        bool burn( fire_data &frd );


        const item_category &get_category_shallow() const;


        const item_category &get_category_of_contents( int depth = 0, int maxdepth = 2 ) const;

        class reload_option
        {
            public:
                reload_option() = default;

                reload_option( const reload_option & );
                reload_option &operator=( const reload_option & );

                reload_option( const Character *who, const item_location &target, const item_location &ammo,
                               int pocket_index );


                static constexpr int POCKET_FALLBACK = -1;

                const Character *who = nullptr;
                item_location target;
                item_location ammo;
                bool is_reload_one = false;

                int pocket_index = POCKET_FALLBACK;

                int qty() const {
                    return qty_;
                }
                void qty( int val );

                int moves() const;

                explicit operator bool() const {
                    return who && target && ammo && qty_ > 0;
                }
            private:
                int qty_ = 0;
                int max_qty = INT_MAX;
        };










        bool reload( Character &u, item_location ammo, int qty, int pocket_index = -1 );

        bool allows_speedloader( const itype_id &speedloader_id ) const;

        template<typename Archive>
        void io( Archive & );
        using archive_type_tag = io::object_archive_tag;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        const std::string &symbol() const;





        int price( bool practical ) const;






        int price_no_contents( bool practical, std::optional<int> price_override = std::nullopt ) const;








        bool display_stacked_with( const item &rhs, bool check_components = false ) const;






        stacking_info stacks_with( const item &rhs, bool check_components = false,
                                   bool combine_liquid = false, bool check_cat = false,
                                   int depth = 0, int maxdepth = 2, bool precise = false ) const;





        bool same_contents( const item &rhs ) const;








        bool same_for_rle( const item &rhs ) const;

        bool combine( const item &rhs );
        bool can_combine( const item &rhs ) const;






        bool merge_charges( const item &rhs );






        units::mass weight( bool include_contents = true, bool integral = false ) const;









        units::volume volume( bool integral = false, bool ignore_contents = false,
                              int charges_in_vol = -1 ) const;

        units::length length() const;
        units::length barrel_length() const;
        units::length sawn_off_reduction() const;






        units::volume base_volume() const;


        units::volume corpse_volume( const mtype *corpse ) const;





        units::volume collapsed_volume_delta() const;


        int lift_strength() const;













        int attack_time( const Character &you ) const;


        int damage_melee( const damage_type_id &dt ) const;


        damage_instance base_damage_melee() const;

        damage_instance base_damage_thrown() const;





        double effective_dps( const Character &guy, Creature &mon ) const;







        std::map<std::string, double> dps( bool for_display, bool for_calc, const Character &guy ) const;
        std::map<std::string, double> dps( bool for_display, bool for_calc ) const;

        double average_dps( const Character &guy ) const;




        bool is_two_handed( const Character &guy ) const;


        bool is_melee( const damage_type_id &dt ) const;






        bool is_melee() const;





        skill_id melee_skill() const;








        std::pair<int, int> reach_range( const Character &guy ) const;







        std::pair<int, int> current_reach_range( const Character &guy ) const;




        void set_countdown( int num_turns );










        bool use_charges( const item_location &self, const itype_id &what, int &qty, std::list<item> &used,
                          const tripoint_bub_ms &pos, const std::function<bool( const item & )> &filter = return_true<item>,
                          bool in_tools = false );







        bool on_drop( const tripoint_bub_ms &pos );








        bool on_drop( const tripoint_bub_ms &pos, map &map );










        bool use_amount( item_location self, const itype_id &it, int &quantity, std::list<item> &used,
                         const std::function<bool( const item & )> &filter = return_true<item> );


        bool allow_crafting_component() const;


        bool seal();

        bool all_pockets_sealed() const;
        bool any_pockets_sealed() const;




        bool is_container() const;

        bool is_container_with_restriction() const;

        bool is_single_container_with_restriction() const;

        bool has_pocket_type( pocket_type pk_type ) const;
        bool has_any_with( const std::function<bool( const item & )> &filter,
                           pocket_type pk_type ) const;


        bool all_pockets_rigid() const;

        bool container_type_pockets_empty() const;

        std::vector<item_pocket *> get_pockets( const std::function<bool( const item_pocket &pocket )> &
                                                include_pocket );
        std::vector<const item_pocket *> get_pockets( const
                std::function<bool( const item_pocket &pocket )> &
                include_pocket ) const;

        std::vector<const item_pocket *> get_container_pockets() const;
        std::vector<item_pocket *> get_container_pockets();
        std::vector<const item_pocket *> get_standard_pockets() const;
        std::vector<item_pocket *> get_standard_pockets();
        std::vector<item_pocket *> get_ablative_pockets();
        std::vector<const item_pocket *> get_ablative_pockets() const;
        std::vector<const item_pocket *> get_container_and_mod_pockets() const;
        std::vector<item_pocket *> get_container_and_mod_pockets();





        void update_modified_pockets();


        void validate_mod_pocket_refs() const;





        item_pocket *contained_where( const item &contained );
        const item_pocket *contained_where( const item &contained ) const;

        bool is_watertight_container() const;

        bool is_container_empty() const;




        bool is_container_full( bool allow_bucket = false ) const;




        bool is_magazine_full() const;












        int fill_with( const item &contained, int amount = INFINITE_CHARGES,
                       bool unseal_pockets = false,
                       bool allow_sealed = false,
                       bool ignore_settings = false,
                       bool into_bottom = false,
                       bool allow_nested = true,
                       Character *carrier = nullptr );










        int get_remaining_capacity_for_liquid( const item &liquid, bool allow_bucket = false,
                                               rem_cap_return *err = nullptr ) const;
        int get_remaining_capacity_for_liquid( const item &liquid, const Character &p,
                                               rem_cap_return *err = nullptr ) const;




        units::volume get_volume_capacity( const std::function<bool( const item_pocket & )> &include_pocket
                                           =
                                               item_pocket::ok_default_containers ) const;
        units::volume get_volume_capacity_recursive( const std::function<bool( const item_pocket & )> &
                include_pocket,
                const std::function<bool( const item_pocket & )> &check_pocket_tree,
                units::volume &out_volume_expansion ) const;
        units::mass get_total_weight_capacity( bool unrestricted_pockets_only = false ) const;

        units::volume get_remaining_volume( const std::function<bool( const item_pocket & )> &include_pocket
                                            =
                                                item_pocket::ok_default_containers ) const;
        units::volume get_remaining_volume_recursive( const std::function<bool( const item_pocket & )> &
                include_pocket,
                const std::function<bool( const item_pocket & )> &check_pocket_tree,
                units::volume &out_volume_expansion ) const;
        units::mass get_remaining_weight_capacity( bool unrestricted_pockets_only = false ) const;

        units::volume get_contents_volume( const std::function<bool( const item_pocket & )> &include_pocket
                                           =
                                               item_pocket::ok_default_containers ) const;
        units::mass get_total_contained_weight( bool unrestricted_pockets_only = false ) const;






        units::volume get_biggest_pocket_capacity() const;

        bool has_unrestricted_pockets() const;





        int get_pocket_size() const;




        bool can_attach_as_pocket() const;





        int obtain_cost( const item &it ) const;

        int insert_cost( const item &it ) const;





        ret_val<void> put_in( const item &payload, pocket_type pk_type,
                              bool unseal_pockets = false, Character *carrier = nullptr,
                              bool quiet = false );
        void force_insert_item( const item &it, pocket_type pk_type );







        item in_its_container( int qty = 0 ) const;
        item in_container( const itype_id &container_type, int qty = 0, bool sealed = true,
                           const std::string &container_variant = "" ) const;
        void add_automatic_whitelist();
        void clear_automatic_whitelist();





        bool item_has_uses_recursive( bool contents_only = false ) const;





        bool is_funnel_container( units::volume &bigger_than ) const;
        void add_rain_to_container( int charges = 1 );






        bool has_quality_nonrecursive( const quality_id &qual, int level = 1 ) const;







        int get_quality_nonrecursive( const quality_id &id, bool strict_boiling = true ) const;







        int get_quality( const quality_id &id, bool strict_boiling = true ) const;






        float get_quality_speed( const quality_id &id, int level,
                                 const Character *crafter = nullptr ) const;





        bool count_by_charges() const;






        void compress_charges_or_liquid( int &compcount );




        int count() const;
        bool craft_has_charges() const;







        void mod_charges( int mod );





        void preserve_location( const tripoint_abs_ms &location );




        float calc_hourly_rotpoints_at_temp( const units::temperature &temp ) const;








        void calc_rot( units::temperature temp, float spoil_modifier, const time_duration &time_delta );






        void calc_rot_while_processing( time_duration processing_duration );











        bool process_temperature_rot( float insulation, const tripoint_bub_ms &pos, map &here,
                                      Character *carrier,
                                      temperature_flag flag = temperature_flag::NORMAL, float spoil_modifier = 1.0f,
                                      bool watertight_container = false );


        void heat_up();


        void cold_up();


        void set_item_temperature( units::temperature new_temperature );


        void set_item_specific_energy( const units::specific_energy &specific_energy );




        float get_item_thermal_energy() const;


        void reset_temp_check();

        int get_comestible_fun() const;


        bool goes_bad() const;


        time_duration get_shelf_life() const;


        double get_relative_rot() const;


        void set_relative_rot( double val );

        void set_rot( time_duration val );





        void randomize_rot();
















        int spoilage_sort_order() const;


        bool is_fresh() const {
            return goes_bad() && get_relative_rot() < 0.1;
        }


        bool is_going_bad() const {
            return get_relative_rot() > 0.9;
        }


        bool rotten() const {
            return get_relative_rot() > 1.0;
        }






        bool has_rotten_away() const;


        void apply_freezerburn();

        time_duration get_rot() const {
            return rot;
        }
        void mod_rot( const time_duration &val ) {
            rot += val;
        }

        bool is_smokable() const;


        time_duration brewing_time() const;

        const std::map<std::pair<itype_id, std::string>, int> &brewing_results() const;


        time_duration composting_time() const;

        const std::map<std::pair<itype_id, std::string>, int> &composting_results() const;







        bool detonate( const tripoint_bub_ms &p, std::vector<item> &drops );

        bool will_explode_in_fire() const;





















        const material_type &get_random_material() const;




        const material_type &get_base_material() const;





        const std::map<material_id, int> &made_of() const;






        std::vector<const part_material *> armor_made_of( const bodypart_id &bp ) const;
        std::vector<const part_material *> armor_made_of( const sub_bodypart_id &bp ) const;



        std::vector<const material_type *> made_of_types() const;





        bool made_of_any( const std::set<material_id> &mat_idents ) const;





        bool only_made_of( const std::set<material_id> &mat_idents ) const;





        int made_of( const material_id &mat_ident ) const;




        bool can_repair_with( const material_id &mat_ident ) const;



        bool made_of( phase_id phase ) const;
        bool made_of_from_type( phase_id phase ) const;




        std::vector<item_comp> get_uncraft_components() const;



        bool conductive() const;





        bool flammable( int threshold = 0 ) const;






        float get_clothing_mod_val_for_damage_type( const damage_type_id &dmg_type ) const;





        bool damage_type_can_damage_items( const damage_type_id &dmg_type ) const;
















        template<typename bodypart_target = bodypart_id>
        float resist( const damage_type_id &dmg_type, bool to_self = false,
                      const bodypart_target &bp = bodypart_target(),
                      int resist_value = 0 ) const;

    private:
        float _resist( const damage_type_id &dmg_type, bool to_self = false, int resist_value = 0,
                       bool bp_null = true,
                       const std::vector<const part_material *> &armor_mats = {},
                       float avg_thickness = 1.0f ) const;










        float _environmental_resist( const damage_type_id &dmg_type, bool to_self = false,
                                     int base_env_resist = 0,
                                     bool bp_null = true,
                                     const std::vector<const part_material *> &armor_mats = {} ) const;

    public:







        int breathability( const bodypart_id &bp ) const;
        int breathability() const;








        void mitigate_damage( damage_unit &du, const bodypart_id &bp = bodypart_id(), int roll = 0 ) const;
        void mitigate_damage( damage_unit &du, const sub_bodypart_id &bp, int roll = 0 ) const;






        int chip_resistance( bool worst = false, const bodypart_id &bp = bodypart_id() ) const;


        int damage() const;


        int degradation() const;


        void rand_degradation();


        int damage_level( bool precise = false ) const;





        bool activation_success() const;


        float damage_adjusted_melee_weapon_damage( float value, const damage_type_id &dt ) const;

        float damage_adjusted_gun_damage( float value ) const;

        float damage_adjusted_armor_resist( float value, const damage_type_id &dmg_type ) const;


        int max_damage() const;






        int repairable_levels() const;


        float get_relative_health() const;









        bool mod_damage( int qty, const Character *holder = nullptr );






        bool inc_damage( const Character *holder = nullptr );

        enum class armor_status {
            UNDAMAGED,
            DAMAGED,
            DESTROYED,
            TRANSFORMED
        };







        armor_status damage_armor_durability( damage_unit &du, damage_unit &premitigated,
                                              const bodypart_id &bp,
                                              double enchant_multiplier = 1,
                                              const Character *holder = nullptr );






        armor_status damage_armor_transforms( damage_unit &du, double enchant_multiplier = 1 ) const;


        std::string damage_indicator() const;










        std::string durability_indicator( bool include_intact = false ) const;


        const std::set<itype_id> &repaired_with() const;





        bool already_used_by_player( const Character &p ) const;




        void mark_as_used_by_player( const Character &p );


        bool is_filthy() const;
















        bool process( map &here, Character *carrier, const tripoint_bub_ms &pos, float insulation = 1,
                      temperature_flag flag = temperature_flag::NORMAL, float spoil_multiplier_parent = 1.0f,
                      bool watertight_container = false, bool recursive = true );

        bool leak( map &here, Character *carrier, const tripoint_bub_ms &pos,
                   item_pocket *pocke = nullptr );



        std::vector<desired_wakeup> enumerate_scheduled_wakeups( const item_location &loc ) const;


        void actualize_scheduled( item_wakeup_kind kind, time_point now,
                                  const item_location &loc );

        struct link_data {

            link_state source = link_state::no_link;

            link_state target = link_state::no_link;

            safe_reference<vehicle> t_veh; // NOLINT(cata-serialize)

            tripoint_abs_ms t_abs_pos = tripoint_abs_ms::invalid;

            point_rel_ms t_mount = point_rel_ms::zero;

            tripoint_abs_ms s_abs_pos = tripoint_abs_ms::invalid; // NOLINT(cata-serialize)

            time_point last_processed = calendar::turn;

            int length = 0;

            int max_length = 2;

            int charge_rate = 0;

            float efficiency = 0.0f;

            int charge_interval = 0;

            void serialize( JsonOut &jsout ) const;
            void deserialize( const JsonObject &data );
        };

        static const int LINK_RESPOOL_THRESHOLD = 6;



        item::link_data &link();
        const item::link_data &link() const;

        bool has_link_data() const;

        bool can_link_up() const;


        bool link_has_state( link_state state ) const;


        bool link_has_states( link_state source, link_state target ) const;

        bool has_no_links() const;


        std::string link_name() const;







        ret_val<void> link_to( const optional_vpart_position &linked_vp,
                               link_state link_type = link_state::no_link );







        ret_val<void> link_to( const optional_vpart_position &first_linked_vp,
                               const optional_vpart_position &second_linked_vp,
                               link_state link_type = link_state::no_link );







        ret_val<void> link_to( vehicle &veh, const point_rel_ms &mount,
                               link_state link_type = link_state::no_link );






        void update_link_traits();






        int link_length() const;





        int max_link_length() const;




        int link_sort_key() const;











        bool reset_link( bool unspool_if_too_long = true, Character *p = nullptr, int vpart_index = -1,
                         bool loose_message = false, tripoint_bub_ms cable_position = tripoint_bub_ms::zero );










        int charge_linked_batteries( vehicle &linked_veh, int turns_elapsed = 1 );




        bool needs_processing() const;



        int processing_speed() const;
        static constexpr int NO_PROCESSING = 10000;






        void process_relic( Character *carrier, const tripoint_bub_ms &pos );

        void overwrite_relic( const relic &nrelic );


        bool is_null() const;
        bool is_comestible() const;
        bool is_food() const;
        bool is_food_container() const;
        bool is_ammo_container() const;
        bool is_medication() const;
        bool is_medical_tool() const;
        bool is_bionic() const;
        bool is_magazine() const;
        bool is_battery() const;
        bool is_vehicle_battery() const;
        bool is_ammo_belt() const;
        bool is_holster() const;
        bool is_ammo() const;


        bool is_pet_armor( bool on_pet = false ) const;
        bool is_armor() const;
        bool is_book() const;
        bool is_map() const;
        bool is_salvageable() const;
        bool is_disassemblable() const;
        bool is_craft() const;

        bool is_deployable() const;
        bool is_tool() const;
        bool is_transformable() const;
        bool is_relic() const;
        bool is_same_relic( item const &rhs ) const;
        bool is_same_temperature( item const &rhs ) const;
        bool is_bucket_nonempty() const;

        bool is_brewable() const;
        bool is_compostable() const;
        bool is_engine() const;
        bool is_wheel() const;
        bool is_fuel() const;
        bool is_toolmod() const;

        bool is_irremovable() const;


        bool is_identifiable() const;


        bool is_broken() const;


        bool is_broken_on_active() const;

        bool has_temperature() const;


        bool is_frozen_liquid() const;


        std::string get_book_skill() const;


        float get_specific_heat_liquid() const;
        float get_specific_heat_solid() const;


        float get_latent_heat() const;

        units::temperature get_freeze_point() const;

        void set_last_temp_check( const time_point &pt );


        int wind_resist() const;


        std::set<fault_id> faults_potential() const;

        std::set<fault_id> faults_potential_of_type( const std::string &fault_type ) const;


        int wheel_area() const;


        units::energy fuel_energy() const;

        std::string fuel_pump_terrain() const;







        ret_val<void> is_compatible( const item &it ) const;












        ret_val<void> can_contain( const item &it, bool nested = false,
                                   bool ignore_rigidity = false,
                                   bool ignore_pkt_settings = true,
                                   bool ignore_non_container_pocket = false,
                                   const item_location &parent_it = item_location(),
                                   units::volume remaining_parent_volume = 10000000_ml,
                                   bool allow_nested = true ) const;
        ret_val<void> can_contain( const item &it, int &copies_remaining, bool nested = false,
                                   bool ignore_rigidity = false,
                                   bool ignore_pkt_settings = true,
                                   bool ignore_non_container_pocket = false,
                                   const item_location &parent_it = item_location(),
                                   units::volume remaining_parent_volume = 10000000_ml,
                                   bool allow_nested = true ) const;
        ret_val<void> can_contain( const itype &tp ) const;
        ret_val<void> can_contain_partial( const item &it ) const;
        ret_val<void> can_contain_partial( const item &it, int &copies_remaining,
                                           bool nested = false ) const;
        ret_val<void> can_contain_directly( const item &it ) const;
        ret_val<void> can_contain_partial_directly( const item &it ) const;














        std::pair<item_location, item_pocket *> best_pocket( const item &it, item_location &this_loc,
                const item *avoid = nullptr, bool allow_sealed = false, bool ignore_settings = false,
                bool nested = false, bool ignore_rigidity = false, bool allow_nested = true );

        units::length max_containable_length( bool unrestricted_pockets_only = false ) const;
        units::length min_containable_length() const;
        units::volume max_containable_volume() const;






        bool is_reloadable() const;






        bool can_reload_with( const item &ammo, bool now ) const;





        bool can_unload() const;




        bool contains_no_solids() const;


        bool is_dangerous() const;


        bool is_tainted() const;




        bool is_soft() const;


        bool is_rigid() const;

        bool is_comfortable() const;
        template <typename T>
        bool is_bp_rigid( const T &bp ) const;

        template <typename T>
        bool is_bp_rigid_selective( const T &bp ) const;
        template <typename T>
        bool is_bp_comfortable( const T &bp ) const;





        void set_snippet( const snippet_id &id );

        bool operator<( const item &other ) const;




        std::string components_to_string() const;


        uint64_t make_component_hash() const;


        itype_id typeId() const;


        bool affects_fall() const;


        int fall_damage_reduction() const;



        bool will_spill() const;
        bool will_spill_if_unsealed() const;






        bool spill_contents( Character &c );





        bool spill_contents( const tripoint_bub_ms &pos );
        bool spill_contents( map *here, const tripoint_bub_ms &pos );
        bool spill_open_pockets( Character &guy, const item *avoid = nullptr );

        void overflow( map &here, const tripoint_bub_ms &pos,
                       const item_location &loc = item_location::nowhere );





        bool can_holster( const item &obj ) const;





        void on_wear( Character &p );




        void on_takeoff( Character &p );




        int on_wield_cost( const Character &you ) const;







        void on_wield( Character &you, bool combat = true );





        void on_pickup( Character &p );



        void on_contents_changed();

        bool can_use_relic( const Character &guy ) const;
        bool use_relic( Character &guy, const tripoint_bub_ms &pos );
        bool has_relic_recharge() const;
        bool has_relic_activation() const;










        std::string type_name( unsigned int quantity = 1, bool use_variant = true,
                               bool use_cond_name = true, bool use_corpse = true ) const;








        int charges_per_volume( const units::volume &vol, bool suppress_warning = false ) const;
        int charges_per_weight( const units::mass &m, bool suppress_warning = false ) const;










        double get_var( std::string_view key, double default_value ) const;
        std::string get_var( std::string_view key, std::string default_value = {} ) const;
        tripoint_abs_ms get_var( std::string_view key, tripoint_abs_ms default_value ) const;

        void set_var( const std::string &key, diag_value value );
        template <typename... Args>
        void set_var( const std::string &key, Args... args ) {
            set_var( key, diag_value{ std::forward<Args>( args )... } );
        }

        void remove_var( const std::string &key );
        diag_value const &get_value( std::string_view name ) const;
        diag_value const *maybe_get_value( std::string_view name ) const;

        bool has_var( std::string_view name ) const;

        void erase_var( const std::string &name );

        void clear_vars();


        struct extended_photo_def {
            int quality = 0;
            std::string name;
            std::string description;

            void deserialize( const JsonObject &obj );
            void serialize( JsonOut &jsout ) const;
        };
        bool read_extended_photos( std::vector<extended_photo_def> &extended_photos,
                                   std::string_view var_name, bool insert_at_begin ) const;
        void write_extended_photos( const std::vector<extended_photo_def> &, const std::string & );














        bool has_flag( const flag_id &flag ) const;

        template<typename Container, typename T = std::decay_t<decltype( *std::declval<const Container &>().begin() )>>
        bool has_any_flag( const Container &flags ) const {
            return std::any_of( flags.begin(), flags.end(), [&]( const T & flag ) {
                return has_flag( flag );
            } );
        }

        template<typename Container, typename T = std::decay_t<decltype( *std::declval<const Container &>().begin() )>>
        bool has_any_vitamin( const Container &vitamins ) const {
            return std::any_of( vitamins.begin(), vitamins.end(), [&]( const T & vitamin ) {
                return has_vitamin( vitamin );
            } );
        }






        bool has_own_flag( const flag_id &f ) const;


        const FlagsSetType &get_flags() const;


        const FlagsSetType &get_prefix_flags() const;


        const FlagsSetType &get_suffix_flags() const;


        item &set_flag( const flag_id &flag );








        bool set_fault( const fault_id &f_id, bool force = false,
                        const Character *holder = nullptr );





        void set_random_fault_of_type( const std::string &fault_type, bool force = false,
                                       const Character *holder = nullptr );


        bool remove_fault( const fault_id &fault_id );


        void remove_single_fault_of_type( const std::string &fault_type );


        bool can_have_fault( const fault_id &f_id );


        item &unset_flag( const flag_id &flag );


        item &set_flag_recursive( const flag_id &flag );


        void unset_flags();



        bool has_vitamin( const vitamin_id &vitamin ) const;

        std::string get_fault_description( const fault_id &f_id ) const;


        bool has_fault( const fault_id &fault ) const;

        bool has_fault_of_type( const std::string &fault_type ) const;


        bool has_fault_flag( const std::string &searched_flag ) const;










        bool has_property( const std::string &prop ) const;




        std::string get_property_string( const std::string &prop, const std::string &def = "" ) const;
        int64_t get_property_int64_t( const std::string &prop, int64_t def = 0 ) const;















        bool getlight( float &luminance, units::angle &width, units::angle &direction ) const;



        int getlight_emit() const;



        bool is_emissive() const;









        bool is_seed() const;




        std::string get_plant_name() const;












        bool covers( const bodypart_id &bp, bool check_ablative_armor = true ) const;



        bool covers( const sub_bodypart_id &bp, bool check_ablative_armor = true ) const;

        std::optional<side> covers_overlaps( const item &rhs ) const;









        body_part_set get_covered_body_parts() const;











        body_part_set get_covered_body_parts( side s ) const;




        std::vector<sub_bodypart_id> get_covered_sub_body_parts() const;




        std::vector<sub_bodypart_id> get_covered_sub_body_parts( side s ) const;




        bool has_sublocations() const;




        bool is_sided() const;



        side get_side() const;



        bool set_side( side s );




        bool swap_side();



        bool is_ablative() const;



        bool has_additional_encumbrance() const;



        bool has_ripoff_pockets() const;



        bool has_noisy_pockets() const;





        int get_warmth() const;

        int get_warmth( const bodypart_id &bp ) const;

        void get_warmth_by_bodypart( std::vector<std::pair<bodypart_id, int>> &result ) const;




        float get_thickness() const;




        float get_thickness( const bodypart_id &bp ) const;




        float get_thickness( const sub_bodypart_id &bp ) const;



        std::vector<layer_level> get_layer() const;




        std::vector<layer_level> get_layer( bodypart_id bp ) const;




        std::vector<layer_level> get_layer( sub_bodypart_id sbp ) const;





        bool has_layer( const std::vector<layer_level> &ll, const bodypart_id &bp ) const;




        bool has_layer( const std::vector<layer_level> &ll, const sub_bodypart_id &sbp ) const;




        bool has_layer( const std::vector<layer_level> &ll ) const;




        layer_level get_highest_layer( const sub_bodypart_id &sbp ) const;

        enum class cover_type {
            COVER_DEFAULT,
            COVER_MELEE,
            COVER_RANGED,
            COVER_VITALS
        };
        static cover_type get_cover_type( const damage_type_id &type );




        int get_avg_coverage( const cover_type &type = cover_type::COVER_DEFAULT ) const;

        int get_avg_coverage( const body_part_set &relevant_parts,
                              const cover_type &type = cover_type::COVER_DEFAULT ) const;





        int get_coverage( const bodypart_id &bodypart,
                          const cover_type &type = cover_type::COVER_DEFAULT ) const;

        int get_coverage( const sub_bodypart_id &bodypart,
                          const cover_type &type = cover_type::COVER_DEFAULT ) const;

        enum class encumber_flags : int {
            none = 0,
            assume_full = 1,
            assume_empty = 2
        };

        const armor_portion_data *portion_for_bodypart( const bodypart_id &bodypart ) const;

        const armor_portion_data *portion_for_bodypart( const sub_bodypart_id &bodypart ) const;





        int get_avg_encumber( const Character &, encumber_flags = encumber_flags::none ) const;






        int get_encumber( const Character &, const bodypart_id &bodypart,
                          encumber_flags = encumber_flags::none ) const;











        int get_env_resist( int override_base_resist = 0 ) const;






        int get_base_env_resist_w_filter() const;




        bool is_power_armor() const;




        int max_worn() const;




        const islot_armor *find_armor_data() const;



        bool is_worn_only_with( const item &it ) const;



        bool is_worn_by_player() const;








        units::volume get_pet_armor_max_vol() const;
        units::volume get_pet_armor_min_vol() const;
        bodytype_id get_pet_armor_bodytype() const;











        book_proficiency_bonuses get_book_proficiency_bonuses() const;





        static int pages_in_book( const itype &type );





        int get_chapters() const;





        int get_remaining_chapters( const Character &u ) const;




        void mark_chapter_as_read( Character &u );




        std::set<recipe_id> get_saved_recipes() const;



        void set_saved_recipes( const std::set<recipe_id> &recipes );




        void generate_recipes();




        std::vector<std::pair<const recipe *, int>> get_available_recipes( const Character &u ) const;












        bool has_technique( const matec_id &tech ) const;



        std::set<matec_id> get_techniques() const;




        void add_technique( const matec_id &tech );



        std::vector<item *> toolmods();
        std::vector<const item *> toolmods() const;









        bool is_gunmod() const;






        bool is_gun() const;





        bool has_itype_variant( bool check_option = true ) const;




        const itype_variant_data &itype_variant() const;




        void set_itype_variant( const std::string &variant );

        void clear_itype_variant();


        std::string variant_description() const;





        int shots_remaining( const map &here, const Character *carrier ) const;


        bool uses_energy() const;

        bool is_chargeable() const;






        units::energy energy_remaining( const Character *carrier = nullptr ) const;
        units::energy energy_remaining( const Character *carrier, bool ignoreExternalSources ) const;







        int ammo_remaining_linked( const map &here, const Character *carrier ) const;

        int ammo_remaining( const Character *carrier ) const;

        int ammo_remaining_linked( const map &here ) const;

        int ammo_remaining() const;

    private:
        units::energy energy_per_second() const;



        int ammo_remaining( const map &here, const std::set<ammotype> &ammo,
                            const Character *carrier,
                            bool include_linked ) const;
    public:






        int ammo_capacity( const ammotype &ammo, bool include_linked = false ) const;





        int remaining_ammo_capacity() const;







        int ammo_remaining( int well_idx ) const;
        int ammo_capacity( int well_idx ) const;
        int remaining_ammo_capacity( int well_idx ) const;


        int ammo_required() const;
        item &first_ammo();
        const item &first_ammo() const;





        void handle_liquid_or_spill( Character &guy, const item *avoid = nullptr );















        bool ammo_sufficient( const Character *carrier, int qty = 1 ) const;
        bool ammo_sufficient( const Character *carrier, const std::string &method, int qty = 1 ) const;










        int ammo_consume( int qty, const tripoint_bub_ms &pos, Character *carrier );
        int ammo_consume( int qty, map &here, const tripoint_bub_ms &pos, Character *carrier );










        units::energy energy_consume( units::energy qty, const tripoint_bub_ms &pos, Character *carrier,
                                      float fuel_efficiency = -1.0 );
        units::energy energy_consume( units::energy qty, map *here, const tripoint_bub_ms &pos,
                                      Character *carrier,
                                      float fuel_efficiency = -1.0 );









        int activation_consume( int qty, const tripoint_bub_ms &pos, Character *carrier );






        bool has_ammo() const;

        bool has_ammo_data() const;

        const itype *ammo_data() const;

        itype_id ammo_current() const;




        const item &loaded_ammo() const;



        ammotype ammo_type() const;






        std::set<ammotype> ammo_types( bool conversion = true ) const;





        itype_id ammo_default( bool conversion = true ) const;

        std::string print_ammo( ammotype at, const item *gun = nullptr ) const;






        itype_id common_ammo_default( bool conversion = true ) const;


        std::set<ammo_effect_str_id> ammo_effects( bool with_ammo = true ) const;


        std::string ammo_sort_name() const;


        int casings_count() const;


        void casings_handle( const std::function<bool( item & )> &func );


        bool magazine_integral() const;


        bool uses_magazine() const;






        itype_id magazine_default( bool conversion = false ) const;








        std::vector<itype_id> magazines_default() const;





        std::vector<item_pocket *> all_magazine_well_pockets();
        std::vector<const item_pocket *> all_magazine_well_pockets() const;







        void dress_magazine_wells( bool insert_default_mag, bool fill_with_default_ammo );






        std::set<itype_id> magazine_compatible() const;






        item *magazine_current();
        const item *magazine_current() const;

        std::vector<item *> magazines_current();
        std::vector<const item *> magazines_current() const;

        item_pocket *pocket_by_id( const std::string &id );
        const item_pocket *pocket_by_id( const std::string &id ) const;

        int ammo_remaining_in_pocket( const std::string &id ) const;
        int ammo_consume_in_pocket( const std::string &id, int qty, map &here,
                                    const tripoint_bub_ms &pos );

        bool uses_firing_requirements() const;


        bool needs_charges_to_use() const;




        const item_pocket *primary_ammo_pocket() const;




        const item_pocket *pocket_for_ammo( const ammotype &at ) const;



        const item *ammo_identity_mag() const;

        bool pocket_accepts_battery( const item_pocket *p ) const;
        bool pocket_is_primary_ammo( const item_pocket *p ) const;





        int effective_qty( const pocket_consumption_entry &e ) const;

        std::string format_consumption_requirements(
            const std::string &method = "",
            const gun_mode_id &mode = gun_mode_id( "DEFAULT" ),
            int uses = 1 ) const;


        int expected_cost_per_use( const std::string &method = "" ) const;



        int tool_uses_remaining( map &here, const Character *carrier ) const;
        int tool_uses_remaining_local() const;




        int feasible_tool_uses( int external_pool ) const;

        int available_cable_charges( map &here ) const;
        int available_ups_charges( const Character *carrier ) const;
        int available_bionic_charges( const Character *carrier ) const;

        int consume_shots( const gun_mode_id &mode, int shots, map &here,
                           const tripoint_bub_ms &pos, Character *carrier );
        int consume_tool_uses( int uses, map &here, const tripoint_bub_ms &pos,
                               Character *carrier );
        int consume_one_shot( map &here, const tripoint_bub_ms &pos, Character *carrier );


        std::vector<item *> gunmods();
        std::vector<const item *> gunmods() const;

        std::vector<const item *> mods() const;

        std::vector<const item *> softwares() const;

        std::vector<const item *> ebooks() const;

        std::vector<item *> efiles();
        std::vector<const item *> efiles() const;

        std::vector<const item *> cables() const;


        item *gunmod_find( const itype_id &mod );
        const item *gunmod_find( const itype_id &mod ) const;

        item *gunmod_find_by_flag( const flag_id &flag );





        ret_val<void> is_gunmod_compatible( const item &mod ) const;


        std::map<gun_mode_id, gun_mode> gun_all_modes() const;


        gun_mode gun_get_mode( const gun_mode_id &mode ) const;


        gun_mode gun_current_mode() const;


        gun_mode_id gun_get_mode_id() const;


        bool gun_set_mode( const gun_mode_id &mode );


        void gun_cycle_mode();



        bool firing_mode_blocked( const gun_mode_id &mode ) const;


        std::pair<int, int> sight_dispersion( const Character &character ) const;

        struct sound_data {

            int volume;

            std::string sound;
        };




        sound_data gun_noise( bool burst = false ) const;

        bool is_silent() const;









        int gun_range( const Character *p ) const;



        int gun_range( bool with_ammo = true ) const;







        int gun_recoil( const Character &p, bool bipod = false, bool ideal_strength = false ) const;





        damage_instance gun_damage( bool with_ammo = true, bool shot = false ) const;
        damage_instance gun_damage( itype_id ammo ) const;



        units::mass gun_base_weight() const;



        int min_cycle_recoil() const;



        int gun_dispersion( bool with_ammo = true, bool with_scaling = true ) const;



        float gun_shot_spread_multiplier() const;



        skill_id gun_skill() const;


        gun_type_type gun_type() const;


        std::map<gunmod_location, int> get_mod_locations() const;




        int get_free_mod_locations( const gunmod_location &location ) const;



        bool is_firearm() const;



        int get_reload_time() const;










        bool is_upgrade() const;







        const use_function *get_use( const std::string &use_name ) const;





        const item *get_usable_item( const std::string &use_name ) const;
        item *get_usable_item( const std::string &use_name );




        std::string get_corpse_name() const;






        static std::string nname( const itype_id &id, unsigned int quantity = 1 );



        static const itype *find_type( const itype_id &type );


        static std::optional<ammotype> ammotype_of( const itype_id &id );




        static bool count_by_charges( const itype_id &id );










        static bool type_is_defined( const itype_id &id );




        bool has_label() const;




        std::string label( unsigned int quantity = 0, bool use_variant = true,
                           bool use_cond_name = true, bool use_corpse = true ) const;

        bool has_infinite_charges() const;


        skill_id contextualize_skill( const skill_id &id ) const;



        int get_to_hit() const;






        bool release_monster( const tripoint_bub_ms &target, int radius = 0 );

        int contain_monster( const tripoint_bub_ms &target );

        time_duration age() const;
        void set_age( const time_duration &age );
        time_point birthday() const;
        void set_birthday( const time_point &bday );
        void handle_pickup_ownership( Character &c );




        std::vector<trait_id> template_traits;





        units::energy get_gun_energy_drain() const;
        units::energy get_gun_ups_drain() const;
        units::energy get_gun_bionic_drain() const;

        void validate_ownership() const;
        inline void set_old_owner( const faction_id &temp_owner ) {
            old_owner = temp_owner;
        }
        inline void remove_old_owner() const {
            old_owner = faction_id::NULL_ID();
        }
        void set_owner( const faction_id &new_owner );
        void set_owner( const Character &c );
        inline void remove_owner() const {
            owner = faction_id::NULL_ID();
        }
        faction_id get_owner() const;
        faction_id get_old_owner() const;
        bool is_owned_by( const Character &c, bool available_to_take = false ) const;
        bool is_owned_by( const monster &m, bool available_to_take = false ) const;
        bool is_old_owner( const Character &c, bool available_to_take = false ) const;
        std::string get_old_owner_name() const;
        std::string get_owner_name() const;
        int get_min_str() const;

        const cata::value_ptr<islot_comestible> &get_comestible() const;






        const recipe &get_making() const;
        int get_making_batch_size() const;








        int get_next_failure_point() const;






        void set_next_failure_point( const Character &crafter );







        bool handle_craft_failure( Character &crafter );






        requirement_data get_continue_reqs() const;






        void inherit_flags( const item &parent, const recipe &making );






        void inherit_flags( const item_components &parents, const recipe &making );

        void set_tools_to_continue( bool value );
        bool has_tools_to_continue() const;


        void set_step_tool_allocs( const std::vector<std::vector<step_tool_alloc>> &allocs );
        const std::vector<std::vector<step_tool_alloc>> &get_step_tool_allocs() const;



        int get_current_step() const;
        void set_current_step( int step );
        double get_step_progress() const;
        void set_step_progress( double progress );
        void mod_step_progress( double delta );


        const std::vector<attention_plan> &get_step_plans() const;
        void set_step_plans( std::vector<attention_plan> plans );


        time_point get_passive_started_at() const;
        void set_passive_started_at( time_point t );
        time_point get_ready_at() const;
        void set_ready_at( time_point t );
        time_point get_alarm_at() const;
        void set_alarm_at( time_point t );
        time_point get_fail_at() const;
        void set_fail_at( time_point t );
        time_point get_pause_started_at() const;
        void set_pause_started_at( time_point t );
        time_point get_saved_ready_at() const;
        void set_saved_ready_at( time_point t );
        time_point get_saved_alarm_at() const;
        void set_saved_alarm_at( time_point t );
        time_point get_saved_fail_at() const;
        void set_saved_fail_at( time_point t );
        time_point get_env_check_at() const;
        void set_env_check_at( time_point t );

        character_id get_crafter_id() const;
        void set_crafter_id( character_id id );

        int get_passive_start_counter() const;
        void set_passive_start_counter( int c );
        int get_passive_end_counter() const;
        void set_passive_end_counter( int c );

        bool is_awaiting_collection() const;
        void set_awaiting_collection( bool v );

        const std::vector<craft_reservation::binding> &get_reservations() const;
        void set_reservations( std::vector<craft_reservation::binding> b );
        std::optional<tripoint_abs_ms> get_reserved_tile() const;
        void set_reserved_tile( std::optional<tripoint_abs_ms> tile );

        int64_t reservation_owner_token();
        int64_t peek_reservation_owner_token() const;
        time_point get_reservation_expiry() const;
        void set_reservation_expiry( time_point t );
        uint8_t get_reservation_search_attempts() const;
        void set_reservation_search_attempts( uint8_t n );
        uint64_t get_reservation_pool_fingerprint() const;
        void set_reservation_pool_fingerprint( uint64_t f );
        uint8_t get_reservation_pause_reason() const;
        void set_reservation_pause_reason( uint8_t r );

        std::vector<enchant_cache> get_proc_enchantments() const;
        std::vector<enchantment> get_defined_enchantments() const;

        double calculate_by_enchantment_wield( const Character &owner, double modify,
                                               enchant_vals::mod value,
                                               bool round_value = false ) const;






        int get_recursive_disassemble_moves( const Character &guy ) const;


        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const;


        VisitResponse visit_contents( const std::function<VisitResponse( const item_location & )> &func,
                                      const item_location &parent, const std::set<pocket_type> &allowed_pockets = {pocket_type::CONTAINER} );
        VisitResponse visit_contents_legacy( const std::function<VisitResponse( item *, item * )> &func,
                                             item *parent,
                                             const std::set<pocket_type> &allowed_pockets = { pocket_type::CONTAINER } );
        std::vector<const item *> items_with( const item_filter &filter ) const;
        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const;
        bool has_item_with( const std::function<bool( const item & )> &filter ) const;
        bool has_item( const item & ) const;
        int amount_of( const itype_id &what, bool pseudo = true, int limit = INT_MAX,
                       const item_filter &filter = return_true<item> ) const;
        int charges_of( const itype_id &what, int limit = INT_MAX,
                        const item_filter &filter = return_true<item>,
                        const std::function<void( int )> &visitor = nullptr, bool in_tools = false ) const;
        void remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );


        std::list<const item *> all_items_top() const;

        std::list<item *> all_items_top();

        std::list<const item *> all_items_container_top() const;

        std::list<item *> all_items_container_top();

        std::list<const item *> all_items_top( pocket_type pk_type ) const;




        std::list<item *> all_items_top( pocket_type pk_type, bool unloading = false );

        item const *this_or_single_content() const;
        bool contents_only_one_type() const;
        struct aggregate_t {
            stacking_info info;
            item const *header{};
            unsigned int count{};

            aggregate_t() = default;
            explicit aggregate_t( item const *i )
                : header( i ), count( 1 ) {
                info.bits.set();
            };

            explicit operator bool() const noexcept {
                return header != nullptr;
            }
        };
        aggregate_t aggregated_contents( int depth = 0, int maxdepth = 2 ) const;





        std::list<const item *> all_items_ptr() const;
        std::list<item *> all_items_ptr();

        std::list<const item *> all_items_ptr( pocket_type pk_type ) const;

        std::list<item *> all_items_ptr( pocket_type pk_type );



        std::list<item *> all_known_contents();
        std::list<const item *> all_known_contents() const;

        std::list<item *> all_holstered_items();
        std::list<const item *> all_holstered_items() const;

        std::list<item *> all_ablative_armor();
        std::list<const item *> all_ablative_armor() const;

        void clear_items();

        void begin_bulk_fill();
        void end_bulk_fill();
        bool empty() const;

        bool empty_container() const;


        item &only_item();
        const item &only_item() const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        const item *get_item_with( const std::function<bool( const item & )> &filter ) const;






        size_t num_item_stacks() const;





        item &legacy_front();
        const item &legacy_front() const;




        void favorite_settings_menu( item_location il );

        void combine( const item_contents &read_input, bool convert = false );

        bool is_collapsed() const;

    private:

        void migrate_content_item( const item &contained );

        bool use_amount_internal( const itype_id &it, int &quantity, std::list<item> &used,
                                  const std::function<bool( const item & )> &filter = return_true<item> );
        const use_function *get_use_internal( const std::string &use_name ) const;
        template<typename Item>
        static Item *get_usable_item_helper( Item &self, const std::string &use_name );
        bool process_internal( map &here, Character *carrier, const tripoint_bub_ms &pos, float insulation,
                               temperature_flag flag, float spoil_modifier, bool watertight_container );
        void iterate_covered_body_parts_internal( side s,
                const std::function<void( const bodypart_str_id & )> &cb,
                bool check_ablative_armor = true ) const;
        void iterate_covered_sub_body_parts_internal( side s,
                const std::function<void( const sub_bodypart_str_id & )> &cb,
                bool check_ablative_armor = true ) const;






        void calc_temp( const units::temperature &temp, float insulation, const time_duration &time_delta );


        units::specific_energy get_specific_energy_from_temperature( const units::temperature
                &new_temperature )
        const;


        void set_temp_flags( units::temperature new_temperature, float freeze_percentage );

        std::list<item *> all_items_top_recursive( pocket_type pk_type );
        std::list<const item *> all_items_top_recursive( pocket_type pk_type ) const;


        bool armor_full_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const;

        void update_inherited_flags();



        void update_prefix_suffix_flags();
        void update_prefix_suffix_flags( const flag_id &flag );

        void inherit_rot_from_components( item &it );

    public:
        enum class sizing : int {
            human_sized_human_char = 0,
            big_sized_human_char,
            small_sized_human_char,
            big_sized_big_char,
            human_sized_big_char,
            small_sized_big_char,
            small_sized_small_char,
            human_sized_small_char,
            big_sized_small_char,
            ignore
        };

        sizing get_sizing( const Character & ) const;

    protected:



        bool process_corpse( map &here, Character *carrier, const tripoint_bub_ms &pos );
        bool process_wet( Character *carrier, const tripoint_bub_ms &pos );
        bool process_litcig( map &here, Character *carrier, const tripoint_bub_ms &pos );
        bool process_extinguish( map &here, Character *carrier, const tripoint_bub_ms &pos );

        bool process_fake_smoke( map &here, Character *carrier, const tripoint_bub_ms &pos );
        bool process_fake_mill( map &here, Character *carrier, const tripoint_bub_ms &pos );
        bool process_link( map &here, Character *carrier, const tripoint_bub_ms &pos );
        bool process_linked_item( Character *carrier, const tripoint_bub_ms &pos,
                                  link_state required_state );
        bool process_blackpowder_fouling( Character *carrier );
        bool process_gun_cooling( Character *carrier );
        bool process_tool( Character *carrier, const tripoint_bub_ms &pos );
        bool process_decay_in_air( map &here, Character *carrier, const tripoint_bub_ms &pos,
                                   int max_air_exposure_hours,
                                   time_duration time_delta );


    public:
        static const int INFINITE_CHARGES;

        const itype *type;
        item_components components;

        cata::heap<std::set<fault_id>> faults;
        const mtype *get_corpse_mon() const;

    private:
        item_contents contents;




        bool requires_tags_processing = true;
        uint64_t hot_flags_own = 0;
        uint64_t hot_flags_inherited = 0;
    public:

        uint64_t combined_hot_flags() const;
        uint64_t own_hot_flags() const {
            return hot_flags_own;
        }
    private:
        cata::heap<FlagsSetType> item_tags;
        cata::heap<FlagsSetType> inherited_tags_cache;
        cata::heap<FlagsSetType> prefix_tags_cache;
        cata::heap<FlagsSetType> suffix_tags_cache;
        lazy<safe_reference_anchor> anchor;
        cata::heap<global_variables::impl_t> item_vars;
        const mtype *corpse = nullptr;
        std::string corpse_name;
        cata::heap<std::set<matec_id>> techniques;





        void select_itype_variant();

        bool can_have_itype_variant() const;


        bool possible_itype_variant( const std::string &test ) const;


        const itype_variant_data *_itype_variant = nullptr;




        class craft_data
        {
            public:
                const recipe *making = nullptr;
                int next_failure_point = -1;
                std::vector<item_comp> comps_used;

                bool tools_to_continue = false;
                int batch_size = -1;
                std::vector<std::vector<step_tool_alloc>> step_tool_allocs;
                std::optional<units::mass> cached_weight; // NOLINT(cata-serialize)
                std::optional<units::volume> cached_volume; // NOLINT(cata-serialize)



                int current_step = 0;
                double step_progress = 0.0;


                std::vector<attention_plan> step_plans;



                time_point passive_started_at = calendar::before_time_starts;
                time_point ready_at  = calendar::before_time_starts;
                time_point alarm_at  = calendar::before_time_starts;
                time_point fail_at   = calendar::before_time_starts;




                time_point pause_started_at = calendar::before_time_starts;
                time_point saved_ready_at = calendar::before_time_starts;
                time_point saved_alarm_at = calendar::before_time_starts;
                time_point saved_fail_at  = calendar::before_time_starts;



                time_point env_check_at = calendar::before_time_starts;



                int passive_start_counter = 0;
                int passive_end_counter = 0;



                bool awaiting_collection = false;

                std::vector<craft_reservation::binding> reservations;


                std::optional<tripoint_abs_ms> reserved_tile;


                int64_t reservation_owner = 0;


                time_point reservation_expires_at = calendar::before_time_starts;


                uint8_t reservation_search_attempts = 0;

                uint64_t reservation_pool_fingerprint = 0;


                uint8_t reservation_pause_reason = 0;



                character_id crafter_id;


                bool disassembly = false;
                void serialize( JsonOut &jsout ) const;
                void deserialize( const JsonObject &obj );
        };

        cata::value_ptr<craft_data> craft_data_;
    public:

        cata::value_ptr<relic> relic_data;
        int charges = 0;
        units::energy energy = 0_mJ;

        int recipe_charges = 1;
        int burnt = 0;
        int poison = 0;
        int frequency = 0;
        snippet_id snip_id = snippet_id::NULL_ID();
        int irradiation = 0;
        int item_counter = 0;


        time_point countdown_point = calendar::turn_max;

        units::specific_energy specific_energy = units::from_joule_per_gram(
                    -10 );
        units::temperature temperature = units::from_kelvin( 0 );
        int mission_id = -1;
        int player_id = -1;
        bool ethereal = false;
        int wetness = 0;

        int seed = rng( 0, INT_MAX );

        harvest_drop_type_id dropped_from =
            harvest_drop_type_id::NULL_ID();






        bool encumbrance_update_ = false;

        item_contents &get_contents() {
            return contents;
        };

        const item_contents &get_contents() const {
            return contents;
        };

    private:





        time_duration rot = 0_turns;

        time_point last_temp_check = calendar::turn_zero;

        time_point bday;
        item_uid uid_;






        phase_id current_phase = static_cast<phase_id>( 0 );

        mutable faction_id owner = faction_id::NULL_ID();

        mutable faction_id old_owner = faction_id::NULL_ID();
        int damage_ = 0;
        int degradation_ = 0;
        light_emission light = nolight;
        mutable std::optional<float> cached_relative_encumbrance;
        mutable cata::value_ptr<link_data> link_;

        struct cat_cache {
            item_category_id id;
            time_point timestamp = calendar::turn_max;
        };
        mutable cat_cache cached_category;


        bool browsed;

        units::volume additional_encumbrance = 0_ml;

    public:
        char invlet = 0;
        bool active = false;

        bool is_active() const {
            return active;
        }
        bool is_favorite = false;

        void set_favorite( bool favorite );
        bool has_clothing_mod() const;
        float get_clothing_mod_val( clothing_mod_type type ) const;
        void update_clothing_mod_val();
};

extern template float item::resist<bodypart_id>( const damage_type_id &dmg_type, bool to_self,
        const bodypart_id &bp,
        int resist_value ) const;
extern template float item::resist<sub_bodypart_id>( const damage_type_id &dmg_type,
        bool to_self,
        const sub_bodypart_id &bp,
        int resist_value ) const;

template<>
struct enum_traits<item::encumber_flags> {
    static constexpr bool is_flag_enum = true;
};

bool item_compare_by_charges( const item &left, const item &right );
bool item_ptr_compare_by_charges( const item *left, const item *right );





enum class hint_rating {

    good,

    iffy,

    cant
};


constexpr units::mass TOOL_LIFT_FACTOR = 500_kilogram;

inline units::mass lifting_quality_to_mass( int quality_level )
{
    return TOOL_LIFT_FACTOR * quality_level;
}




inline bool is_crafting_component( const item &component )
{
    return ( component.allow_crafting_component() || component.count_by_charges() ) &&
           !component.is_filthy();
}




bool is_preferred_component( const item &component );

struct disp_mod_by_barrel {
    units::length barrel_length;
    int dispersion_modifier;

    disp_mod_by_barrel();
    disp_mod_by_barrel( units::length bl, int disp ) : barrel_length( bl ),
        dispersion_modifier( disp ) {}
    void deserialize( const JsonObject &jo );
};







std::vector<std::pair<const item *, int>> get_item_duplicate_counts(
        const std::list<const item *> &items );

#endif
