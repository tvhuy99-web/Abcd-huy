#pragma once
#ifndef CATA_SRC_MONSTER_H
#define CATA_SRC_MONSTER_H

#include <algorithm>
#include <bitset>
#include <climits>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "character_id.h"
#include "color.h"
#include "compatibility.h"
#include "coordinates.h"
#include "creature.h"
#include "type_id.h"
#include "units_fwd.h"
#include "value_ptr.h"
#include "weakpoint.h"

class Character;
class JsonObject;
class JsonOut;
class effect_source;
class item;
class item_location;
class map;
enum class mon_trigger : int;
enum class phase_id : int;
struct monster_plan;
struct mtype;

class mon_special_attack
{
    public:
        int cooldown = 0;
        bool enabled = true;

        void serialize( JsonOut &json ) const;

};

enum monster_attitude {
    MATT_NULL = 0,
    MATT_FRIEND,
    MATT_FPASSIVE,
    MATT_FLEE,
    MATT_IGNORE,
    MATT_FOLLOW,
    MATT_ATTACK,
    NUM_MONSTER_ATTITUDES
};

enum monster_effect_cache_fields {
    MOVEMENT_IMPAIRED = 0,
    FLEEING,
    VISION_IMPAIRED,
    NUM_MEFF
};

enum monster_horde_attraction {
    MHA_NULL = 0,
    MHA_ALWAYS,
    MHA_LARGE,
    MHA_OUTDOORS,
    MHA_OUTDOORS_AND_LARGE,
    MHA_NEVER,
    NUM_MONSTER_HORDE_ATTRACTION
};

class monster : public Creature
{
        friend class editmap_ui;
    public:
        monster();
        explicit monster( const mtype_id &id );
        monster( const mtype_id &id, const tripoint_bub_ms &pos );
        monster( const monster & );
        monster( monster && ) noexcept( map_is_noexcept );
        ~monster() override;
        monster &operator=( const monster & );
        monster &operator=( monster && ) noexcept( string_is_noexcept );

        bool is_monster() const override {
            return true;
        }
        monster *as_monster() override {
            return this;
        }
        const monster *as_monster() const override {
            return this;
        }

        mfaction_id get_monster_faction() const override {
            return faction.id();
        }
        void gravity_check() override;
        void gravity_check( map *here ) override;
        void poly( const mtype_id &id );
        bool can_upgrade() const;
        void hasten_upgrade();
        int get_upgrade_time() const;
        void allow_upgrade();
        void try_upgrade( bool pin_time );
        void set_baby_timer( const time_point &time );
        void try_reproduce();
        void try_biosignature();
        void refill_udders();
        void spawn( const tripoint_bub_ms &p );
        void spawn( const tripoint_abs_ms &loc );
        std::vector<material_id> get_absorb_material() const;
        std::vector<material_id> get_no_absorb_material() const;
        creature_size get_size() const override;
        units::mass get_weight() const override;
        units::mass weight_capacity() const override;
        units::volume get_volume() const;
        int get_hp( const bodypart_id & ) const override;
        int get_hp() const override;
        int get_hp_max( const bodypart_id & ) const override;
        int get_hp_max() const override;
        int hp_percentage() const override;
        int get_eff_per() const override;
        void witness_thievery( item *it ) override;

        float get_mountable_weight_ratio() const;

        static std::string speed_description( float mon_speed_rating,
                                              bool immobile = false,
                                              const speed_description_id &speed_desc = speed_description_id::NULL_ID() );


        std::string get_name() const override;
        std::string name( unsigned int quantity = 1 ) const;
        std::string name_with_armor() const;

        std::string disp_name( bool possessive = false, bool capitalize_first = false ) const override;
        std::string skin_name() const override;
        void get_HP_Bar( nc_color &color, std::string &text ) const;
        std::pair<std::string, nc_color> get_attitude() const;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;
        void print_info_imgui() const;


        nc_color basic_symbol_color() const override;
        nc_color symbol_color() const override;
        const std::string &symbol() const override;
        bool is_symbol_highlighted() const override;

