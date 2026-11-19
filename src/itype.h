#pragma once
#ifndef CATA_SRC_ITYPE_H
#define CATA_SRC_ITYPE_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "calendar.h"
#include "color.h"
#include "coords_fwd.h"
#include "damage.h"
#include "enums.h"
#include "explosion.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "global_vars.h"
#include "item.h"
#include "item_pocket.h"
#include "item_transformation.h"
#include "iuse.h"
#include "mapdata.h"
#include "material.h"
#include "proficiency.h"
#include "relic.h"
#include "stomach.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "weighted_list.h"

// IWYU pragma: no_forward_declare std::hash
class Character;
class Item_factory;
class Trait_group;
class map;
template <typename E> struct enum_traits;

class gun_modifier_data
{
    private:
        translation name_;
        int qty_;
        std::set<std::string> flags_;

    public:
        gun_modifier_data() = default;
        gun_modifier_data( const translation &n, const int q, const std::set<std::string> &f ) : name_( n ),
            qty_( q ), flags_( f ) { }
        const translation &name() const {
            return name_;
        }

        std::string tname() const {
            return name_.translated();
        }
        int qty() const {
            return qty_;
        }
        const std::set<std::string> &flags() const {
            return flags_;
        }
        void deserialize( const JsonObject &jo );
};

class gunmod_location
{
    private:
        std::string _id;

    public:
        gunmod_location() = default;
        explicit gunmod_location( const std::string &id ) : _id( id ) { }


        std::string name() const;

        std::string str() const {
            return _id;
        }

        bool operator==( const gunmod_location &rhs ) const {
            return _id == rhs._id;
        }
        bool operator<( const gunmod_location &rhs ) const {
            return _id < rhs._id;
        }

        void deserialize( const JsonValue &jo ) {
            _id = jo.get_string();
        }
};

struct pocket_consumption_entry {
    std::string pocket;
    int qty = 0;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};



struct pocket_consumption_mod {
    std::string pocket;
    std::optional<int> set;
    float multiply = 1.0f;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};



struct pocket_capacity_mod {
    std::string pocket;
    float multiply = 1.0f;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};



struct firing_requirement_set {
    std::map<gun_mode_id, std::vector<pocket_consumption_entry>> per_mode;

    bool empty() const {
        return per_mode.empty();
    }
    const std::vector<pocket_consumption_entry> *for_mode( const gun_mode_id &m ) const {
        const auto it = per_mode.find( m );
        return it == per_mode.end() ? nullptr : &it->second;
    }

    bool was_loaded = false;
    void deserialize_firing_requirements( const JsonObject &jo, std::string_view member );
    void deserialize_consumption_per_use( const JsonObject &jo, std::string_view member );
};

struct islot_tool {
    std::set<ammotype> ammo_id;

    translation revert_msg;

    itype_id subtype;

    int max_charges = 0;
    int def_charges = 0;
    int charge_factor = 1;
    int charges_per_use = 0;
    int turns_per_charge = 0;
    units::power power_draw = 0_W;

    float fuel_efficiency = -1.0f;


    units::ememory etransfer_rate = 0_KB;


    std::string e_port;

    std::vector<std::string> e_ports_banned;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

constexpr float base_metabolic_rate =
    2500.0f;

struct rot_spawn_data {

    mtype_id rot_spawn_monster = mtype_id::NULL_ID();
    mongroup_id rot_spawn_group = mongroup_id::NULL_ID();

    float rot_spawn_chance;

    std::pair<int, int> rot_spawn_monster_amount;


    void load( const JsonObject &jo, bool was_loaded );
    bool handle_extend( const JsonValue &jv );
    void deserialize( const JsonObject &jo );
};

struct islot_comestible {
    public:
        friend Item_factory;
        friend item;

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );


        std::string comesttype;


        itype_id tool = itype_id::NULL_ID();


        int def_charges = 0;


        int stack_size = 0;


        int quench = 0;


