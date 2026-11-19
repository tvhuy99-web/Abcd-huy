#pragma once
#ifndef CATA_SRC_VEHICLE_H
#define CATA_SRC_VEHICLE_H

#include <array>
#include <climits>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "calendar.h"
#include "cata_bitset.h"
#include "cata_utility.h"
#include "character_id.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "global_vars.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_stack.h"
#include "item_stack_use_charges.h" // IWYU pragma: keep
#include "line.h"
#include "magic_enchantment.h"
#include "map.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "point.h"
#include "ret_val.h"
#include "safe_reference.h"
#include "talker.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "vpart_position.h"
#include "vpart_range.h"

// IWYU pragma: no_forward_declare npc // behind unique_ptr
class Character;
class Creature;
class effect_source;
class JsonArray;
class JsonObject;
class JsonOut;
class monster;
class veh_menu;
class vehicle;
class vehicle_cursor;
class vpart_info;
class vpart_variant;
enum class ter_furn_flag : int;
enum vpart_bitflags : int;
struct itype;
struct vehicle_part;
template <typename E> struct enum_traits;

void handbrake( map &here );

void practice_athletic_proficiency( Character &p );
void practice_pilot_proficiencies( Character &p, bool &boating );

namespace catacurses
{
class window;
}

namespace vehicles
{

constexpr double rolling_constant_to_variable = 33.33;
constexpr float vmiph_per_tile = 400.0f;

constexpr units::angle steer_increment = 15_degrees;
}

struct rider_data {
    Creature *psg = nullptr;
    int prt = -1;
    bool moved = false;
};

float get_collision_factor( float delta_v );


constexpr int SCATTER_DISTANCE = 3;

constexpr int k_mvel = 200;

enum class part_status_flag : int {
    any = 0,
    working = 1 << 0,
    available = 1 << 1,
    enabled = 1 << 2
};

template<>
struct enum_traits<part_status_flag> {
    static constexpr bool is_flag_enum = true;
};

enum veh_coll_type : int {
    veh_coll_nothing,
    veh_coll_body,
    veh_coll_veh,
    veh_coll_bashable,
    veh_coll_other,
    num_veh_coll_types
};

struct smart_controller_cache {
    time_point created = calendar::turn;
    time_point gas_engine_last_turned_on = calendar::start_of_cataclysm;
    bool gas_engine_shutdown_forbidden = false;
    int velocity = 0;
    int battery_percent = 0;
    units::power battery_net_charge_rate = 0_W;
    float load = 0.0f;
};

struct smart_controller_config {
    int battery_lo = 25;
    int battery_hi = 90;

    void deserialize( const JsonObject &data );
    void serialize( JsonOut &json ) const;
};

struct veh_collision {

    int part  = 0;
    veh_coll_type type = veh_coll_nothing;

    int  imp = 0;

    void *target  = nullptr;

    int target_part = 0;
    std::string target_name;

    veh_collision() = default;
};

struct vpart_edge_info {
    int forward;
    int back;
    int left;
    int right;
    bool left_edge = false;
    bool right_edge = false;
    vpart_edge_info() : forward( -1 ), back( -1 ), left( -1 ), right( -1 ) {}
    vpart_edge_info( int forward, int back, int left, int right, bool left_edge, bool right_edge ) :
        forward( forward ), back( back ), left( left ), right( right ), left_edge( left_edge ),
        right_edge( right_edge ) {}

    bool is_edge_mount() const {
        return left_edge || right_edge || left == -1 || right == -1;
    }
    bool is_left_edge() const {
        return left_edge || left == -1;
    }
};

class vehicle_stack : public item_stack
{
    private:
        vehicle &veh;
        vehicle_part &vp;
    public:
        vehicle_stack( vehicle &veh, vehicle_part &vp );
        item_stack::iterator erase( item_stack::const_iterator it ) override;
        void insert( map &here, const item &newitem ) override;
        int count_limit() const override;
        units::volume max_volume() const override;
        units::volume stored_volume() const override;
};

enum towing_point_side : int {
    TOW_FRONT,
    TOW_SIDE,
    TOW_BACK,
    NUM_TOW_TYPES
};

class towing_data
{
    private:
        vehicle *towing;
        vehicle *towed_by;
    public:
        explicit towing_data( vehicle *towed_veh = nullptr, vehicle *tower_veh = nullptr ) :
            towing( towed_veh ), towed_by( tower_veh ) {}
        vehicle *get_towed_by() const {
            return towed_by;
        }
        bool set_towing( vehicle *tower_veh, vehicle *towed_veh );
        vehicle *get_towed() const {
            return towing;
        }
        void clear_towing() {
            towing = nullptr;
            towed_by = nullptr;
        }
        towing_point_side tow_direction = NUM_TOW_TYPES;

        tripoint_bub_ms other_towing_point;
};

struct bounding_box {
    point_rel_ms p1;
    point_rel_ms p2;
};

int mps_to_vmiph( double mps );
double vmiph_to_mps( int vmiph );
int cmps_to_vmiph( int cmps );
int vmiph_to_cmps( int vmiph );
constexpr float accel_g = 9.81f;

enum class vp_flag : uint32_t {
    none = 0,
    passenger_flag = 1,
    animal_flag = 2,
    carried_flag = 4,
    carrying_flag = 8,
    tracked_flag = 16,
    linked_flag = 32,
    unsalvageable_flag = 64
};

class turret_cpu
{
        friend vehicle_part;
    private:
        std::unique_ptr<npc> brain;
    public:
        turret_cpu() = default;
        turret_cpu( const turret_cpu & );
        turret_cpu &operator=( const turret_cpu & );
        ~turret_cpu();
};




struct vehicle_part {
        friend vehicle;
        friend class veh_interact;
        friend class vehicle_activity_actor;
        friend class vehicle_cursor;
        friend class vehicle_stack;
        friend item_location;
        friend class turret_data;


        vehicle_part();

        vehicle_part( const vpart_id &type, item &&base );

        vehicle_part( const vpart_id &type, item &&base, std::vector<item> &installed_with );


        const item &get_base() const;

        std::vector<item> get_salvageable() const;

        void set_base( item &&new_base );

        bool has_flag( const vp_flag flag ) const noexcept {
            const uint32_t flag_as_uint32 = static_cast<uint32_t>( flag );
            return ( flags & flag_as_uint32 ) == flag_as_uint32;
        }
        void set_flag( const vp_flag flag ) noexcept {
            flags |= static_cast<uint32_t>( flag );
        }
        void remove_flag( const vp_flag flag ) noexcept {
            flags &= ~static_cast<uint32_t>( flag );
        }





        std::string name( bool with_prefix = true ) const;

        struct carried_part_data {
            tripoint_rel_ms mount;
            units::angle face_dir;
            std::string veh_name;

            void deserialize( const JsonObject &data );
            void serialize( JsonOut &json ) const;
        };


        std::stack<carried_part_data> carried_stack;


        itype_id ammo_current() const;


        int ammo_capacity( const ammotype &ammo ) const;


        int item_capacity( const itype_id &stuffing_id ) const;


        int ammo_remaining() const;
        int remaining_ammo_capacity() const;


        itype_id fuel_current() const;

        bool fuel_set( const itype_id &fuel );






        int ammo_set( const itype_id &ammo, int qty = -1 );


        void ammo_unset();







        int ammo_consume( int qty, map *here, const tripoint_bub_ms &pos );