        nc_color color_with_effects() const;

        std::vector<std::string> extended_description() const override;


        bool has_flag( const mon_flag_id &f ) const final;

        using Creature::has_flag;
        bool has_flag( const flag_id &f ) const override;
        bool can_see() const;
        bool can_hear() const;
        bool can_submerge() const;
        bool can_drown() const;
        bool can_climb() const;
        bool digging() const override;
        bool can_dig() const;
        bool digs() const;
        bool flies() const;
        bool climbs() const;
        bool swims() const;


        int dig_skill() const;

        int get_dig_mod() const;


        int climb_skill() const;

        int get_climb_mod() const;


        int swim_skill() const;

        int get_swim_mod() const;


        bool can_act() const;
        int sight_range( float light_level ) const override;
        bool made_of( const material_id &m ) const override;
        bool made_of_any( const std::set<material_id> &ms ) const override;
        bool made_of( phase_id p ) const;

        bool shearable() const;
        bool is_pet() const;
        bool is_pet_follow() const;
        bool has_intelligence() const;

        bool avoid_trap( const tripoint_bub_ms &pos, const trap &tr ) const override;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );


        void shift( const point_rel_sm &sm_shift );
        void set_patrol_route( const std::vector<point_rel_ms> &patrol_pts_rel_ms );











        bool can_move_to( const tripoint_bub_ms &p ) const;
        bool can_reach_to( const tripoint_bub_ms &p ) const;
        bool will_move_to( const tripoint_bub_ms &p ) const;
        bool will_move_to( map *here, const tripoint_bub_ms &p ) const;
        bool know_danger_at( const tripoint_bub_ms &p ) const;
        bool know_danger_at( map *here, const tripoint_bub_ms &p ) const;

        bool will_reach( const point_bub_ms &p );
        int  turns_to_reach( const point_bub_ms &p );


        bool has_dest() const;

        tripoint_abs_ms get_dest() const;

        Creature *attack_target();

        void set_dest( const tripoint_abs_ms &p );

        void unset_dest();


        bool is_wandering() const;










        void wander_to( const tripoint_abs_ms &p, int f );


        float rate_target( Creature &c, float best, bool smart = false ) const;

        bool mating_angry() const;
        void plan();
        void anger_hostile_seen( const monster_plan &mon_plan );
        void anger_mating_season( const monster_plan &mon_plan );

        void anger_cub_threatened( monster_plan &mon_plan );
        void move();
        void footsteps( const tripoint_bub_ms &p );
        void shove_vehicle( const tripoint_bub_ms &remote_destination,
                            const tripoint_bub_ms &nearby_destination );


        bool is_aquatic_danger( const tripoint_bub_ms &at_pos ) const;




        bool die_if_drowning( const tripoint_bub_ms &at_pos, int chance = 1 );

        tripoint_bub_ms scent_move();
        int calc_movecost( const map &here, const tripoint_bub_ms &f,
                           const tripoint_bub_ms &t, bool force  = false ) const;

        bool is_immune_field( const field_type_id &fid ) const override;
        bool check_immunity_data( const field_immunity_data &ft ) const override;


















        bool move_to( const tripoint_bub_ms &p, bool force = false, bool step_on_critter = false,
                      float stagger_adjustment = 1.0 );









        bool attack_at( const tripoint_bub_ms &p );






        bool bash_at( const tripoint_bub_ms &p );











        bool push_to( const tripoint_bub_ms &p, int boost, size_t depth );


        std::map<damage_type_id, int> bash_skill() const;
        std::map<damage_type_id, int> bash_estimate() const;


        std::map<damage_type_id, int> group_bash_skill( const tripoint_bub_ms &target );

        void stumble_base( bool is_voluntary );
        void stumble_voluntary();
        void stumble_involuntary();
        void knock_back_to( const tripoint_bub_ms &to ) override;


        bool is_fleeing( Character &u ) const;
        monster_attitude attitude( const Character *u = nullptr ) const;
        Attitude attitude_to( const Creature &other ) const override;
        void process_triggers();

        bool is_underwater() const override;
        bool is_on_ground() const override;
        bool is_warm() const override;
        bool in_species( const species_id &spec ) const override;
        void respond_to_light( float light_level );

        bool has_weapon() const override;
        bool is_dead_state() const override;
        bool is_elec_immune() const override;
        bool is_immune_effect( const efftype_id & ) const override;
        bool is_immune_damage( const damage_type_id & ) const override;

        void make_bleed( const effect_source &source, time_duration duration,
                         int intensity = 1, bool permanent = false, bool force = false, bool defferred = false );

        const weakpoint *absorb_hit( const weakpoint_attack &attack, const bodypart_id &bp,
                                     damage_instance &dam, const weakpoint &wp = weakpoint() ) override;

        float weakpoint_skill() const;

        bool block_hit( Creature *source, bodypart_id &bp_hit, damage_instance &d ) override;
        bool melee_attack( Creature &target );
        bool melee_attack( Creature &target, float accuracy );
        void melee_attack( Creature &p, bool ) = delete;
        void deal_projectile_attack( map *here, Creature *source, dealt_projectile_attack &attack,
                                     const double &missed_by = 0, bool print_messages = true,
                                     const weakpoint_attack &wp_attack = weakpoint_attack() ) override;
        void deal_damage_handle_type( const effect_source &source, const damage_unit &du, bodypart_id bp,
                                      int &damage, int &pain ) override;
        void apply_damage( Creature *source, bodypart_id bp, int dam,
                           bool bypass_med = false ) override;

        void explode();

        void die_in_explosion( Creature *source );

        void heal_bp( bodypart_id bp, int dam ) override;




        int heal( int delta_hp, bool overheal = false );




        void set_hp( int hp );


        void process_effects() override;


        bool movement_impaired();


        bool move_effects( bool attacking ) override;


        std::string get_effect_status() const;

        float power_rating() const override;
        float speed_rating() const override;

        int get_worn_armor_val( const damage_type_id &dt ) const;
        int  get_armor_type( const damage_type_id &dt, bodypart_id bp ) const override;

        float get_hit_base() const override;
        float get_dodge_base() const override;

        float  get_dodge() const override;
        int blocks_left = 0;
        float  get_melee() const override;
        float  hit_roll() const override;
        float  dodge_roll() const override;

        bool can_attack_high() const override;
        int get_grab_strength() const;

        void add_grab( bodypart_str_id bp );
        void remove_grab( bodypart_str_id bp );
        bool is_grabbing( bodypart_str_id bp );

        monster_horde_attraction get_horde_attraction();
        void set_horde_attraction( monster_horde_attraction mha );
        bool will_join_horde( int size );


        float fall_damage_mod() const override;

        int impact( int force, const tripoint_bub_ms &pos ) override;

        bool has_grab_break_tec() const override;

        float stability_roll() const override;

        void on_dodge( Creature *source, float difficulty, float training_level = 0.0 ) override;
        void on_try_dodge() override {}

        void on_hit( map *here, Creature *source, bodypart_id bp_hit,
                     float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) override;


        void reset_special( const std::string &special_name );

        void reset_special_rng( const std::string &special_name );

        void set_special( const std::string &special_name, int time );

        void disable_special( const std::string &special_name );

        bool has_special( const std::string &special_name ) const;

        bool special_available( std::string_view special_name ) const;

        void process_turn() override;

        void reset_bonuses() override;

        void reset_stats() override;

        void die( map *here, Creature *killer ) override;
        void drop_items_on_death( map *here, item *corpse ) const;
        void spawn_dissectables_on_death( item *corpse ) const;

        void generate_inventory( bool disableDrops = true );







        bool make_fungus();
        void make_friendly();

        void make_ally( const monster &z );

        void add_item( const item &it );






        units::energy use_mech_power( units::energy amt );
        bool check_mech_powered() const;
        int mech_str_addition() const;








        void hear_sound( const tripoint_bub_ms &source, int vol, int distance, bool provocative );

        bool is_hallucination() const override;

        float luminance() const;

        bool is_electrical() const override;

        bool is_fae() const override;

        bool is_nether() const override;

        bool has_mind() const override;

        field_type_id bloodType() const override;
        field_type_id gibType() const override;

        using Creature::add_msg_if_npc;
        void add_msg_if_npc( const std::string &msg ) const override;
        void add_msg_if_npc( const game_message_params &params, const std::string &msg ) const override;
        using Creature::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;


        std::unordered_set<bodypart_str_id> grabbed_limbs;

        tripoint_abs_ms wander_pos;
        bool provocative_sound = false;
        int wandf = 0;
        std::vector<item> inv;
        std::vector<item> dissectable_inv;
        Character *mounted_player = nullptr;
        character_id mounted_player_id;
        character_id dragged_foe_id;
        cata::value_ptr<item> tied_item;
        cata::value_ptr<item> tack_item;
        cata::value_ptr<item> armor_item;
        cata::value_ptr<item> storage_item;
        cata::value_ptr<item> battery_item;
        units::mass get_carried_weight() const;
        units::volume get_carried_volume() const;
        void move_special_item_to_inv( cata::value_ptr<item> &it );


        int friendly = 0;
        int anger = 0;
        int morale = 2;
    private:
        int amount_eaten = 0;
        void recheck_fed_status();
    public:

        mfaction_id faction;

        std::set<int> mission_ids;

        std::vector<std::string> mission_fused;
        const mtype *type;

        bool no_extra_death_drops = false;

        bool no_corpse_quiet = false;

        bool death_drops = true;

        bool quiet_death = false;
        bool is_dead() const;
        bool made_footstep = false;

        bool is_nemesis() const;

        std::string unique_name;

        std::string nickname;
        bool hallucination = false;

        int fish_population = 1;

        short ignoring = 0;
        bool aggro_character = true;

        std::optional<time_point> lastseen_turn;


        int staircount = 0;


        std::map<itype_id, int> ammo;






        item to_item() const;





        void init_from_item( item_location loc );



        void on_unload();






        void on_load();

        const pathfinding_settings &get_pathfinding_settings() const override;
        std::function<bool( const tripoint_bub_ms & )> get_path_avoid() const override;
        std::vector<std::pair<std::string, std::string>> get_overlay_ids() const;
    private:
        void process_trigger( mon_trigger trig, int amount );
        void process_trigger( mon_trigger trig, const std::function<int()> &amount_func );

        int hp = 60;
        std::map<std::string, mon_special_attack, std::less<>> special_attacks;
        std::optional<tripoint_abs_ms> goal;
        bool dead = false;

        int next_upgrade_time();
        bool upgrades = false;
        int upgrade_time = -1;
        bool reproduces = false;
        std::optional<time_point> baby_timer;
        bool biosignatures = false;
        std::optional<time_point> biosig_timer;
        time_point udder_timer;
        monster_horde_attraction horde_attraction = MHA_NULL;

        std::vector<tripoint_bub_ms> path;


        time_point pathfinding_cd = calendar::turn;
        time_duration pathfinding_backoff = 2_seconds;

        bool can_pathfind() const {
            return pathfinding_cd <= calendar::turn;
        }
        void reset_pathfinding_cd() {
            pathfinding_cd = calendar::turn;
            pathfinding_backoff = 2_seconds;
        }
        void increment_pathfinding_cd() {
            pathfinding_cd = calendar::turn + pathfinding_backoff;
            pathfinding_backoff = std::min( pathfinding_backoff * 2, 10_seconds );
        }


        std::vector<tripoint_abs_ms> patrol_route;
        int next_patrol_point = -1;

        std::bitset<NUM_MEFF> effect_cache;
        int turns_since_target = 0;

        Character *find_dragged_foe();
        void nursebot_operate( Character *dragged_foe );

    protected:
        void store( JsonOut &json ) const;
        void load( const JsonObject &data );

        void on_move( const tripoint_abs_ms &old_pos ) override;
        void on_effect_int_change( const efftype_id &eid, int intensity,
                                   const bodypart_id &bp ) override;

        void process_one_effect( effect &it, bool is_new ) override;
};

#endif