        time_duration spoils = 0_turns;


        std::map<addiction_id, int> addictions;


        int stim = 0;


        int sleepiness_mod = 0;


        itype_id cooks_like;


        itype_id eats_like;


        itype_id smoking_result;





        nutrients default_nutrition_read_only() const {
            return default_nutrition;
        }


        void set_default_nutrition( nutrients new_nutrition ) {
            default_nutrition = std::move( new_nutrition );
        };


        int healthy = 0;


        int parasites = 0;


        float freeze_point = 0;


        std::set<std::string> petfood;


        std::vector<effect_on_condition_id> consumption_eocs;


        std::map<diseasetype_id, float> contamination;


        float specific_heat_liquid = 4.186f; // NOLINT(cata-serialize)
        float specific_heat_solid = 2.108f; // NOLINT(cata-serialize)
        float latent_heat = 333.0f; // NOLINT(cata-serialize)


        int monotony_penalty = -1;


        static constexpr float kcal_per_nutr = base_metabolic_rate / ( 12 * 24 );

        bool has_calories() const {
            return default_nutrition.calories > 0;
        }

        int get_default_nutr() const {
            return default_nutrition.kcal() / kcal_per_nutr;
        }

        std::list<itype_id> get_seasonings() const;


        rot_spawn_data rot_spawn;

    private:

        std::vector<item_group_id> seasonings;



        nutrients default_nutrition;


        int fun = 0;


        int default_addict_potential = 0;
};

struct islot_brewable {

    std::map<std::pair<itype_id, std::string>, int> results;


    time_duration time = 0_turns;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

struct islot_compostable {

    std::map<std::pair<itype_id, std::string>, int> results;


    time_duration time = 0_turns;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};


struct part_material {
    material_id id;
    int cover;
    float thickness;
    bool ignore_sheet_thickness = false;

    bool operator ==( const part_material &comp ) const {
        return id == comp.id && cover == comp.cover && thickness == comp.thickness;
    }
    part_material() : id( material_id::NULL_ID() ), cover( 100 ), thickness( 0.0f ) {}
    part_material( material_id id, int cover, float thickness ) :
        id( id ), cover( cover ), thickness( thickness ) {}
    part_material( const std::string &id, int cover, float thickness ) :
        id( material_id( id ) ), cover( cover ), thickness( thickness ) {}

    void deserialize( const JsonObject &jo );
};


enum class encumbrance_modifier : int {
    IMBALANCED = 0,
    RESTRICTS_NECK,
    WELL_SUPPORTED,
    NONE,
    last
};

template<>
struct enum_traits<encumbrance_modifier> {
    static constexpr encumbrance_modifier last = encumbrance_modifier::last;
};


enum class encumbrance_modifier_type : int {
    MULT = 0,
    FLAT,
    last
};

enum class item_display_type {
    DEFAULT,
    BY_WEIGHT,
    BY_VOLUME,
    BY_LENGTH,
    LAST
};

template<>
struct enum_traits<item_display_type> {
    static constexpr item_display_type last = item_display_type::LAST;
};

struct armor_portion_data {


    static constexpr units::volume volume_per_encumbrance = 250_ml; // NOLINT(cata-serialize)


    std::vector<encumbrance_modifier> encumber_modifiers;


    int encumber = 0;


    int max_encumber = -1;


    float volume_encumber_modifier = 1;



    int coverage = 0;
    int cover_melee = 0;
    int cover_ranged = 0;
    int cover_vitals = 0;





    float avg_thickness = 0.0f;



    int env_resist = 0;



    int env_resist_w_filter = 0;





    std::vector<part_material> materials;


    std::optional<body_part_set> covers;

    std::set<sub_bodypart_str_id> sub_coverage;


    std::set<layer_level> layers;





    int best_protection_chance = 100; // NOLINT(cata-serialize)



    int worst_protection_chance = 0; // NOLINT(cata-serialize)


