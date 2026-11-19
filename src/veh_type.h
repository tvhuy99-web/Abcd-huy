#pragma once
#ifndef CATA_SRC_VEH_TYPE_H
#define CATA_SRC_VEH_TYPE_H

#include <array>
#include <bitset>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "lightmap.h"
#include "coordinates.h"
#include "memory_fast.h"
#include "point.h"
#include "requirements.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"

class Character;
class JsonObject;
class JsonOut;
class JsonValue;
class vehicle;
class vpart_info;
struct vehicle_prototype;
template <typename T> class generic_factory;

namespace vehicles
{


constexpr char variant_separator = '#';

void load_prototype( const JsonObject &jo, const std::string &src );
void reset_prototypes();
void finalize_prototypes();
const std::vector<vehicle_prototype> &get_all_prototypes();

namespace parts
{
void load( const JsonObject &jo, const std::string &src );
void check();
void reset();
void finalize();
const std::vector<vpart_info> &get_all();
}
}



enum vpart_bitflags : int {
    VPFLAG_ARMOR,
    VPFLAG_APPLIANCE,
    VPFLAG_ARCADE,
    VPFLAG_EVENTURN,
    VPFLAG_ODDTURN,
    VPFLAG_CONE_LIGHT,
    VPFLAG_WIDE_CONE_LIGHT,
    VPFLAG_HALF_CIRCLE_LIGHT,
    VPFLAG_CIRCLE_LIGHT,
    VPFLAG_BOARDABLE,
    VPFLAG_AISLE,
    VPFLAG_MWS,
    VPFLAG_ADVANCED_MWS,
    VPFLAG_CONTROLS,
    VPFLAG_OBSTACLE,
    VPFLAG_OPAQUE,
    VPFLAG_OPENABLE,
    VPFLAG_SEATBELT,
    VPFLAG_SIMPLE_PART,
    VPFLAG_SPACE_HEATER,
    VPFLAG_HEATED_TANK,
    VPFLAG_COOLER,
    VPFLAG_WALL_MOUNTED,
    VPFLAG_WHEEL,
    VPFLAG_ROTOR,
    VPFLAG_MOUNTABLE,
    VPFLAG_FLOATS,
    VPFLAG_NO_LEAK,
    VPFLAG_DOME_LIGHT,
    VPFLAG_AISLE_LIGHT,
    VPFLAG_ATOMIC_LIGHT,
    VPFLAG_ALTERNATOR,
    VPFLAG_ENGINE,
    VPFLAG_FRIDGE,
    VPFLAG_FREEZER,
    VPFLAG_LIGHT,
    VPFLAG_WINDOW,
    VPFLAG_CURTAIN,
    VPFLAG_CARGO,
    VPFLAG_CARGO_PASSABLE,
    VPFLAG_INTERNAL,
    VPFLAG_SOLAR_PANEL,
    VPFLAG_WATER_WHEEL,
    VPFLAG_WIND_TURBINE,
    VPFLAG_RECHARGE,
    VPFLAG_EXTENDS_VISION,
    VPFLAG_ENABLED_DRAINS_EPOWER,
    VPFLAG_AUTOCLAVE,
    VPFLAG_WASHING_MACHINE,
    VPFLAG_DISHWASHER,
    VPFLAG_FLUIDTANK,
    VPFLAG_REACTOR,
    VPFLAG_RAIL,
    VPFLAG_TURRET_CONTROLS,
    VPFLAG_ROOF,
    VPFLAG_CABLE_PORTS,
    VPFLAG_BATTERY,
    VPFLAG_POWER_TRANSFER,
    VPFLAG_HUGE_OK,
    VPFLAG_NEED_LEG,
    VPFLAG_IGNORE_LEG_REQUIREMENT,
    VPFLAG_INOPERABLE_SMALL,
    VPFLAG_IGNORE_HEIGHT_REQUIREMENT,
    VPFLAG_NL_BOILER,

    NUM_VPFLAGS
};

struct vpslot_engine {
    bool was_loaded = false;

    float backfire_threshold = 0.0f;
    int backfire_freq = 1;
    int muscle_power_factor = 0;
    float damaged_power_factor = 0.0f;
    int noise_factor = 0;
    int m2c = 100;
    std::vector<std::string> exclusions;
    std::vector<itype_id> fuel_opts;

    void deserialize( const JsonObject &jo );
};

struct veh_ter_mod {
    std::string terrain_flag;
    int move_override;
    int move_penalty;

    void deserialize( const JsonValue &jv );

