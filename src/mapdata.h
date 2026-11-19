#pragma once
#ifndef CATA_SRC_MAPDATA_H
#define CATA_SRC_MAPDATA_H

#include <array>
#include <bitset>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "calendar.h"
#include "clone_ptr.h"
#include "color.h"
#include "coords_fwd.h"
#include "enum_bitset.h"
#include "game_constants.h"
#include "iexamine.h"
#include "lightmap.h"
#include "requirements.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class Character;
class JsonObject;
class JsonValue;
struct connect_group;
struct furn_t;
struct itype;
struct ter_t;
template <typename T> class generic_factory;

namespace mapdata
{

extern generic_factory<ter_t> terrain_data;
extern generic_factory<furn_t> furniture_data;

}


const int NUM_TERCONN = 256;
connect_group get_connect_group( const std::string &name );

template <typename E> struct enum_traits;
struct map_data_common_t;

struct map_common_bash_info {

        int str_min = -1;

        int str_max = -1;

        int str_min_blocked = -1;
        int str_max_blocked = -1;

        int str_min_supported = -1;
        int str_max_supported = -1;
        int explosive = -1;
        int sound_vol = -1;
        int sound_fail_vol = -1;
        int collapse_radius = 1;
        bool destroy_only = false;

        bool bash_below = false;
        bash_damage_profile_id damage_profile;
        item_group_id drop_group;
        translation sound;
        translation sound_fail;
        std::vector<furn_str_id> tent_centers;
        std::pair<field_type_str_id, int> hit_field;
        std::pair<field_type_str_id, int> destroyed_field;

        void load( const JsonObject &jo, bool was_loaded, const std::string &context );
        void check( const std::string &id ) const;

        std::string potential_bash_items( const map_data_common_t &ter_furn ) const;

        int damage_to( const std::map<damage_type_id, int> &str,
                       bool supported = false, bool blocked = false ) const;
        int hp( bool supported = false, bool blocked = false ) const;
    public:
        virtual ~map_common_bash_info() = default;
};
struct map_ter_bash_info : map_common_bash_info {
    ter_str_id ter_set = ter_str_id::NULL_ID();

    ter_str_id ter_set_bashed_from_above = ter_str_id::NULL_ID();
    map_ter_bash_info() = default;
    void load( const JsonObject &jo, bool was_loaded, const std::string &context );
    void check( const std::string &id ) const;
};
struct map_furn_bash_info : map_common_bash_info {

    furn_str_id furn_set = furn_str_id::NULL_ID();
    map_furn_bash_info() = default;
    void load( const JsonObject &jo, bool was_loaded, const std::string &context );
    void check( const std::string &id ) const;
};
struct map_fd_bash_info : map_common_bash_info {
    int fd_bash_move_cost = 100;
    translation field_bash_msg_success;
    map_fd_bash_info() = default;
    void load( const JsonObject &jo, bool was_loaded, const std::string &context );
    void check( const std::string &id ) const;
};
struct map_deconstruct_skill {
    skill_id id;
    int min;
    int max;
    double multiplier;
};
struct map_common_deconstruct_info {

        bool deconstruct_above = false;

        item_group_id drop_group;
        std::optional<map_deconstruct_skill> skill;
        virtual void load( const JsonObject &jo, bool was_loaded, const std::string &context );
        virtual void check( const std::string &id ) const;
    public:
        virtual ~map_common_deconstruct_info() = default;

        std::string potential_deconstruct_items( const map_data_common_t &ter_furn ) const;
};
struct map_ter_deconstruct_info : map_common_deconstruct_info {
    ter_str_id ter_set = ter_str_id::NULL_ID();
    void load( const JsonObject &jo, bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
    map_ter_deconstruct_info() = default;
};
struct map_furn_deconstruct_info : map_common_deconstruct_info {
    furn_str_id furn_set = furn_str_id::NULL_ID();
    void load( const JsonObject &jo, bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
    map_furn_deconstruct_info() = default;
};
struct map_shoot_info {

    int chance_to_hit = 100;

    int reduce_dmg_min = 0;

    int reduce_dmg_max = 0;

    int reduce_dmg_min_laser = 0;

    int reduce_dmg_max_laser = 0;

    int destroy_dmg_min = 0;

    int destroy_dmg_max = 0;