    bool has_unique_layering = false; // NOLINT(cata-serialize)





    int breathability = -1; // NOLINT(cata-serialize)


    bool rigid = false; // NOLINT(cata-serialize)


    bool rigid_layer_only = false;


    bool comfortable = false; // NOLINT(cata-serialize)








    int max_coverage( bodypart_str_id bp ) const;


    static bool should_consolidate( const armor_portion_data &l, const armor_portion_data &r );


    int calc_encumbrance( units::mass weight, bodypart_id bp ) const;


    static std::tuple<encumbrance_modifier_type, int> convert_descriptor_to_val(
        encumbrance_modifier em );

    void deserialize( const JsonObject &jo );
};

struct islot_armor {
    public:


        static const int test_threshold = 40;




        bool sided = false;



        itype_id non_functional;



        translation damage_verb;



        int warmth = 0;





        int max_energy_shield_hp = 0;



        bool power_armor = false;



        bool ablative = false; // NOLINT(cata-serialize)



        bool additional_pocket_enc = false; // NOLINT(cata-serialize)



        bool ripoff_chance = false; // NOLINT(cata-serialize)




        bool rigid = false; // NOLINT(cata-serialize)




        bool comfortable = true; // NOLINT(cata-serialize)




        bool noisy = false; // NOLINT(cata-serialize)




        std::vector<std::string> valid_mods;




        bool has_sub_coverage = false; // NOLINT(cata-serialize)




        std::vector<armor_portion_data> data; // NOLINT(cata-serialize)



        std::vector<armor_portion_data> sub_data;


        std::vector<layer_level> all_layers; // NOLINT(cata-serialize)

        int avg_env_resist() const;
        int avg_env_resist_w_filter() const;
        float avg_thickness() const;

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );

    private:

        std::optional<float> _material_thickness = 0.0f;
        std::optional<int> _env_resist = 0;
        std::optional<int> _env_resist_w_filter = 0;
};

struct islot_pet_armor {



    float thickness = 0;



    int env_resist = 0;



    int env_resist_w_filter = 0;



    units::volume max_vol = 0_ml;



    units::volume min_vol = 0_ml;



    std::string bodytype = "none";



    bool power_armor = false;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

struct islot_book {



    skill_id skill = skill_id::NULL_ID();



    matype_id martial_art = matype_id::NULL_ID();



    int level = 0;



    int req = 0;



    int fun = 0;



    int intel = 0;




    time_duration time = 0_turns;




    bool generic = false;



    int chapters = 0;



    struct recipe_with_description_t {



        const class recipe *recipe;



        int skill_level;



        std::optional<translation> optional_name;



        bool hidden;
        bool operator<( const recipe_with_description_t &rhs ) const {
            return recipe < rhs.recipe;
        }
        bool is_hidden() const {
            return hidden;
        }
        std::string name() const;
    };
    using recipe_list_t = std::set<recipe_with_description_t>;
    recipe_list_t recipes; // NOLINT(cata-serialize)
    std::vector<book_proficiency_bonus> proficiencies;

    bool is_scannable = false;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

struct islot_mod {
    bool was_loaded = false;
    void deserialize( const JsonObject &jo );


    std::set<ammotype> acceptable_ammo;


    std::set<ammotype> ammo_modifier;


    std::map< ammotype, std::set<itype_id> > magazine_adaptor;






    std::vector<pocket_data> add_pockets;


    float capacity_multiplier = 1.0f;



    std::vector<pocket_consumption_mod> consumption_mods;



    std::vector<pocket_capacity_mod> capacity_mods;
};






struct common_ranged_data {





    damage_instance damage;



    int range = 0;



    float range_multiplier = 1.0;



    int dispersion = 0;
};

struct islot_engine {
        friend Item_factory;
        friend item;

    public:

        int displacement = 0;

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );
};

struct islot_wheel {
    public:

        int diameter = 0;


        int width = 0;

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );
};