    bool operator==( const veh_ter_mod &rhs ) const {
        return terrain_flag == rhs.terrain_flag;
    }
};

struct vpslot_wheel {
    bool was_loaded = false;

    float rolling_resistance = 1.0f;
    int contact_area = 1;
    std::vector<veh_ter_mod> terrain_modifiers;
    float offroad_rating = 0.5f;

    void deserialize( const JsonObject &jo );
};

struct vpslot_rotor {
    bool was_loaded = false;

    int rotor_diameter = 1;

    void deserialize( const JsonObject &jo );
};

struct vpslot_workbench {

    float multiplier = 1.0f;

    units::mass allowed_mass = 0_gram;
    units::volume allowed_volume = 0_ml;

    void deserialize( const JsonObject &jo );
};

struct vpslot_toolkit {
    bool was_loaded = false;

    std::set<itype_id> allowed_types;

    void deserialize( const JsonObject &jo );
};

struct vpslot_terrain_transform {
    std::set<std::string> pre_flags;
    std::optional<ter_str_id> post_terrain;
    std::optional<furn_str_id> post_furniture;
    std::optional<field_type_str_id> post_field;

    int post_field_intensity;
    time_duration post_field_age;

    void deserialize( const JsonObject &jo );
};

struct vp_control_req {
    std::set<std::pair<skill_id, int>> skills;
    std::set<proficiency_id> proficiencies;

    void deserialize( const JsonObject &jo );
};

class vpart_category
{
    public:
        static const std::vector<vpart_category> &all();

        static void load_all( const JsonObject &jo );
        static void finalize_all();
        void load( const JsonObject &jo );
        static void reset();

        std::string get_id() const {
            return id_;
        }

        std::string name() const {
            return name_.translated();
        }

        std::string short_name() const {
            return short_name_.translated();
        }

        bool operator < ( const vpart_category &cat ) const {
            return priority_ < cat.priority_;
        }

    private:
        std::string id_;
        translation name_;
        translation short_name_;
        int priority_ = 0;
};

class vpart_variant
{
    public:
        std::string id;
        std::array<char32_t, 8> symbols;
        std::array<char32_t, 8> symbols_broken;

        std::string get_label() const;





        char32_t get_symbol( units::angle direction, bool is_broken ) const;





        int get_symbol_curses( units::angle direction, bool is_broken ) const;


        static int get_symbol_curses( char32_t sym );

    private:
        std::string label_;
        friend class vpart_info;
        void load( const JsonObject &jo, bool was_loaded );
};

class vpart_info
{
    public:
        vpart_id id;

        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        void finalize();
        void handle_inheritance( const vpart_info &copy_from,
                                 const std::unordered_map<std::string, vpart_info> &abstracts );


        std::string name() const;

        const std::set<std::string> &get_flags() const {
            return flags;
        }
        bool has_flag( const std::string &flag ) const {
            return flags.count( flag ) != 0;
        }
        bool has_flag( const vpart_bitflags flag ) const {
            return bitflags.test( flag );
        }
        void set_flag( const std::string &flag );


        const std::set<std::string> &get_categories() const;


        bool has_category( const std::string &category ) const;


        int format_description( std::string &msg, const nc_color &format_color, int width ) const;


        requirement_data install_requirements() const;


        void set_install_requirements( const std::vector<std::pair<requirement_id, int>> &reqs );


        time_duration install_time( const Character &you ) const;


        requirement_data removal_requirements() const;


        time_duration removal_time( const Character &you ) const;


        requirement_data repair_requirements() const;


        bool is_repairable() const;


        time_duration repair_time( const Character &you ) const;

        std::optional<vpslot_workbench> workbench_info;
        std::optional<vpslot_toolkit> toolkit_info;
        std::optional<vpslot_engine> engine_info;
        std::optional<vpslot_wheel> wheel_info;
        std::optional<vpslot_rotor> rotor_info;
        std::optional<vpslot_terrain_transform> transform_terrain_info;

        std::vector<enchantment_id> enchantments;
        std::optional<effect_on_condition_id> activatable_eoc;

        std::set<std::pair<itype_id, int>> get_pseudo_tools() const;


        std::vector<itype_id> get_folding_tools() const;

        std::vector<itype_id> get_unfolding_tools() const;

        time_duration get_folding_time() const;

        time_duration get_unfolding_time() const;


        bool has_control_req() const;

    private:
        std::set<std::string> flags;

        std::set<std::string> categories;

        std::bitset<NUM_VPFLAGS> bitflags;