        units::energy consume_energy( const vehicle_cursor &cur, const itype_id &ftype,
                                      units::energy wanted_energy );


        bool can_reload( const item &obj = item() ) const;


        turret_cpu cpu; //NOLINT(cata-serialize)






        void process_contents( map &here, const tripoint_bub_ms &pos, bool e_heater );





        bool fill_with( item &liquid, int qty = INT_MAX );


        const std::set<fault_id> &faults() const;


        bool has_fault_flag( const std::string &searched_flag ) const;

        bool has_fault( const fault_id &fault ) const;


        std::set<fault_id> faults_potential() const;


        bool fault_set( const fault_id &f );

        void load_furniture( map &here, const tripoint_bub_ms &from );
        void unload_furniture( map &here, const tripoint_bub_ms &to );






        npc *crew() const;




        bool set_crew( const npc &who );


        void unset_crew();


        void reset_target( const tripoint_abs_ms &pos );










        bool is_engine() const;


        bool is_light() const;




        bool is_fuel_store( bool skip_broke = true ) const;


        bool is_tank() const;


        bool contains_liquid() const;


        bool is_battery() const;


        bool is_reactor() const;





        bool is_leaking() const;


        bool is_turret() const;


        bool is_seat() const;

        bool is_wheel() const;


        std::string carried_name() const;


    public:

        point_rel_ms mount;


        std::array<tripoint_rel_ms, 2> precalc = { { tripoint_rel_ms( -1, -1, 0 ), tripoint_rel_ms( -1, -1, 0 ) } };


        tripoint_abs_ms next_pos = tripoint_abs_ms( -1, -1, 0 ); // NOLINT(cata-serialize)


        int hp() const;


        int damage() const;

        int degradation() const;

        int max_damage() const;

        int damage_level() const;

        bool is_repairable() const;


        double damage_percent() const;

        double health_percent() const;


        double floating_leak_threshold() const;


        bool is_broken() const;


        bool is_cleaner_on() const;


        bool is_unavailable( bool carried = true ) const;

        bool is_available( bool carried = true ) const;





        npc &get_targeting_npc( vehicle &veh );



        int contact_area() const;


        float rolling_resistance() const;


        int move_penalty() const;


        int blood = 0;





        bool inside = false; // NOLINT(cata-serialize)





        bool removed = false; // NOLINT(cata-serialize)
        bool enabled = true;

        bool power_disabled = false; // NOLINT(cata-serialize)


        character_id passenger_id;


        bool open = false;


        bool locked = false;


        bool hidden = false;


        units::angle direction = 0_degrees;





        std::pair<tripoint_abs_ms, tripoint_abs_ms> target = { tripoint_abs_ms::invalid, tripoint_abs_ms::invalid };


        std::string variant;

        time_point last_disconnected = calendar::before_time_starts;

        time_point last_charged = calendar::turn;

    private:



        const vpart_info *info_; // NOLINT(cata-serialize)

        uint32_t flags = 0;

        item base;

        std::vector<item> tools;

        cata::colony<item> items;

        std::vector<item> salvageable;


        itype_id ammo_pref = itype_id::NULL_ID();





        character_id crew_id;

    public:

        const vpart_info &info() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );




        item_group::ItemList pieces_for_broken_part() const;


        bool is_fake = false; // NOLINT(cata-serialize)
        bool is_active_fake = false; // NOLINT(cata-serialize)
        int fake_part_to = -1; // NOLINT(cata-serialize)
        int fake_protrusion_on = -1; // NOLINT(cata-serialize)
        bool has_fake = false; // NOLINT(cata-serialize)
        int fake_part_at = -1; // NOLINT(cata-serialize)

        bool is_real_or_active_fake() const {
            return !is_fake || is_active_fake;
        }
};



struct multimag_pocket_state {
    enum class source_kind { BATTERY, TANK };
    int initial_qty = 0;
    source_kind kind = source_kind::BATTERY;
    int vpart_index = -1;
};




struct multimag_display_pocket {
    std::string pocket_id;
    itype_id ammo_itype;
    int local_qty = 0;
    int effective_qty = 0;
    int per_use_qty = 0;
    bool vehicle_bound = false;
};

class turret_data
{
        friend vehicle;

    public:
        turret_data() = default;
        turret_data( const turret_data & ) = delete;
        turret_data &operator=( const turret_data & ) = delete;
        turret_data( turret_data && ) = default;
        turret_data &operator=( turret_data && ) = default;


        explicit operator bool() const {
            return veh && part;
        }

        std::string name() const;


        item_location base();
        item_location base() const;


        bool uses_vehicle_tanks_or_batteries() const;


        int ammo_remaining() const;


        int ammo_capacity( const ammotype &ammo ) const;


        const itype *ammo_data() const;


        itype_id ammo_current() const;


        std::set<itype_id> ammo_options() const;


        bool ammo_select( const itype_id &ammo );


        std::set<ammo_effect_str_id> ammo_effects() const;


        int range() const;





        bool in_range( const tripoint_abs_ms &target ) const;







        void prepare_fire( Character &you );






        void post_fire( map *here, Character &you, int shots );







        int fire( Character &c, map *here, const tripoint_bub_ms &target );

        bool can_reload() const;
        bool can_unload() const;

        std::vector<multimag_display_pocket> multimag_display_state() const;

        enum class status : int {
            invalid,
            no_ammo,
            no_power,
            ready
        };

        status query() const;

    private:
        turret_data( vehicle *veh, vehicle_part *part )
            : veh( veh ), part( part ) {}
        double cached_recoil = 0;



        std::map<std::string, multimag_pocket_state> mm_bindings_;

    protected:
        vehicle *veh = nullptr;
        vehicle_part *part = nullptr;
};





struct label : public point_rel_ms {
    label() = default;
    explicit label( const point_rel_ms &p ) : point_rel_ms( p ) {}
    label( const point_rel_ms &p, std::string text ) : point_rel_ms( p ), text( std::move( text ) ) {}

    std::string text;

    void deserialize( const JsonObject &data );
    void serialize( JsonOut &json ) const;
};

enum class autodrive_result : int {


    ok,


    abort,

    finished
};

class RemovePartHandler;

class vpart_display
{
    public:
        vpart_display();
        explicit vpart_display( const vehicle_part &vp );

        const vpart_id &id;
        const vpart_variant &variant;
        nc_color color = c_black;
        std::string carried_furn;
        char32_t symbol = ' ';
        int symbol_curses = ' ';
        bool is_broken = false;
        bool is_open = false;
        bool has_cargo = false;

        std::string get_tileset_id() const;
};









































































class vehicle
{
    private:


        global_variables::impl_t values;
        bool has_structural_part( const point &dp ) const;
        bool has_structural_part( const point_rel_ms &dp ) const;
        bool is_structural_part_removed() const;
        void open_or_close( map &here, int part_index, bool opening );
        void lock_or_unlock( int part_index, bool locking );
        bool is_connected( const vehicle_part &to, const vehicle_part &from,
                           const vehicle_part &excluded ) const;

    public:


        int damage_direct( map &here, vehicle_part &vp, int dmg,
                           const damage_type_id &type = damage_type_id( "pure" ),
                           bool interrupt_autodrive = true );
    private:

        int break_off( map &here, vehicle_part &vp, int dmg );

        bool explode_fuel( map &here, vehicle_part &vp, const damage_type_id &type );

        void smash_security_system( map &here );

        units::power part_vpower_w( map &here, const vehicle_part &vp, bool at_full_hp = false ) const;