enum class itype_variant_kind : int {
    gun,
    generic,
    drug,
    last
};

template<>
struct enum_traits<itype_variant_kind> {
    static constexpr itype_variant_kind last = itype_variant_kind::last;
};

struct itype_variant_data {
    std::string id;
    translation alt_name;
    translation alt_description;
    ascii_art_id art;
    std::optional<std::string> alt_sym;
    std::optional<nc_color> alt_color = std::nullopt;

    bool append = false;

    bool expand_snippets = false;

    int weight = 1;



    bool operator==( const itype_variant_data &rhs ) const {
        return id == rhs.id;
    }
    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
};


struct islot_gun : common_ranged_data {
    bool was_loaded = false;
    void deserialize( const JsonObject &jo );



    skill_id skill_used = skill_id::NULL_ID();



    std::set<ammotype> ammo;



    int durability = 0;



    int clip = 0;



    int reload_time = 100;



    translation reload_noise = to_translation( "click." );



    int reload_noise_volume = 0;


    int sight_dispersion = 30;


    int loudness = 0;




    units::energy energy_drain = 0_kJ;



    int blackpowder_tolerance = 8;



    int min_cycle_recoil = 0;



    units::volume barrel_volume = 0_ml;
    units::length barrel_length = 0_mm;



    std::set<ammo_effect_str_id> ammo_effects;





    std::map<gunmod_location, int> valid_mod_locations;



    std::set<itype_id> built_in_mods;



    std::set<itype_id> default_mods;


    std::map<gun_mode_id, gun_modifier_data> modes;


    int handling = -1;





    int recoil = 0;

    int ammo_to_fire = 1;





    double cooling_value = 100.0;




    double heat_per_shot = 0.0;






    double overheat_threshold = -1.0;




    double gun_jam_mult = 1.0;

    std::map<ammotype, std::set<itype_id>> cached_ammos; // NOLINT(cata-serialize)




    std::map<bodypart_str_id, int> hurt_part_when_fired;
};


class gun_type_type
{
    private:
        std::string name_;

    public:
        gun_type_type() = default;


        explicit gun_type_type( const std::string &name ) : name_( name ) {}

        std::string name() const;

        void deserialize( const JsonValue &jo ) {
            name_ = jo.get_string();
        }

        friend bool operator==( const gun_type_type &l, const gun_type_type &r ) {
            return l.name_ == r.name_;
        }

        friend struct std::hash<gun_type_type>;
};

namespace std
{

template<>
struct hash<gun_type_type> {
    size_t operator()( const gun_type_type &t ) const noexcept {
        return hash<std::string>()( t.name_ );
    }
};

}

struct islot_gunmod : common_ranged_data {
    bool was_loaded = false;
    void deserialize( const JsonObject &jo );


    gunmod_location location;


    std::unordered_set<gun_type_type> usable;


    int sight_dispersion = -1;






    int field_of_view = -1;





    int aim_speed = -1;




    double aim_speed_modifier = 0.0;


    int loudness = 0;


    float loudness_multiplier = 1;


    int to_hit_mod = 0;


    time_duration install_time = 0_seconds;


    float energy_drain_multiplier = 1.0f;


    units::energy energy_drain_modifier = 0_kJ;


    float ammo_to_fire_multiplier = 1.0f;


    int ammo_to_fire_modifier = 0;


    float weight_multiplier = 1.0f;


    std::map<gun_mode_id, gun_modifier_data> mode_modifier;



    firing_requirement_set firing_requirements;


    std::set<gun_mode_id> hide_modes;

    std::set<std::string> ammo_effects;


    int handling = 0;


    int reload_modifier = 0;


    int consume_chance = 10000;


    int consume_divisor = 1;


    float shot_spread_multiplier_modifier = 0.0f;


    int min_str_required_mod = 0;


    int min_str_required_mod_if_prone = 0;


    std::map<gunmod_location, int> add_mod;


    bool is_bayonet = false;