        std::vector<std::pair<requirement_id, int>> install_reqs;
        std::vector<std::pair<requirement_id, int>> removal_reqs;
        std::vector<std::pair<requirement_id, int>> repair_reqs;


        std::set<std::pair<itype_id, int>> pseudo_tools;

        std::vector<itype_id> folding_tools;

        std::vector<itype_id> unfolding_tools;

        time_duration folding_time = 10_seconds;

        time_duration unfolding_time = 10_seconds;


        translation name_;

    public:
        std::map<std::string, vpart_variant> variants;

        std::string variant_default;

        std::vector<std::pair<std::string, std::string>> variants_bases;


        std::map<skill_id, int> install_skills;
        std::map<skill_id, int> repair_skills;
        std::map<skill_id, int> removal_skills;


        vp_control_req control_air;
        vp_control_req control_land;


        item_group_id breaks_into_group = item_group_id( "EMPTY_GROUP" );


        std::unordered_map<damage_type_id, float> damage_reduction;


        std::map<quality_id, int> qualities;


        std::set<emit_id> emissions;







        std::set<emit_id> exhaust;


        nc_color color = c_light_gray;
        nc_color color_broken = c_light_gray;


        itype_id fuel_type = itype_id::NULL_ID();


        itype_id default_ammo = itype_id::NULL_ID();


        std::optional<units::volume> folded_volume = std::nullopt;


        units::volume size = 0_ml;


        std::string looks_like;


        translation description;


        itype_id base_item;


        std::optional<itype_id> removed_item;


        vpart_location_id location = vpart_location_id::NULL_ID();


        int durability = 0;


        int dmg_mod = 100;





        units::power epower = 0_W;





        units::power energy_consumption = 0_W;





        units::power power = 0_W;


        time_duration install_moves = 1_hours;

        time_duration repair_moves = 1_hours;

        time_duration removal_moves = -1_seconds;







        int bonus = 0;
        light_color_rgb light_color{};


        int cargo_weight_modifier = 100;


        int comfort = 0;
        units::temperature_delta floor_bedding_warmth = 0_C_delta;
        units::temperature_delta bonus_fire_warmth_feet = 0.6_C_delta;

    private:
        bool was_loaded = false;
        std::vector<std::pair<vpart_id, mod_id>> src;
        friend class generic_factory<vpart_info>;
        friend struct mod_tracker;
};

struct vehicle_item_spawn {
    point_rel_ms pos;
    int chance = 0;

    int with_ammo = 0;

    int with_magazine = 0;

    std::vector<std::pair<itype_id, std::string>> item_ids;
    std::vector<item_group_id> item_groups;

    void deserialize( const JsonObject &jo );
};





struct vehicle_prototype {
    public:
        struct part_def {
            point_rel_ms pos;
            vpart_id part;
            std::string variant;
            int with_ammo = 0;
            std::set<itype_id> ammo_types;
            std::pair<int, int> ammo_qty = { -1, -1 };
            itype_id fuel = itype_id::NULL_ID();
            std::vector<itype_id> tools;

            bool operator==( const part_def &other ) const {
                return pos == other.pos && part == other.part && variant == other.variant &&
                       with_ammo == other.with_ammo && ammo_types == other.ammo_types && ammo_qty == other.ammo_qty &&
                       fuel == other.fuel && tools == other.tools;
            }
        };

        struct zone_def {
            zone_type_id zone_type;
            std::string name;
            std::string filter;
            point_rel_ms pt;

            void deserialize( const JsonObject &jo );
            bool operator==( const zone_def &other ) const {
                return zone_type == other.zone_type && name == other.name && filter == other.name && pt == other.pt;
            }
        };

        vproto_id id;
        translation name;
        std::vector<part_def> parts;
        std::vector<vehicle_item_spawn> item_spawns;
        std::vector<zone_def> zone_defs;
        std::vector<std::pair<vproto_id, mod_id>> src;

        shared_ptr_fast<vehicle> blueprint;

        void load( const JsonObject &jo, std::string_view src );
        static void save_vehicle_as_prototype( const vehicle &veh, JsonOut &json );
    private:
        bool was_loaded = false;
        friend class generic_factory<vehicle_prototype>;
        friend struct mod_tracker;
};





class vpart_migration
{
    public:
        vpart_id part_id_old;
        vpart_id part_id_new;
        std::optional<std::string> variant;
        std::vector<itype_id> add_veh_tools;


        static void load( const JsonObject &jo );


        static void reset();


        static void finalize();


        static void check();


        static const vpart_migration *find_migration( const vpart_id &original );
};

#endif