        units::power part_epower( const vehicle_part &vp ) const;


        int power_to_energy_bat( units::power power, const time_duration &d ) const;


        bool do_environmental_effects( map &here ) const;



        std::string print_fuel_indicator( map &here, const itype_id &fuel_type,
                                          std::map<itype_id, units::energy> fuel_usages );



        void print_fuel_indicator( map &here, const catacurses::window &w, const point &p,
                                   const itype_id &fuel_type,
                                   bool verbose = false, bool desc = false );

        void print_fuel_indicator( map &here, const catacurses::window &w, const point &p,
                                   const itype_id &fuel_type,
                                   std::map<itype_id, units::energy> fuel_usages,
                                   bool verbose = false, bool desc = false );


        time_duration engine_start_time( map &here, const vehicle_part &vp ) const;


        double engine_cold_factor( map &here, const vehicle_part &vp ) const;


        void refresh_pivot( map &here ) const;

        void calc_mass_center( map &here, bool precalc ) const;


        void leak_fuel( map &here,  vehicle_part &pt ) const;






        template<typename Vehicle>
        static std::map<Vehicle *, float> search_connected_vehicles( const map &here, Vehicle *start );
    public:
        std::vector<std::string> chat_topics;
        void set_value( const std::string &key, diag_value value );
        template <typename... Args>
        void set_value( const std::string &key, Args... args ) {
            set_value( key, diag_value{ std::forward<Args>( args )... } );
        }
        void remove_value( const std::string &key );
        diag_value const &get_value( const std::string &key ) const;
        diag_value const *maybe_get_value( const std::string &key ) const;
        void clear_values();
        void add_chat_topic( const std::string &topic );
        int get_passenger_count( bool hostile ) const;
        enchant_cache enchantment_cache; //NOLINT(cata-serialize)
        void recalculate_enchantment_cache();
        std::map<efftype_id, effect> effects;
        void process_effects();
        void remove_effect( const efftype_id &eff_id );
        bool has_effect( const efftype_id &eff_id ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects() const;


        void add_effect( const effect_source &source, const efftype_id &eff_id, const time_duration &dur,
                         bool permanent = false, int intensity = 0 );
        bool has_visible_effect();






        static vehicle *find_vehicle( const map &here, const tripoint_abs_ms &where );


        static vehicle *find_vehicle_using_parts( const map &here,  const tripoint_abs_ms &where );

        std::map<vehicle *, float> search_connected_vehicles( const map &here );

        std::map<const vehicle *, float> search_connected_vehicles( const map &here ) const;

        void get_connected_vehicles( const map &here, std::unordered_set<vehicle *> &dest );





        std::map<vpart_reference, float> search_connected_batteries( map &here );





        explicit vehicle( const vproto_id &proto_id );
        vehicle( const vehicle & ) = delete;
        ~vehicle();
        vehicle &operator=( vehicle && ) = default;

    private:
        vehicle &operator=( const vehicle & ) = default;

    public:

        void suspend_refresh();
        void enable_refresh();

        void refresh( bool remove_fakes = true );


        void refresh_active_item_cache();



        safe_reference<vehicle> get_safe_reference();





        void set_hp( vehicle_part &pt, int qty, bool keep_degradation, int new_degradation = -1 );







        bool mod_hp( vehicle_part &pt, int qty );


        bool player_in_control( const map &here, const Character &p ) const;

        bool player_is_driving_this_veh( map *here ) const;

        bool remote_controlled( const Character &p ) const;


        void init_state( map &placed_on, int init_veh_fuel, veh_spawn_status init_veh_status,
                         bool force_status = false );


        void damage_all_parts( float hp_percent_loss_min = 0.1f, float hp_percent_loss_max = 1.2f,
                               float percent_of_parts_to_affect = 1.0f,
                               point_rel_ms damage_origin = point_rel_ms::zero,
                               float damage_size = 0 );


        void smash( map &m, float hp_percent_loss_min = 0.1f, float hp_percent_loss_max = 1.2f,
                    float percent_of_parts_to_affect = 1.0f, point_rel_ms damage_origin = point_rel_ms::zero,
                    float damage_size = 0 );

        void serialize( JsonOut &json ) const;

        void deserialize( const JsonObject &data );

        void deserialize_parts( const JsonArray &data );

        int print_part_list( const catacurses::window &win, int y1, int max_y, int width,
                             int p,
                             int hl = -1, bool detail = false, bool include_fakes = true ) const;


        void print_vparts_descs( const catacurses::window &win, int max_y, int width, int p,
                                 int &start_at, int &start_limit ) const;

        void invalidate_towing( map &here, bool first_vehicle = false, Character *remover = nullptr );
        void do_towing_move( map &here );
        bool tow_cable_too_far() const;
        bool no_towing_slack() const;
        bool is_towing() const;
        bool has_tow_attached() const;
        int get_tow_part() const;
        bool is_external_part( map *here, const tripoint_bub_ms &part_pt ) const;
        bool is_external_part( const point_rel_ms &mount ) const;
        bool is_towed() const;
        void set_tow_directions();

        bool is_appliance() const;

        bool is_owned_by( const Character &c, bool available_to_take = false ) const;
        bool is_old_owner( const Character &c, bool available_to_take = false ) const;
        std::string get_owner_name() const;
        void set_old_owner( const faction_id &temp_owner ) {
            theft_time = calendar::turn;
            old_owner = temp_owner;
        }
        void remove_old_owner() {
            theft_time = std::nullopt;
            old_owner = faction_id::NULL_ID();
        }
        void set_owner( const faction_id &new_owner ) {
            owner = new_owner;
        }
        void set_owner( const Character &c );
        faction_id get_owner() const {
            return owner;
        }
        faction_id get_old_owner() const {
            return old_owner;
        }
        bool has_owner() const {
            return !owner.is_null();
        }
        bool has_old_owner() const {
            return !old_owner.is_null();
        }
        bool handle_potential_theft( Character const &you, bool check_only = false, bool prompt = true );

        std::set<point_abs_ms> immediate_path( map *here, const units::angle &rotate = 0_degrees,
                                                int requested_distance = 0, bool reverse = false );
        std::set<point_abs_ms> collision_check_points; // NOLINT(cata-serialize)


        bool a11y_brake_alerted = false; // NOLINT(cata-serialize)


        bool a11y_flat_tire_alerted = false; // NOLINT(cata-serialize)

        bool a11y_continuous_drive = false; // NOLINT(cata-serialize)


        bool a11y_manual_drive = false; // NOLINT(cata-serialize)
        bool a11y_obstacle_stop = false; // NOLINT(cata-serialize)

        std::optional<tripoint_abs_omt> a11y_recovery_segment_omt; // NOLINT(cata-serialize)
        bool a11y_pivot_tried = false; // NOLINT(cata-serialize)
        int a11y_pivot_steps_remaining = 0; // NOLINT(cata-serialize)
        bool a11y_route_alternatives_tried = false; // NOLINT(cata-serialize)

        bool a11y_reverse_recovery_active = false; // NOLINT(cata-serialize)
        int a11y_reverse_recovery_attempts = 0; // NOLINT(cata-serialize)
        int a11y_reverse_recovery_ticks = 0; // NOLINT(cata-serialize)
        tripoint_abs_ms a11y_reverse_recovery_start = tripoint_abs_ms::zero; // NOLINT(cata-serialize)


        std::string a11y_stopped_obstacle_name; // NOLINT(cata-serialize)
        units::angle a11y_stopped_heading = 0_degrees; // NOLINT(cata-serialize)
        bool a11y_obstacle_override = false; // NOLINT(cata-serialize)
        std::string a11y_override_obstacle_name; // NOLINT(cata-serialize)
        int a11y_obstacle_resume_speed = 0; // NOLINT(cata-serialize)
        int a11y_drive_stall_ticks = 0; // NOLINT(cata-serialize)
        bool a11y_skid_alerted = false; // NOLINT(cata-serialize)

        bool a11y_drive_paused = false; // NOLINT(cata-serialize)


        bool a11y_release_controls_pending = false; // NOLINT(cata-serialize)
        int a11y_resume_cruise_velocity = 0; // NOLINT(cata-serialize)
        void autopilot_patrol( map *here );
        units::angle get_angle_from_targ( const tripoint_abs_ms &targ ) const;
        void drive_to_local_target( map *here, const tripoint_abs_ms &target, bool follow_protocol );

        autodrive_result do_autodrive( map &here,  Character &driver );

        void stop_autodriving( bool apply_brakes = true );

        void connect( map *here, const tripoint_bub_ms &source_pos, const tripoint_bub_ms &target_pos );

        bool precollision_check( units::angle &angle, map &here, bool follow_protocol );

        bool auto_select_fuel( map &here, vehicle_part &vp );

        bool start_engine( map &here, vehicle_part &vp );

        void stop_engines( map &here );

        void start_engines( map &here, Character *driver = nullptr, bool take_control = false,
                            bool autodrive = false );


        void backfire( map *here, const vehicle_part &vp ) const;






        ret_val<void> can_mount( const point_rel_ms &dp, const vpart_info &vpi ) const;


        ret_val<void> can_unmount( const vehicle_part &vp_to_remove, bool allow_splits = false ) const;



        int install_part( map &here, const point_rel_ms &dp, const vpart_id &type );



        int install_part( map &here, const point_rel_ms &dp, const vpart_id &type, item &&base );

        int install_part( map &here, const point_rel_ms &dp, const vpart_id &type, item &&base,
                          std::vector<item> &installed_with );



        int install_part( map &here, const point_rel_ms &dp, vehicle_part &&vp );

        struct rackable_vehicle {
            std::string name;
            vehicle *veh;
            std::vector<int> racks;
        };

        struct unrackable_vehicle {
            std::string name;
            std::vector<int> racks;
            std::vector<int> parts;
        };



        std::vector<rackable_vehicle> find_vehicles_to_rack( map *here, int rack ) const;



        std::vector<unrackable_vehicle> find_vehicles_to_unrack( int rack,
                bool only_healthy = true ) const;


        bool merge_rackable_vehicle( map *here, vehicle *carry_veh, const std::vector<int> &rack_parts );

        bool merge_vehicle_parts( map *here, vehicle *veh );
        bool merge_appliance_into_grid( map *here, vehicle &veh_target );
        void separate_from_grid( map *here, point_rel_ms mount );

        bool is_powergrid() const;










        bool remove_part( vehicle_part &vp );


        bool remove_part( vehicle_part &vp, RemovePartHandler &handler );

        void part_removal_cleanup( map &here );


        bool do_remove_part_actual( map *here );


        bool remove_carried_vehicle( map &here, const std::vector<int> &carried_parts,
                                     const std::vector<int> &racks );


        bool find_and_split_vehicles( map &here, std::set<int> exclude );

        void relocate_passengers( const std::vector<Character *> &passengers ) const;











        bool split_vehicles( map &here, const std::vector<std::vector <int>> &new_vehs,
                             const std::vector<vehicle *> &new_vehicles,
                             const std::vector<std::vector<point_rel_ms>> &new_mounts,
                             std::vector<vehicle *> *added_vehicles = nullptr );


        item_location part_base( int p );






        std::optional<vpart_reference> get_remote_part( map &here, const vehicle_part &vp_local ) const;



        void remove_remote_part( map &here, const vehicle_part &vp_local ) const;






        vehicle_part_range get_all_parts() const;

        vehicle_part_with_fakes_range get_all_parts_with_fakes( bool with_inactive = false ) const;






        vehicle_part_with_feature_range<std::string> get_avail_parts( std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_avail_parts( vpart_bitflags f ) const;







        vehicle_part_with_feature_range<std::string> get_parts_including_carried(
            std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_parts_including_carried(
            vpart_bitflags f ) const;






        vehicle_part_with_feature_range<std::string> get_any_parts( std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_any_parts( vpart_bitflags f ) const;






        vehicle_part_with_feature_range<std::string> get_enabled_parts( std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_enabled_parts( vpart_bitflags f ) const;



        std::vector<int> parts_at_relative( const point_rel_ms &dp, bool use_cache,
                                            bool include_fake = false ) const;










        int part_with_feature( const point_rel_ms &pt, const std::string &f, bool unbroken,
                               bool include_fake = false ) const;









        int part_with_feature( const point_rel_ms &pt, vpart_bitflags f, bool unbroken,
                               bool include_fake = false ) const;









        int part_with_feature( int p, vpart_bitflags f, bool unbroken, bool include_fake = false ) const;









        int avail_part_with_feature( const point_rel_ms &pt, const std::string &f ) const;









        int avail_part_with_feature( int p, vpart_bitflags f ) const;









        int avail_linkable_part( const point_rel_ms &pt, bool to_ports ) const;







        bool has_part( const std::string &flag, bool enabled = false ) const;







        bool has_part( map *here, const tripoint_bub_ms &pos, const std::string &flag,
                       bool enabled = false ) const;
        bool has_part( const tripoint_abs_ms &pos, const std::string &flag, bool enabled = false ) const;












        tiny_bitset has_parts( const std::vector<std::string> &flags, bool enabled = false ) const;







        std::vector<vehicle_part *> get_parts_at( map *here, const tripoint_bub_ms &pos,
                const std::string &flag,
                part_status_flag condition );
        std::vector<vehicle_part *> get_parts_at( const tripoint_abs_ms &pos,
                const std::string &flag,
                part_status_flag condition );


        bool can_enable( map &here, const vehicle_part &pt, bool alert = false ) const;











        int next_part_to_open( int p, bool outside = false ) const;











        int next_part_to_close( int p, bool outside = false ) const;











        int next_part_to_lock( int p, bool outside = false ) const;











        int next_part_to_unlock( int p, bool outside = false ) const;


        std::vector<int> all_parts_at_location( const vpart_location_id &location ) const;

        int get_next_shifted_index( int original_index, Character &you ) const;


        std::vector<std::vector<int>> find_lines_of_parts( int part, const std::string &flag,
                                   bool only_healthy = true ) const;


        point_rel_ms coord_translate( const point_rel_ms &p ) const;


        void coord_translate( const units::angle &dir, const point_rel_ms &pivot, const point_rel_ms &p,
                              tripoint_rel_ms &q ) const;


        void coord_translate( tileray tdir, const point_rel_ms &pivot, const point_rel_ms &p,
                              tripoint_rel_ms &q ) const;

        tripoint_bub_ms mount_to_tripoint( map *here,  const point_rel_ms &mount ) const;
        tripoint_abs_ms mount_to_tripoint_abs( const point_rel_ms &mount ) const;
        tripoint_bub_ms mount_to_tripoint( map *here, const point_rel_ms &mount,
                                           const point_rel_ms &offset ) const;
        tripoint_abs_ms mount_to_tripoint_abs( const point_rel_ms &mount,
                                               const point_rel_ms &offset ) const;


        int part_at( const point_rel_ms &dp ) const;
        int part_displayed_at( const point_rel_ms &dp, bool include_fake = false,
                               bool below_roof = true, bool roof = true ) const;
        int roof_at_part( int p ) const;



        int index_of_part( const vehicle_part *part, bool include_removed = false ) const;







        vpart_display get_display_of_tile( const point_rel_ms &dp, bool rotate = true,
                                           bool include_fake = true,
                                           bool below_roof = true, bool roof = true ) const;


        std::vector<itype_id> get_printable_fuel_types( map &here ) const;



        std::string print_fuel_indicators( map &here, int start_index = 0 );



        void print_fuel_indicators( map &here,
                                    const catacurses::window &win, const point &, int start_index = 0,
                                    bool fullsize = false, bool verbose = false, bool desc = false,
                                    bool isHorizontal = false );









        void print_speed_gauge( map &here, const catacurses::window &win, const point &,
                                int spacing = 0 ) const;



        void precalc_mounts( int idir, const units::angle &dir, const point_rel_ms &pivot );


        std::vector<int> boarded_parts() const;


        std::vector<rider_data> get_riders() const;


        bool is_passenger( Character &c ) const;

        Character *get_passenger( int you ) const;
        bool has_driver( map &here ) const;

        Character *get_driver( map &here ) const;

        monster *get_monster( const map &here, int p ) const;

        bool enclosed_at( map *here, const tripoint_bub_ms
                          &pos );
        bool enclosed_at( const tripoint_abs_ms &pos );

        tripoint_abs_ms pos_abs() const;

        tripoint_abs_omt pos_abs_omt() const;


        tripoint_bub_ms pos_bub( const map &here ) const;



        tripoint_bub_ms bub_part_pos( const map &here, int index ) const;
        tripoint_bub_ms bub_part_pos( const map &here, const vehicle_part &pt ) const;

        std::set<int> occupied_levels( const map &here ) const;
        tripoint_abs_ms abs_part_pos( int index ) const;
        tripoint_abs_ms abs_part_pos( const vehicle_part &pt ) const;





        std::map<itype_id, int> fuels_left( ) const;




        std::list<item *> fuel_items_left();


        int64_t fuel_left( map &here, const itype_id &ftype,
                           const std::function<bool( const vehicle_part & )> &filter = return_true<const vehicle_part &> )
        const;

        int engine_fuel_left( map &here, const vehicle_part &vp ) const;

        int fuel_capacity( map &here, const itype_id &ftype ) const;



        int drain( map &here, const itype_id &ftype, int amount,
                   const std::function<bool( vehicle_part & )> &filter = return_true< vehicle_part &>,
                   bool apply_loss = true );
        int drain( map *here, const itype_id &ftype, int amount,
                   const std::function<bool( vehicle_part & )> &filter = return_true< vehicle_part &>,
                   bool apply_loss = true );
        int drain( map &here, int index, int amount, bool apply_loss = true );






        units::energy drain_energy( const itype_id &ftype, units::energy wanted_energy );


        units::power basic_consumption( map &here, const itype_id &ftype ) const;

        int consumption_per_hour( const itype_id &ftype, units::energy fuel_per_s ) const;

        void consume_fuel( map &here, int load, bool idling );




        std::map<itype_id, units::power> fuel_usage() const;





        units::power engine_fuel_usage( const vehicle_part &vp ) const;

        std::vector<vehicle_part *> lights();


        static float cone_light_luminance( const std::vector<vehicle_part *> &lights );

        void update_alternator_load( map &here );


        units::power total_engine_epower() const;

        units::power total_alternator_epower( map &here ) const;

        units::power total_solar_epower( map &here ) const;

        units::power total_wind_epower( map &here ) const;

        units::power total_water_wheel_epower( map &here ) const;

        units::power rated_solar_epower() const;

        units::power rated_wind_epower() const;

        units::power rated_water_epower() const;

        units::power total_accessory_epower() const;

        units::power linked_item_epower_this_turn; // NOLINT(cata-serialize)

        units::power recharge_epower_this_turn; // NOLINT(cata-serialize)

        units::power net_battery_charge_rate( map &here, bool include_reactors ) const;


        units::power max_reactor_epower() const;

        units::power active_reactor_epower( map &here ) const;


        void power_parts( map &here );


        std::pair<int, int> battery_power_level( ) const;


        std::pair<int, int> connected_battery_power_level( const map &here ) const;




        bool is_battery_available( map &here ) const;





        int64_t battery_left( map &here, bool apply_loss = true ) const;







        int charge_battery( map &here, int amount, bool apply_loss = true );







        int discharge_battery( map &here, int amount, bool apply_loss = true );




        void invalidate_mass();


        units::mass total_mass( map &here ) const;

        units::mass unloaded_mass() const;


        units::mass weight_on_wheels( map &here ) const;


        const point_rel_ms &rotated_center_of_mass( map &here ) const;

        const point_rel_ms &local_center_of_mass( map &here ) const;



        const point_rel_ms &pivot_point( map &here ) const;




        point_rel_ms pivot_displacement() const;




        units::power total_power( map &here, bool fueled = true, bool safe = false ) const;



        int ground_acceleration( map &here, bool fueled = true, int at_vel_in_vmi = -1 ) const;


        int water_acceleration( map &here, bool fueled = true, int at_vel_in_vmi = -1 ) const;


        int rotor_acceleration( map &here, bool fueled = true, int at_vel_in_vmi = -1 ) const;

        int acceleration( map &here, bool fueled = true, int at_vel_in_vmi = -1 ) const;


        int current_acceleration( map &here, bool fueled = true ) const;


        bool is_moving() const;


        bool can_use_rails( map &here ) const;



        int max_ground_velocity( map &here, bool fueled = true ) const;


        int max_water_velocity( map &here, bool fueled = true ) const;

        int max_rotor_velocity( map &here, bool fueled = true ) const;

        int max_velocity( map &here, bool fueled = true ) const;

        int max_reverse_velocity( map &here, bool fueled = true ) const;



        int safe_ground_velocity( map &here, bool fueled = true ) const;


        int safe_rotor_velocity( map &here, bool fueled = true ) const;


        int safe_water_velocity( map &here, bool fueled = true ) const;

        int safe_velocity( map &here, bool fueled = true ) const;


        void spew_field( map &here,  double joules, int part, field_type_id type, int intensity = 1 ) const;


        void noise_and_smoke( map &here, int load, time_duration time = 1_turns );


        void check_flats_do_rim_damage_or_sounds( map &here );




        int wheel_area() const;

        float average_offroad_rating() const;











        double coeff_air_drag() const;







        double coeff_rolling_drag( map &here ) const;







        double coeff_water_drag( map &here ) const;





        double water_hull_height( map &here ) const;






        double water_draft( map &here ) const;





        bool can_float( map &here ) const;



        bool is_in_water( bool deep_water = false ) const;





        bool is_watercraft() const;



        double lift_thrust_of_rotorcraft( map &here, bool fuelled, bool safe = false ) const;
        bool has_sufficient_rotorlift( map &here ) const;
        int get_z_change() const;
        bool is_flying_in_air() const;
        void set_flying( bool new_flying_value );
        bool is_rotorcraft( map &here ) const;


        bool is_flyable() const;
        void set_flyable( bool val );

        bool would_install_prevent_flyable( const vpart_info &vpinfo, const Character &pc ) const;
        bool would_removal_prevent_flyable( const vehicle_part &vp, const Character &pc ) const;
        bool would_repair_prevent_flyable( const vehicle_part &vp, const Character &pc ) const;

        bool can_control_in_air( const Character &pc ) const;
        bool can_control_on_land( const Character &pc ) const;







        float k_traction( map &here, float wheel_traction_area ) const;

        float k_traction( map &here, float wheel_traction_area, units::mass mass ) const;




        int drag_str_req_at( map &here, const tripoint_bub_ms &tile_pos,
                             units::mass projected_mass ) const;
        int drag_str_req_at( map &here, const tripoint_bub_ms &tile_pos ) const;




        units::power static_drag( bool actual = true ) const;


        float strain( map &here ) const;


        bool sufficient_wheel_config() const;
        bool balanced_wheel_config( map &here ) const;
        bool valid_wheel_config( map &here ) const;



        float steering_effectiveness( map &here ) const;


        float handling_difficulty( map &here ) const;



        void idle( map &here, bool on_map );

        void alarm( map &here );

        void slow_leak( map &here );



        void thrust( map &here, int thd, int z = 0 );






        void smart_controller_handle_turn( map &here,
                                           const std::optional<float> &k_traction_cache = std::nullopt );

        bool has_available_electric_engine();
        void disable_smart_controller_if_needed();


        int slowdown( map &here, int velocity ) const;


        void possibly_recover_from_skid();


        float forward_velocity() const;


        void cruise_thrust( map &here, int amount );


        void turn( units::angle deg );




        bool a11y_rotate_in_place( map &here, int steering_steps, bool preview_only = false );


        bool collision( map &here, std::vector<veh_collision> &colls,
                        const tripoint_rel_ms &dp,
                        bool just_detect, bool bash_floor = false );



        veh_collision part_collision( map &here, int part, const tripoint_abs_ms &p,
                                      bool just_detect, bool bash_floor );


        static double hit_probability( const item &it, const vehicle_part *vp_wheel );


        double wheel_damage_chance_vs_item( const item &it, const vehicle_part &vp_wheel ) const;


        void damage_wheel_on_item( vehicle_part *vp_wheel, const item &it,
                                   std::vector<std::string> *messages ) const;


        void handle_trap( map *here, const tripoint_bub_ms &p, vehicle_part &vp_wheel );
        void activate_magical_follow();
        void activate_animal_follow( map &here );



        void selfdrive( map &here, int trn, int acceleration );



        bool check_heli_descend( map &here, Character &p ) const;
        bool check_heli_ascend( map &here, Character &p ) const;
        bool check_is_heli_landed( map &here );






        void pldrive( map &here, Character &driver, int trn, int acceleration, int z = 0 );








        int prepare_tool( map &here, item &tool ) const;
        static bool use_vehicle_tool( vehicle &veh, map *here, const tripoint_bub_ms &vp_pos,
                                      const itype_id &tool_type,
                                      bool no_invoke = false );





        std::pair<const itype_id &, int> tool_ammo_available( map &here, const itype_id &tool_type ) const;









        static std::map<std::string, multimag_pocket_state> prepare_multimag_pockets(
            vehicle &veh, map &here, item &tool, const gun_mode_id &mode,
            const itype_id &preferred_primary = itype_id::NULL_ID() );


        static std::map<std::string, multimag_pocket_state> prepare_multimag_pockets(
            vehicle &veh, map &here, item &tool );
        static void drain_back_multimag( vehicle &veh, map &here, const item &tool,
                                         const std::map<std::string, multimag_pocket_state> &bindings );







        int run_legacy_charge_tool_uses( map &here, const itype_id &tool_type, int uses );






        std::map<item, int> prepare_tools( map &here, const vehicle_part &vp ) const;





        void make_active( item_location &loc );





        std::optional<vehicle_stack::iterator> add_item( map &here, vehicle_part &vp, const item &itm );





        int add_charges( map &here, vehicle_part &vp, const item &itm );


        bool remove_item( vehicle_part &vp, item *it );
        vehicle_stack::iterator remove_item( vehicle_part &vp, const vehicle_stack::const_iterator &it );



        vehicle_stack get_items( const vehicle_part &vp ) const;
        vehicle_stack get_items( vehicle_part &vp );

        std::vector<item> &get_tools( vehicle_part &vp );
        const std::vector<item> &get_tools( const vehicle_part &vp ) const;


        void place_spawn_items( map &here );

        void place_zones( map &pmap ) const;

        void gain_moves( map &here );


        bool decrement_summon_timer( map &here );


        void stop( map &here );

        void refresh_insides();

        void unboard_all( map &here ) const;





        int damage( map &here, int p, int dmg, const damage_type_id &type = damage_type_id( "bash" ),
                    bool aimed = true );


        void damage_all( map &here, int dmg1, int dmg2, const damage_type_id &type,
                         const point_rel_ms &impact );


        void shift_parts( map &here, const point_rel_ms &delta );
        bool shift_if_needed( map &here );











        void shed_loose_parts( trinary shed_cables = trinary::NONE, map *here = nullptr,
                               const tripoint_bub_ms *dst = nullptr );








        void unlink_cables( map &here, const point_rel_ms &mount, Character &remover,
                            bool unlink_items = false,
                            bool unlink_tow_cables = false, bool unlink_power_cords = false );







        std::vector<vehicle_part *> turrets();


        turret_data turret_query( vehicle_part &pt );
        turret_data turret_query( map *here,  const tripoint_bub_ms &pos );
        turret_data turret_query( const tripoint_abs_ms &pos );


        void turrets_set_targeting();


        void turrets_set_mode();


        void turrets_aim_and_fire_single();






        bool turrets_aim_and_fire_all_manual( bool show_msg = false );


        void turrets_override_automatic_aim();





        int automatic_fire_turret( vehicle_part &pt );

    private:





        std::vector<vehicle_part *> find_all_ready_turrets( bool manual, bool automatic );






        bool turrets_aim( std::vector<vehicle_part *> &turrets );






        int turrets_aim_and_fire( std::vector<vehicle_part *> &turrets );

    public:




        bool assign_seat( vehicle_part &pt, const npc &who );


        const std::set<tripoint_abs_ms> &get_points( bool force_refresh = false,
                bool no_fake = false ) const;


        void part_project_points( const tripoint_rel_ms &dp );


        std::set<tripoint_abs_ms> get_projected_part_points() const;








        std::list<item> use_charges( map &here, const vpart_position &vp, const itype_id &type,
                                     int &quantity,
                                     const std::function<bool( const item & )> &filter, bool in_tools = false );


        void open( map &here, int part_index );
        void close( map &here, int part_index );
        void lock( int part_index );
        void unlock( int part_index );

        bool part_has_lock( int part_index ) const;
        bool can_close( int part_index, Character &who );


        bool is_foldable() const;

        time_duration folding_time() const;

        time_duration unfolding_time() const;

        item get_folded_item( map &here ) const;


        bool restore_folded_parts( const item &it );


        bool has_security_working( map &here ) const;

        void unlock();



        void open_all_at( map &here, int p );


        void honk_horn( map &here, bool quiet_success = false ) const;
        void reload_seeds( map *here, const tripoint_bub_ms &pos );
        void beeper_sound( map &here ) const;
        void play_music( map &here ) const;
        void play_chimes( map &here ) const;
        void operate_planter( map &here );
        void autopilot_patrol_check( map &here );
        void toggle_autopilot( map &here );
        void enable_patrol( map &here );
        void toggle_tracking();
        void display_effects();

        void operate_scoop( map &here );
        void operate_reaper( map &here );

        void crash_terrain_around( map &here );
        void transform_terrain( map &here );

        void control_engines( map &here );

        bool is_engine_type_on( const vehicle_part &vp, const itype_id &ft ) const;

        bool is_engine_on( const vehicle_part &vp ) const;

        bool is_engine_type( const vehicle_part &vp, const itype_id &ft ) const;

        bool is_engine_type_combustion( const vehicle_part &vp ) const;

        bool is_alternator_on( map &here, const vehicle_part &vp ) const;



        bool start_engine( map &here, vehicle_part &vp, bool turn_on );


        bool has_engine_type( const itype_id &ft, bool enabled ) const;
        monster *get_harnessed_animal( const map &here ) const;


        bool has_engine_type_not( const itype_id &ft, bool enabled ) const;

        std::optional<std::string> has_engine_conflict( const vpart_info &possible_conflict ) const;

        bool is_perpetual_type( const vehicle_part &vp ) const;

        void do_engine_damage( map &here, vehicle_part &vp, int strain );

        void control_doors( map &here );

        rl_vec2d velo_vec() const;

        rl_vec2d face_vec() const;
        rl_vec2d move_vec() const;

        rl_vec2d dir_vec() const;

        void on_move( map &here );

        vehicle *act_on_map( map &here );

        void check_falling_or_floating();










        void precalculate_vehicle_turning( map &here, units::angle new_turn_dir, bool check_rail_direction,
                                           ter_furn_flag ter_flag_to_check, int &wheels_on_rail,
                                           int &turning_wheels_that_are_one_axis_counter ) const;
        bool allow_auto_turn_on_rails( map &here, units::angle &corrected_turn_dir ) const;
        bool allow_manual_turn_on_rails( map &here, units::angle &corrected_turn_dir ) const;
        bool is_wheel_state_correct_to_turn_on_rails( int wheels_on_rail, int wheel_count,
                int turning_wheels_that_are_one_axis ) const;






        void set_submap_moved( map *here, const tripoint_bub_sm &p );
        void translate_cables( const tripoint_rel_ms &offset );
        void use_autoclave( map &here, int p );
        void use_washing_machine( map &here, int p );
        void use_dishwasher( map &here, int p );
        void use_mws( map &here, int p );
        void use_nl_boiler( map &here, int p );
        void use_monster_capture( int part, map *here, const tripoint_bub_ms &pos );
        void use_tiedown_furniture( int part, map *here, const tripoint_bub_ms & );
        void use_harness( int part, map *here, const tripoint_bub_ms &pos );

        void build_electronics_menu( map &here, veh_menu &menu );
        void build_bike_rack_menu( map &here, veh_menu &menu, int part );
        void build_interact_menu( veh_menu &menu, map *here, const tripoint_bub_ms &p, bool with_pickup );
        void interact_with( map *here, const tripoint_bub_ms &p, bool with_pickup = false );

        std::string disp_name() const;


        int lift_strength( map &here ) const;


        bool refresh_zones();

        bounding_box get_bounding_box( bool use_precalc = true, bool no_fake = false );


        void update_time( map &here, const time_point &update_to );


        int catchup_off_map_renewables( const time_point &now );


        faction_id owner = faction_id::NULL_ID();

        faction_id old_owner = faction_id::NULL_ID();

    private:
        mutable double coefficient_air_resistance = 1; // NOLINT(cata-serialize)
        mutable double coefficient_rolling_resistance = 1; // NOLINT(cata-serialize)
        mutable double coefficient_water_resistance = 1; // NOLINT(cata-serialize)
        mutable double draft_m = 1; // NOLINT(cata-serialize)
        mutable double hull_height = 0.3; // NOLINT(cata-serialize)


        mutable tripoint_abs_ms occupied_cache_pos = tripoint_abs_ms::invalid; // NOLINT(cata-serialize)

        mutable units::angle occupied_cache_direction = 0_degrees; // NOLINT(cata-serialize)

        mutable std::set<tripoint_abs_ms> occupied_points; // NOLINT(cata-serialize)


        std::vector<vehicle_part> parts; // NOLINT(cata-serialize)

        std::vector<std::reference_wrapper<const vehicle_part>> real_parts() const;

        std::map<point_rel_ms, vpart_edge_info> edges; // NOLINT(cata-serialize)

        vpart_edge_info get_edge_info( const point_rel_ms &mount ) const;


        void remove_fake_parts( map &here, bool cleanup = true );
        bool should_enable_fake( const tripoint_rel_ms &fake_precalc, const tripoint_rel_ms &parent_precalc,
                                 const tripoint_rel_ms &neighbor_precalc ) const;
    public:

        int part_count() const;

        int part_count_real() const;

        int part_count_real_cached() const;


        vehicle_part &part( int part_num );
        const vehicle_part &part( int part_num ) const;

        int get_non_fake_part( int part_num ) const;


        void update_active_fakes();





        item part_to_item( map &here, const vehicle_part &vp ) const;





        item removed_part( map &here, const vehicle_part &vp ) const;



        std::set<int> advance_precalc_mounts( const point_sm_ms &new_pos, map *here,
                                              const tripoint_bub_ms &src,
                                              const tripoint_rel_ms &dp, int ramp_offset,
                                              bool adjust_pos, std::set<int> parts_to_move );

        bool level_vehicle( map &here );



        std::vector<int> alternators; // NOLINT(cata-serialize)
        std::vector<int> engines; // NOLINT(cata-serialize)
        std::vector<int> reactors; // NOLINT(cata-serialize)
        std::vector<int> solar_panels; // NOLINT(cata-serialize)
        std::vector<int> wind_turbines; // NOLINT(cata-serialize)
        std::vector<int> water_wheels; // NOLINT(cata-serialize)
        std::vector<int> sails; // NOLINT(cata-serialize)
        std::vector<int> funnels; // NOLINT(cata-serialize)
        std::vector<int> emitters; // NOLINT(cata-serialize)

        std::vector<int> loose_parts; // NOLINT(cata-serialize)
        std::vector<int> wheelcache; // NOLINT(cata-serialize)
        std::vector<int> rotors; // NOLINT(cata-serialize)
        std::vector<int> rail_wheelcache; // NOLINT(cata-serialize)
        std::vector<int> steering; // NOLINT(cata-serialize)

        std::vector<int> speciality; // NOLINT(cata-serialize)
        std::vector<int> floating; // NOLINT(cata-serialize)
        std::vector<int> batteries; // NOLINT(cata-serialize)
        std::vector<int> fuel_containers; // NOLINT(cata-serialize)
        std::vector<int> turret_locations; // NOLINT(cata-serialize)
        std::vector<int> mufflers; // NOLINT(cata-serialize)
        std::vector<int> planters; // NOLINT(cata-serialize)
        std::vector<int> accessories; // NOLINT(cata-serialize)
        std::vector<int> cable_ports; // NOLINT(cata-serialize)
        std::vector<int> fake_parts; // NOLINT(cata-serialize)
        std::vector<int> control_req_parts; // NOLINT(cata-serialize)


        std::string name;





        vproto_id type;

        std::map<point_rel_ms, std::vector<int>> relative_parts; // NOLINT(cata-serialize)
        std::set<label> labels;
        std::set<std::string> tags;


        std::map<itype_id, units::energy> fuel_remainder;
        std::map<itype_id, units::energy> fuel_used_last_turn;
        std::unordered_multimap<point_rel_ms, zone_data> loot_zones;
        active_item_cache active_items; // NOLINT(cata-serialize)

        bool magic = false;

        std::optional<time_duration> summon_time_limit = std::nullopt;

        // NOLINTNEXTLINE(cata-serialize)
        std::optional<smart_controller_cache> smart_controller_state = std::nullopt;

        std::optional<smart_controller_config> smart_controller_cfg = std::nullopt;
        bool has_enabled_smart_controller = false; // NOLINT(cata-serialize)

        void add_tag( const std::string &tag );
        bool has_tag( const std::string &tag ) const;

    private:
        safe_reference_anchor anchor; // NOLINT(cata-serialize)
        mutable units::mass mass_cache; // NOLINT(cata-serialize)

        mutable point_rel_ms pivot_cache; // NOLINT(cata-serialize)



        mutable point_rel_ms mount_max; // NOLINT(cata-serialize)
        mutable point_rel_ms mount_min; // NOLINT(cata-serialize)
        mutable point_rel_ms mass_center_precalc; // NOLINT(cata-serialize)
        mutable point_rel_ms mass_center_no_precalc; // NOLINT(cata-serialize)
        tripoint_abs_ms autodrive_local_target =
            tripoint_abs_ms::zero;
        class autodrive_controller;

        std::shared_ptr<autodrive_controller> active_autodrive_controller; // NOLINT(cata-serialize)

    public:





        tripoint_abs_sm sm_pos = tripoint_abs_sm::zero; // NOLINT(cata-serialize)


        int alternator_load = 0; // NOLINT(cata-serialize)

        time_point last_update = calendar::before_time_starts;







        point_sm_ms pos = point_sm_ms::zero;

        int velocity = 0;





        int avg_velocity = 0;

        int cruise_velocity = 0;

        int vertical_velocity = 0;

        int om_id = 0;


        units::angle turn_dir = 0_degrees;

        units::angle last_turn = 0_degrees;

        float of_turn = 0.0f; // NOLINT(cata-serialize)

        float of_turn_carry = 0.0f;
        units::power extra_drag = 0_W; // NOLINT(cata-serialize)

        std::optional<time_point> theft_time;

        // NOLINTNEXTLINE(cata-serialize)
        std::array<units::angle, 2> pivot_rotation = { { 0_degrees, 0_degrees } };

        bounding_box rail_wheel_bounding_box; // NOLINT(cata-serialize)
        point_rel_ms front_left; // NOLINT(cata-serialize)
        point_rel_ms front_right; // NOLINT(cata-serialize)
        towing_data tow_data;

        std::array<point_rel_ms, 2> pivot_anchor;

        tileray face;

        tileray move;

    private:
        bool no_refresh = false; // NOLINT(cata-serialize)


        mutable bool pivot_dirty = true; // NOLINT(cata-serialize)
        mutable bool mass_dirty = true; // NOLINT(cata-serialize)
        mutable bool mass_center_precalc_dirty = true; // NOLINT(cata-serialize)
        mutable bool mass_center_no_precalc_dirty = true; // NOLINT(cata-serialize)

        mutable bool coeff_rolling_dirty = true; // NOLINT(cata-serialize)
        mutable bool coeff_air_dirty = true; // NOLINT(cata-serialize)
        mutable bool coeff_water_dirty = true; // NOLINT(cata-serialize)




        mutable bool coeff_air_changed = true; // NOLINT(cata-serialize)


        mutable bool in_deep_water = false;


        mutable bool in_water = false;

        mutable bool is_flying = false;
        bool flyable = true;
        int requested_z_change = 0;

    public:
        bool is_on_ramp = false;

        bool is_autodriving = false;
        bool is_following = false;
        bool is_patrolling = false;
        bool all_wheels_on_one_axis = false; // NOLINT(cata-serialize)

        bool engine_on = false;

        bool tracking_on = false;

        bool is_locked = false;

        bool is_alarm_on = false;
        bool camera_on = false;
        bool autopilot_on = false;
        bool precollision_on = true;

        bool skidding = false;

        bool check_environmental_effects = false; // NOLINT(cata-serialize)

        bool insides_dirty = true; // NOLINT(cata-serialize)

        bool is_falling = false;

        bool zones_dirty = true; // NOLINT(cata-serialize)


        unsigned char vehicle_noise = 0;


        std::pair<int, double> get_exhaust_part() const;


        tripoint_bub_ms exhaust_dest( map *here, int part ) const;



        std::vector<std::tuple<point_rel_ms, int, std::string>> get_debug_overlay_data() const;
};




class RemovePartHandler
{
    public:
        virtual ~RemovePartHandler() = default;

        virtual void unboard( map *here, const tripoint_bub_ms &loc ) = 0;
        virtual void add_item_or_charges( map *here, const tripoint_bub_ms &loc, item it,
                                          bool permit_oob ) = 0;
        virtual void set_transparency_cache_dirty( int z ) = 0;
        virtual void set_floor_cache_dirty( int z ) = 0;
        virtual void removed( map *here, vehicle &veh, int part ) = 0;
        virtual void spawn_animal_from_part( item &base, map *here, const tripoint_bub_ms &loc ) = 0;
        virtual map &get_map_ref() = 0;
};

class DefaultRemovePartHandler : public RemovePartHandler
{
    public:
        ~DefaultRemovePartHandler() override = default;

        void unboard( map *here, const tripoint_bub_ms &loc ) override {
            here->unboard_vehicle( loc );
        }
        void add_item_or_charges( map *here, const tripoint_bub_ms &loc, item it,
                                  bool ) override {
            here->add_item_or_charges( loc, std::move( it ) );
        }
        void set_transparency_cache_dirty( const int z ) override {
            map &here = get_map();
            here.set_transparency_cache_dirty( z );
        }
        void set_floor_cache_dirty( const int z ) override {
            get_map().set_floor_cache_dirty( z );
        }
        void removed( map *here, vehicle &veh, int part ) override;
        void spawn_animal_from_part( item &base, map * , const tripoint_bub_ms &loc ) override {
            base.release_monster( loc, 1 );
        }
        map &get_map_ref() override {
            return get_map();
        }
};

class MapgenRemovePartHandler : public RemovePartHandler
{
    private:
        map &m;

    public:
        explicit MapgenRemovePartHandler( map &m ) : m( m ) { }

        ~MapgenRemovePartHandler() override = default;

        void unboard( map * , const tripoint_bub_ms & ) override {
            debugmsg( "Tried to unboard during mapgen!" );


        }
        void add_item_or_charges( map *here, const tripoint_bub_ms &loc, item it,
                                  bool permit_oob ) override;
        void set_transparency_cache_dirty( const int ) override {

        }
        void set_floor_cache_dirty( const int ) override {

        }
        void removed( map *here, vehicle &veh, const int ) override {

            here->dirty_vehicle_list.insert( &veh );
        }
        void spawn_animal_from_part( item & , map * ,
                                     const tripoint_bub_ms & ) override {
            debugmsg( "Tried to spawn animal from vehicle part during mapgen!" );




        }
        map &get_map_ref() override {
            return m;
        }
};
std::unique_ptr<talker> get_talker_for( vehicle &me );
std::unique_ptr<const_talker> get_talker_for( const vehicle &me );
std::unique_ptr<talker> get_talker_for( vehicle *me );
#endif