    bool is_visible_when_installed = false;


    std::set<gunmod_location> blacklist_slot;


    std::set<itype_id> blacklist_mod;

    units::length barrel_length = 0_mm;


    int overwrite_min_cycle_recoil = -1;


    double overheat_threshold_modifier = 0;
    float overheat_threshold_multiplier = 1.0f;


    double cooling_value_modifier = 0;
    float cooling_value_multiplier = 1.0f;


    double heat_per_shot_modifier = 0;
    float heat_per_shot_multiplier = 1.0f;
};

struct islot_magazine {
    bool was_loaded = false;
    void deserialize( const JsonObject &jo );


    std::set<ammotype> type;


    int capacity = 0;


    int count = 0;


    itype_id default_ammo = itype_id::NULL_ID();


    int reload_time = 100;


    double mag_jam_mult = 1.0 ;


    std::optional<itype_id> linkage;

    std::map<ammotype, std::set<itype_id>> cached_ammos; // NOLINT(cata-serialize)

    static std::map<itype_id, std::set<itype_id>> compatible_guns;
};

struct islot_battery {

    units::energy max_capacity;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

struct islot_ammo : common_ranged_data {



    ammotype type;



    std::optional<itype_id> casing;




    itype_id drop = itype_id::NULL_ID();

    float drop_chance = 1.0f;

    bool drop_active = true;





    int def_charges = 1;





    int stack_size = 0;




    int count = 1;



    int shot_spread = 0;



    damage_instance shot_damage;




    std::set<ammo_effect_str_id> ammo_effects;




    int loudness = -1;


    int recoil = 0;


    int recovery_chance = 0;






    bool cookoff = false; // NOLINT(cata-serialize)






    bool special_cookoff = false; // NOLINT(cata-serialize)




    float critical_multiplier = 2.0f;





    bool force_stat_display;




    std::vector<disp_mod_by_barrel> disp_mod_by_barrels;

    bool was_loaded = false;

    int dispersion_considering_length( units::length barrel_length ) const;
    void deserialize( const JsonObject &jo );
};

struct islot_bionic {
    bool was_loaded = false;
    void deserialize( const JsonObject &jo );



    int difficulty = 0;



    bionic_id id; // NOLINT(cata-serialize)



    bool is_upgrade = false;




    itype_id installation_data;
};

struct islot_seed {

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );




        translation plant_name;



        itype_id fruit_id;



        bool spawn_seeds = true;



        std::vector<itype_id> byproducts;



        ter_furn_flag required_terrain_flag = ter_furn_flag::TFLAG_PLANTABLE;
        islot_seed() = default;

        const std::vector<std::pair<flag_id, time_duration>> &get_growth_stages() const;
        units::temperature get_growth_temp() const;
    private:



        std::vector<std::pair<flag_id, time_duration>> growth_stages;

        units::temperature growth_temp;
};

enum condition_type {
    FLAG,
    VITAMIN,
    COMPONENT_ID,
    COMPONENT_ID_SUBSTRING,
    VAR,
    SNIPPET_ID,
    num_condition_types
};

template<>
struct enum_traits<condition_type> {
    static constexpr condition_type last = condition_type::num_condition_types;
};


struct conditional_name {

    condition_type type;

    std::string condition;

    std::string value;


    translation name;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

class islot_milling
{
    public:
        itype_id into_;
        recipe_id recipe_;

        bool was_loaded = false;
        void deserialize( const JsonObject &jo );
};

struct memory_card_info {
    float data_chance;
    itype_id on_read_convert_to;  // NOLINT(cata-serialize)

    float photos_chance;  // NOLINT(cata-serialize)
    int photos_amount;  // NOLINT(cata-serialize)

    float songs_chance;  // NOLINT(cata-serialize)
    int songs_amount; // NOLINT(cata-serialize)