    bool no_laser_destroy = false;
    bool load( const JsonObject &jsobj, std::string_view member, bool was_loaded );
};
struct furn_workbench_info {

    float multiplier;

    units::mass allowed_mass;
    units::volume allowed_volume;
    furn_workbench_info();
    bool load( const JsonObject &jsobj, std::string_view member );
};
struct plant_data {

    furn_str_id transform;

    furn_str_id base;

    float growth_multiplier;

    float harvest_multiplier;
    plant_data();
    bool load( const JsonObject &jsobj, std::string_view member );
};






































































enum class ter_furn_flag : int {
    TFLAG_TRANSPARENT,
    TFLAG_TRANSLUCENT,
    TFLAG_FLAMMABLE,
    TFLAG_REDUCE_SCENT,
    TFLAG_SWIMMABLE,
    TFLAG_SUPPORTS_ROOF,
    TFLAG_MINEABLE,
    TFLAG_NOITEM,
    TFLAG_NO_SIGHT,
    TFLAG_NO_SCENT,
    TFLAG_SEALED,
    TFLAG_ALLOW_FIELD_EFFECT,
    TFLAG_LIQUID,
    TFLAG_COLLAPSES,
    TFLAG_FLAMMABLE_ASH,
    TFLAG_DESTROY_ITEM,
    TFLAG_INDOORS,
    TFLAG_LIQUIDCONT,
    TFLAG_FIRE_CONTAINER,
    TFLAG_FLAMMABLE_HARD,
    TFLAG_SUPPRESS_SMOKE,
    TFLAG_SHARP,
    TFLAG_DIGGABLE,
    TFLAG_ROUGH,
    TFLAG_UNSTABLE,
    TFLAG_WALL,
    TFLAG_DEEP_WATER,
    TFLAG_SHALLOW_WATER,
    TFLAG_WATER_CUBE,
    TFLAG_CURRENT,
    TFLAG_THIN_ICE,
    TFLAG_THICK_ICE,
    TFLAG_SWIM_UNDER,
    TFLAG_HARVESTED,
    TFLAG_PERMEABLE,
    TFLAG_AUTO_WALL_SYMBOL,
    TFLAG_CONNECT_WITH_WALL,
    TFLAG_CLIMBABLE,
    TFLAG_GOES_DOWN,
    TFLAG_GOES_UP,
    TFLAG_NO_FLOOR,
    TFLAG_ALLOW_ON_OPEN_AIR,
    TFLAG_SEEN_FROM_ABOVE,
    TFLAG_RAMP_DOWN,
    TFLAG_RAMP_UP,
    TFLAG_RAMP,
    TFLAG_HIDE_PLACE,
    TFLAG_BLOCK_WIND,
    TFLAG_FLAT,
    TFLAG_RAIL,
    TFLAG_THIN_OBSTACLE,
    TFLAG_SMALL_PASSAGE,
    TFLAG_Z_TRANSPARENT,
    TFLAG_SUN_ROOF_ABOVE,
    TFLAG_FUNGUS,
    TFLAG_LOCKED,
    TFLAG_PICKABLE,
    TFLAG_WINDOW,
    TFLAG_DOOR,
    TFLAG_SHRUB,
    TFLAG_YOUNG,
    TFLAG_PLANT,
    TFLAG_FISHABLE,
    TFLAG_GRAZABLE,
    TFLAG_GRAZER_INEDIBLE,
    TFLAG_BROWSABLE,
    TFLAG_TREE,
    TFLAG_PLOWABLE,
    TFLAG_ORGANIC,
    TFLAG_CONSOLE,
    TFLAG_PLANTABLE,
    TFLAG_GROWTH_HARVEST,
    TFLAG_GROWTH_OVERGROWN,
    TFLAG_MOUNTABLE,
    TFLAG_RAMP_END,
    TFLAG_FLOWER,
    TFLAG_CAN_SIT,
    TFLAG_FLAT_SURF,
    TFLAG_BUTCHER_EQ,
    TFLAG_GROWTH_SEED,
    TFLAG_GROWTH_SEEDLING,
    TFLAG_GROWTH_MATURE,
    TFLAG_WORKOUT_ARMS,
    TFLAG_WORKOUT_LEGS,
    TFLAG_TRANSLOCATOR,
    TFLAG_TRANSLOCATOR_GREATER,
    TFLAG_AUTODOC,
    TFLAG_AUTODOC_COUCH,
    TFLAG_OPENCLOSE_INSIDE,
    TFLAG_SALT_WATER,
    TFLAG_PLACE_ITEM,
    TFLAG_BARRICADABLE_WINDOW_CURTAINS,
    TFLAG_CLIMB_SIMPLE,
    TFLAG_NANOFAB_TABLE,
    TFLAG_ROAD,
    TFLAG_TINY,
    TFLAG_SHORT,
    TFLAG_NOCOLLIDE,
    TFLAG_BARRICADABLE_DOOR,
    TFLAG_BARRICADABLE_DOOR_DAMAGED,
    TFLAG_BARRICADABLE_DOOR_REINFORCED,
    TFLAG_USABLE_FIRE,
    TFLAG_CONTAINER,
    TFLAG_NO_PICKUP_ON_EXAMINE,
    TFLAG_RUBBLE,
    TFLAG_DIGGABLE_CAN_DEEPEN,
    TFLAG_PIT_FILLABLE,
    TFLAG_DIFFICULT_Z,
    TFLAG_ALIGN_WORKBENCH,
    TFLAG_NO_SPOIL,
    TFLAG_EASY_DECONSTRUCT,
    TFLAG_LADDER,
    TFLAG_ALARMED,
    TFLAG_CHOCOLATE,
    TFLAG_SIGN,
    TFLAG_SIGN_ALWAYS,
    TFLAG_DONT_REMOVE_ROTTEN,
    TFLAG_BLOCKSDOOR,
    TFLAG_NO_SELF_CONNECT,
    TFLAG_BURROWABLE,
    TFLAG_MURKY,
    TFLAG_AMMOTYPE_RELOAD,
    TFLAG_TRANSPARENT_FLOOR,
    TFLAG_ELEVATOR,
    TFLAG_ACTIVE_GENERATOR,
    TFLAG_SMALL_HIDE,
    TFLAG_NO_FLOOR_WATER,
    TFLAG_SINGLE_SUPPORT,
    TFLAG_CLIMB_ADJACENT,
    TFLAG_FLOATS_IN_AIR,
    TFLAG_HARVEST_REQ_CUT1,
    TFLAG_NATURAL_UNDERGROUND,
    TFLAG_PHASE_BACK,
    TFLAG_WIRED_WALL,
    TFLAG_MON_AVOID_STRICT,
    TFLAG_REGION_PSEUDO,
    TFLAG_ONE_DIMENSIONAL_X,
    TFLAG_ONE_DIMENSIONAL_Y,
    TFLAG_ONE_DIMENSIONAL_Z,