    float recipes_chance;
    int recipes_amount;
    int recipes_level_min;
    int recipes_level_max;
    std::set<crafting_category_id> recipes_categories;
    bool secret_recipes;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
};

struct item_melee_damage {
    std::unordered_map<damage_type_id, float> damage_map;
    float default_value = 0.0f;  // NOLINT(cata-serialize)
    bool handle_proportional( const JsonValue &jval );
    item_melee_damage &operator+=( const item_melee_damage &rhs );
    void finalize();
    void deserialize( const JsonObject &jo );
};

struct itype {
        friend class Item_factory;
        friend struct mod_tracker;

        using FlagsSetType = cata::flat_set<flag_id>;






        cata::value_ptr<islot_tool> tool;
        cata::value_ptr<islot_comestible> comestible;
        cata::value_ptr<islot_brewable> brewable;
        cata::value_ptr<islot_compostable> compostable;
        cata::value_ptr<islot_armor> armor;
        cata::value_ptr<islot_pet_armor> pet_armor;
        cata::value_ptr<islot_book> book;
        cata::value_ptr<islot_mod> mod;
        cata::value_ptr<islot_engine> engine;
        cata::value_ptr<islot_wheel> wheel;
        cata::value_ptr<islot_gun> gun;
        cata::value_ptr<islot_gunmod> gunmod;
        cata::value_ptr<islot_magazine> magazine;
        cata::value_ptr<islot_battery> battery;
        cata::value_ptr<islot_bionic> bionic;
        cata::value_ptr<islot_ammo> ammo;
        cata::value_ptr<islot_seed> seed;
        cata::value_ptr<relic> relic_data;
        cata::value_ptr<islot_milling> milling_data;



        use_function drop_action;


        use_function countdown_action;


        std::map<std::string, use_function> tick_action;








        units::mass weight = 0_gram;

        units::mass integral_weight = -1_gram;

        units::ememory ememory_size = 0_KB;

        std::vector<std::pair<itype_id, mod_id>> src;


        std::vector<itype_variant_data> variants;


        std::vector<conditional_name> conditional_names;


        damage_instance thrown_damage;


        std::vector<recipe_id> recipes;


        std::vector<pocket_data> pockets;



        firing_requirement_set firing_requirements;



        int legacy_charges_per_use_factor = 1;


        std::vector<std::string> chat_topics;


        itype_id looks_like;


        itype_id repairs_like;

        std::string snippet_category;

        ascii_art_id picture_id;


        item_category_id category_force;

        std::string sym;

        requirement_id template_requirements;

    public:
        itype_id id = itype_id::NULL_ID();


        std::optional<itype_id> default_container;
        std::optional<std::string> default_container_variant;

        std::set<weapon_category_id> weapon_category;


        struct item_quality {
            int level = 0;
            float speed = 1.0f;


            item_quality &operator+=( const item_quality &rhs ) {
                level += rhs.level;
                return *this;
            }
        };


        std::map<quality_id, item_quality> qualities;

        std::map<quality_id, item_quality> charged_qualities;


        bool has_any_quality( std::string_view quality ) const;


        std::map<std::string, std::string> properties;


        global_variables::impl_t item_variables;





        std::map<material_id, int> materials;
        std::set<material_id> repairs_with;




        material_id default_mat;


        std::map<std::string, use_function> use_methods;


        const itype_id &tool_slot_first_ammo() const;


        std::map<std::string, int> ammo_scale;


        std::set<emit_id> emits;

        std::set<matec_id> techniques;


        std::map<skill_id, int> min_skills;


        std::set<itype_id> repair;


        weighted_int_list<fault_id> faults;








        std::vector<std::tuple<int, int, float, std::string>> fault_groups;


        std::map< ammotype, std::set<itype_id> > magazines;


        std::map< ammotype, itype_id > magazine_default;


        item_group_id nanofab_template_group;


        string_id<Trait_group> trait_group;


        mtype_id source_monster = mtype_id::NULL_ID();
    private:
        FlagsSetType item_tags;
        uint64_t hot_flag_bits = 0;
        friend class item;

    public:

        cata::value_ptr<memory_card_info> memory_card_data;

        explosion_data explosion;

        translation description;

    protected:


        translation name = no_translation( "none" );

    public:

        int mat_portion_total = 0;

        int min_str = 0;
        int min_dex = 0;
        int min_int = 0;
        int min_per = 0;


        itype_variant_kind variant_kind = itype_variant_kind::last;


        std::set<std::string> all_variant_names() const;

        phase_id phase = phase_id::SOLID;


        time_duration countdown_interval = 0_seconds;





        std::optional<item_transformation> transform_into;






        units::volume volume = 0_ml;





        units::volume integral_volume = -1_ml;




        units::length longest_side = -1_mm;




        units::length integral_longest_side = -1_mm;


        int stack_size = 0;


        units::money price = 0_cent;


        units::money price_post = -1_cent;


        item_display_type display_type = item_display_type::DEFAULT;


        int m_to_hit = -2;

        bool using_legacy_to_hit = false;

        unsigned light_emission = 0;

        nc_color color = c_white;






        float insulation_factor = 1.0f;


        int fall_damage_reduction = 0;




        float solar_efficiency = 0.0f;


        int max_worn = MAX_WORN_PER_TYPE;

    private:

        static constexpr int damage_max_ = 4000;
        int degrade_increments_ = 50;

    public:

        item_melee_damage melee;

        bool default_container_sealed = true;


        bool explode_in_fire = false;


        bool expand_snippets = false;

    private:

        bool stackable_ = false;

    public:
        static constexpr int damage_scale = 1000;


        int damage_max() const {
            return count_by_charges() ? 0 : damage_max_;
        }

        int degrade_increments() const {
            return degrade_increments_;
        }










        int damage_level( int damage ) const;





        const material_type &get_base_material() const;


        fuel_explosion_data get_explosion_data() const;

        std::string get_item_type_string() const;

        std::string item_measure_prefix( unsigned int quantity ) const;



        std::string nname( unsigned int quantity ) const;


        const itype_id &get_id() const {
            return id;
        }

        bool count_by_charges() const {
            return stackable_ || ammo || ( comestible && phase != phase_id::SOLID );
        }

        int charges_default() const;

        int charges_to_use() const;


        int charge_factor() const {
            return tool ? tool->charge_factor : 1;
        }

        int maximum_charges() const {
            if( tool ) {
                return tool->max_charges;
            }
            return 1;
        }
        bool can_have_charges() const;





        int charges_per_volume( const units::volume &vol ) const;

        bool has_use() const;

        bool has_flag( const flag_id &flag ) const;


        const FlagsSetType &get_flags() const;

        bool can_use( const std::string &iuse_name ) const;
        const use_function *get_use( const std::string &iuse_name ) const;

        bool has_tick( const std::string &iuse_name ) const;
        const use_function *get_tick( const std::string &iuse_name ) const;



        std::optional<int> invoke( Character *p, item &it,
                                   const tripoint_bub_ms &pos ) const;
        std::optional<int> invoke( Character *p, item &it,
                                   map *here, const tripoint_bub_ms &pos ) const;

        std::optional<int> invoke( Character *p, item &it, const tripoint_bub_ms &pos,
                                   const std::string &iuse_name ) const;
        std::optional<int> invoke( Character *p, item &it, map *here, const tripoint_bub_ms &pos,
                                   const std::string &iuse_name ) const;
        int tick( Character *p, item &it, const tripoint_bub_ms &pos ) const;

        virtual ~itype() = default;


        bool is_basic_component() const;


        bool was_loaded = false;
        void load( const JsonObject &jo, std::string_view src );
        void load_slots( const JsonObject &jo, bool was_loaded );
};

void load_charge_removal_blacklist( const JsonObject &jo, std::string_view src );
void load_temperature_removal_blacklist( const JsonObject &jo, std::string_view src );

#endif