    NUM_TFLAG_FLAGS
};

template<>
struct enum_traits<ter_furn_flag> {
    static constexpr ter_furn_flag last = ter_furn_flag::NUM_TFLAG_FLAGS;
};

struct connect_group {
    public:
        connect_group_id id;
        int index;
        std::set<ter_furn_flag> group_flags;
        std::set<ter_furn_flag> connects_to_flags;
        std::set<ter_furn_flag> rotates_to_flags;

        static void load( const JsonObject &jo );
        static void reset();
};

struct activity_byproduct {
    itype_id item;
    int count      = 0;
    int random_min = 0;
    int random_max = 0;

    int roll() const;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct pry_data {
    bool prying_nails = false;

    int difficulty = 0;
    int prying_level = 0;

    bool noisy = false;
    bool alarm = false;
    bool breakable = false;

    translation failure;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

class activity_data_common
{
    public:
        activity_data_common() = default;

        bool valid() const {
            return valid_;
        }

        const time_duration &duration() const {
            return duration_;
        }

        const translation &message() const {
            return message_;
        }

        const translation &sound() const {
            return sound_;
        }

        const pry_data &prying_data() const {
            return prying_data_;
        }

        const std::vector<activity_byproduct> &byproducts() const {
            return byproducts_;
        }

        bool was_loaded = false;
        void load( const JsonObject &jo );

    protected:
        bool valid_ = false;
        time_duration duration_;
        translation message_;
        translation sound_;
        struct pry_data prying_data_;
        std::vector<activity_byproduct> byproducts_;
};

class activity_data_ter : public activity_data_common
{
    public:
        activity_data_ter() = default;

        const ter_str_id &result() const {
            return result_;
        }

        void load( const JsonObject &jo );

    private:
        ter_str_id result_;
};

class activity_data_furn : public activity_data_common
{
    public:
        activity_data_furn() = default;

        const furn_str_id &result() const {
            return result_;
        }

        void load( const JsonObject &jo );

    private:
        furn_str_id result_;
};

void init_mapdata();


struct map_data_common_t {
        std::set<emit_id> emissions;
        translation lockpick_message;
        cata::value_ptr<map_shoot_info> shoot;

    public:
        virtual ~map_data_common_t() = default;
        virtual std::optional<map_common_bash_info> bash_info() const = 0;
        virtual std::optional<map_common_deconstruct_info> deconstruct_info() const = 0;
    protected:
        friend furn_t null_furniture_t();
        friend ter_t null_terrain_t();

        translation name_;


        std::vector<iexamine_functions> examine_func;


        std::vector<cata::clone_ptr<iexamine_actor>> examine_actor;

    private:
        std::set<std::string> flags;
        enum_bitset<ter_furn_flag> bitflags;

    public:
        ter_str_id curtain_transform;

        bool has_curtains() const {
            return !( curtain_transform.is_empty() || curtain_transform.is_null() );
        }

        bool has_disassembly() const;

        std::string name() const;







        std::array<int, NUM_SEASONS> symbol_;

        bool can_examine( const tripoint_bub_ms &examp ) const;

        bool has_examine( iexamine_examine_function func ) const;

        bool has_examine( const std::string &action ) const;
        void set_examine( const iexamine_functions &func );
        std::vector<cata::clone_ptr<iexamine_actor>> get_examine() const;
        void examine( Character &, const tripoint_bub_ms & ) const;

        int light_emitted = 0;
        light_color_rgb light_color{};

        int movecost = 0;
        int heat_radiation = 0;

        int coverage = 0;

        units::temperature_delta floor_bedding_warmth = 0_C_delta;
        int comfort = 0;

        int fall_damage_reduction = 0;

        units::volume max_volume = DEFAULT_TILE_VOLUME;

        itype_id liquid_source_item_id = itype_id::NULL_ID();
        double liquid_source_min_temp = 4;
        std::pair<int, int> liquid_source_count = { 0, 0 };

        translation description;


        std::array<nc_color, NUM_SEASONS> color_;




        void load_symbol_color( const JsonObject &jo, const std::string &context );

        std::string looks_like;





        std::array<harvest_id, NUM_SEASONS> harvest_by_season = {{
                harvest_id::NULL_ID(), harvest_id::NULL_ID(), harvest_id::NULL_ID(), harvest_id::NULL_ID()
            }
        };



        itype_id base_item = itype_id::NULL_ID();

        bool transparent = false;

        const std::set<std::string> &get_flags() const {
            return flags;
        }

        bool has_flag( const std::string &flag ) const {
            return flags.count( flag ) > 0;
        }

        bool has_flag( const ter_furn_flag flag ) const {
            return bitflags[flag];
        }

        void set_flag( const std::string &flag );
        void set_flag( ter_furn_flag flag );
        void unset_flag( const std::string &flag );
        void unset_flags();


        std::bitset<NUM_TERCONN> connect_groups;

        std::bitset<NUM_TERCONN> connect_to_groups;

        std::bitset<NUM_TERCONN> rotate_to_groups;


        void set_connect_groups( const std::vector<std::string> &connect_groups_vec );
        void unset_connect_groups( const std::vector<std::string> &connect_groups_vec );

        void set_connects_to( const std::vector<std::string> &connect_groups_vec );
        void unset_connects_to( const std::vector<std::string> &connect_groups_vec );

        void set_rotates_to( const std::vector<std::string> &connect_groups_vec );
        void unset_rotates_to( const std::vector<std::string> &connect_groups_vec );


        std::vector<item_comp> get_uncraft_components() const;


        void set_groups( std::bitset<NUM_TERCONN> &bits, const std::vector<std::string> &connect_groups_vec,
                         bool unset = false );

        bool in_connect_groups( const std::bitset<NUM_TERCONN> &test_connect_group ) const {
            return ( connect_groups & test_connect_group ).any();
        }

        int symbol() const;
        nc_color color() const;

        const harvest_id &get_harvest() const;




        const std::set<std::string> &get_harvest_names() const;

        virtual std::vector<std::string> extended_description() const;

        bool was_loaded = false;
        virtual bool is_smashable() const {
            return false;
        }

        bool is_flammable() const {
            return has_flag( ter_furn_flag::TFLAG_FLAMMABLE ) ||
                   has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ||
                   has_flag( ter_furn_flag::TFLAG_FLAMMABLE_HARD );
        }

        virtual bool is_terrain() const;

        virtual void load( const JsonObject &jo, const std::string & );
        virtual void check() const {};
};





struct ter_t : map_data_common_t {

    std::vector<std::pair<ter_str_id, mod_id>> src;

    ter_str_id id;
    ter_str_id open;
    ter_str_id close;

    std::optional<map_ter_bash_info> bash;
    std::optional<map_ter_deconstruct_info> deconstruct;




    std::vector<ter_str_id> phase_targets;


    std::vector<units::temperature> phase_temps;


    std::string phase_method;

    ter_str_id lockpick_result;

    cata::value_ptr<activity_data_ter> boltcut;
    cata::value_ptr<activity_data_ter> hacksaw;
    cata::value_ptr<activity_data_ter> oxytorch;
    cata::value_ptr<activity_data_ter> prying;

    std::string trap_id_str;
    ter_str_id transforms_into;
    ter_str_id roof;

    trap_id trap;

    std::set<itype_id> allowed_template_id;

    ter_t();

    std::optional<map_common_bash_info> bash_info() const override {
        return bash;
    }
    std::optional<map_common_deconstruct_info> deconstruct_info() const override {
        return deconstruct;
    }

    static size_t count();

    bool is_null() const;
    bool is_terrain() const override;

    std::vector<std::string> extended_description() const override;
    bool is_smashable() const override;

    void load( const JsonObject &jo, const std::string &src ) override;
    void check() const override;
};

void set_ter_ids();
void set_furn_ids();
void reset_furn_ter();





struct furn_t : map_data_common_t {

    std::vector<std::pair<furn_str_id, mod_id>> src;

    furn_str_id id;
    furn_str_id open;
    furn_str_id close;
    furn_str_id lockpick_result;
    std::optional<map_furn_bash_info> bash;
    std::optional<map_furn_deconstruct_info> deconstruct;
    itype_id crafting_pseudo_item;
    units::volume keg_capacity = 0_ml;

    units::temperature_delta bonus_fire_warmth_feet = 0.6_C_delta;
    itype_id deployed_item;

    int move_str_req = 0;
    units::mass mass = 0_gram;

    cata::value_ptr<activity_data_furn> boltcut;
    cata::value_ptr<activity_data_furn> hacksaw;
    cata::value_ptr<activity_data_furn> oxytorch;
    cata::value_ptr<activity_data_furn> prying;

    cata::value_ptr<furn_workbench_info> workbench;

    cata::value_ptr<plant_data> plant;

    cata::value_ptr<float> surgery_skill_multiplier;


    const itype *crafting_pseudo_item_type() const;

    std::vector<const itype *> crafting_ammo_item_types() const;

    furn_t();

    std::optional<map_common_bash_info> bash_info() const override {
        return bash;
    }
    std::optional<map_common_deconstruct_info> deconstruct_info() const override {
        return deconstruct;
    }
    static size_t count();

    bool is_movable() const;

    std::vector<std::string> extended_description() const override;
    bool is_smashable() const override;

    void load( const JsonObject &jo, const std::string &src ) override;
    void check() const override;
};


struct ter_furn_id {
    std::variant<ter_id, furn_id> ter_furn;
    void deserialize( const JsonValue &jo );
    ter_furn_id();
    explicit ter_furn_id( const std::string &name );
    bool operator==( const ter_furn_id &rhs ) const {
        return ter_furn == rhs.ter_furn;
    }
    bool resolve( const std::string &name );
};

void load_furniture( const JsonObject &jo, const std::string &src );
void finalize_furniture();
void load_terrain( const JsonObject &jo, const std::string &src );
void finalize_terrain();

class ter_furn_migrations
{
    public:

        static void load( const JsonObject &jo );


        static void reset();


        static void check();
};

class trap_migrations
{
    public:

        static void load( const JsonObject &jo );


        static void reset();


        static void check();
};

class field_type_migrations
{
    public:

        static void load( const JsonObject &jo );


        static void reset();


        static void check();
};










// NOLINTNEXTLINE(cata-static-int_id-constants)
extern ter_id t_null;





void check_furniture_and_terrain();

#endif
