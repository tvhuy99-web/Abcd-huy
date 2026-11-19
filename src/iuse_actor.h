#pragma once
#ifndef CATA_SRC_IUSE_ACTOR_H
#define CATA_SRC_IUSE_ACTOR_H

#include <climits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "color.h"
#include "coords_fwd.h"
#include "damage.h"
#include "explosion.h"
#include "item_transformation.h"
#include "iuse.h"
#include "ret_val.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"

class Character;
class JsonObject;
class item;
class item_location;
class map;
class npc;
class npc_template;
struct furn_t;

enum class link_state : int;
enum class ot_match_type : int;
namespace sounds
{
enum class sound_t : int;
}


template<typename T>
item_location form_loc_recursive( T &loc, item &it );











class iuse_transform : public iuse_actor
{
    public:
        item_transformation transform;


        translation msg_transform;


        int sound_volume = 0;


        bool need_worn = false;


        bool need_wielding = false;


        bool need_empty = false;


        int moves = 0;




        int chance = 100;


        bool set_timer = false;


        translation damage_failure_msg;


        int need_fire = 0;


        translation need_fire_msg;


        int need_charges = 0;


        translation need_charges_msg;


        std::map<quality_id, int> qualities_needed;

        translation menu_text;

        explicit iuse_transform( const std::string &type = "transform" ) : iuse_actor( type ) {}

        ~iuse_transform() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *, item &, map *, const tripoint_bub_ms &pos ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &, const item &, map *here,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
        void finalize( const itype_id &my_item_type ) override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

class message_iuse : public iuse_actor
{
    public:
        explicit message_iuse( const std::string &type = "message" ) : iuse_actor( type ) {}


        translation name;


        translation message;

        ~message_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *, item &, map *here, const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
};

class mp3_iuse : public iuse_actor
{
        itype_id transform;
        translation msg;
        bool activate;
    public:
        explicit mp3_iuse( const std::string &type = "mp3" ) : iuse_actor( type ) {}

        ~mp3_iuse() override = default;
        void load( const JsonObject &jo, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *, item &, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
};

class sound_iuse : public iuse_actor
{
    public:
        explicit sound_iuse( const std::string &type = "sound" ) : iuse_actor( type ) {}


        translation name;


        translation sound_message;

        sounds::sound_t sound_type;
        int sound_volume = 0;
        std::string sound_id;
        std::string sound_variant;

        ~sound_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        std::optional<int> use( Character *, item &, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
};






class explosion_iuse : public iuse_actor
{
    public:

        std::set<ammo_effect_str_id> ammo_effects;



        explosion_data explosion;



        int draw_explosion_radius = -1;
        nc_color draw_explosion_color = c_white;

        bool do_flashbang = false;
        bool flashbang_player_immune = false;

        int fields_radius = -1;
        field_type_id fields_type;
        int fields_min_intensity = 1;
        int fields_max_intensity = 0;

        int emp_blast_radius = -1;

        int scrambler_blast_radius = -1;

        explicit explosion_iuse( const std::string &type = "explosion" ) : iuse_actor( type ) {}

        ~explosion_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

namespace iuse
{


struct effect_data {
    efftype_id id;
    time_duration duration = 0_seconds;
    bodypart_id bp = bodypart_id( "bp_null" );
    int intensity;
    bool permanent = false;

    void deserialize( const JsonObject &jo );

    effect_data() = default;
    effect_data( const efftype_id &nid, const time_duration &dur, bodypart_id nbp, bool perm ) :
        id( nid ), duration( dur ), bp( nbp ), permanent( perm ) {}
};

}




class consume_drug_iuse : public iuse_actor
{
    public:

        translation activation_message;

        std::map<std::string, int> fields_produced;

        std::map<itype_id, int> charges_needed;

        std::map<itype_id, int> tools_needed;

        std::vector<iuse::effect_data> effects;

        std::map<std::string, int> stat_adjustments;


        std::map<vitamin_id, std::pair<int, int>> vitamins;


        std::vector<damage_over_time_data> damage_over_time;


        int moves = 100;

        explicit consume_drug_iuse( const std::string &type = "consume_drug" ) : iuse_actor( type ) {}

        ~consume_drug_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;


        itype_id used_up_item = itype_id::NULL_ID();
};







class delayed_transform_iuse : public iuse_transform
{
    public:



        int transform_age = 0;




        translation not_ready_msg;


        int time_to_do( const item &it ) const;

        explicit delayed_transform_iuse( const std::string &type = "delayed_transform" ) : iuse_transform(
                type ) {}

        ~delayed_transform_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class place_monster_iuse : public iuse_actor
{
    public:

        mtype_id mtypeid;


        bool place_randomly = false;

        int moves = 100;

        int difficulty = 0;

        translation friendly_msg;

        translation hostile_msg;

        std::set<skill_id> skills;

        bool is_pet = false;

        int need_charges = 0;

        place_monster_iuse() : iuse_actor( "place_monster" ) { }
        ~place_monster_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class change_scent_iuse : public iuse_actor
{
    public:

        scenttype_id scenttypeid;

        int moves = 100;

        int charges_to_use = 1;

        int scent_mod = 0;

        time_duration duration;

        bool waterproof = false;

        std::vector<iuse::effect_data> effects;

        change_scent_iuse() : iuse_actor( "change_scent" ) { }
        ~change_scent_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class place_npc_iuse : public iuse_actor
{
    public:
        string_id<npc_template> npc_class_id;
        bool place_randomly = false;
        int radius = 1;
        int moves = 100;
        translation summon_msg;

        place_npc_iuse() : iuse_actor( "place_npc" ) { }
        ~place_npc_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class deploy_furn_actor : public iuse_actor
{
    public:



        string_id<furn_t> furn_type;

        deploy_furn_actor() : iuse_actor( "deploy_furn" ) {}

        ~deploy_furn_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};




class deploy_appliance_actor : public iuse_actor
{
    public:
        itype_id appliance_base;

        deploy_appliance_actor() : iuse_actor( "deploy_appliance" ) {}
        ~deploy_appliance_actor() override = default;

        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};




class reveal_map_actor : public iuse_actor
{
    public:







        int radius = 0;



        std::vector<std::pair<std::string, ot_match_type>> omt_types;



        translation message;

        void reveal_targets(
            const tripoint_abs_omt &center, const std::pair<std::string, ot_match_type> &target,
            int reveal_distance ) const;

        explicit reveal_map_actor( const std::string &type = "reveal_map" ) : iuse_actor( type ) {}

        ~reveal_map_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class firestarter_actor : public iuse_actor
{
    public:



        int moves_cost_fast = 100;




        int moves_cost_slow = 1000;




        bool need_sunlight = false;


        std::map<quality_id, int> qualities_needed;

        enum class start_type : int {
            NONE,
            FIRE,
            SMOKER,
            KILN,
        };

        static start_type prep_firestarter_use( Character &p, map *here, tripoint_bub_ms &pos );

        static void resolve_firestarter_use( Character *p, map *here, const tripoint_bub_ms &pos,
                                             start_type st );

        static bool resolve_start( Character *p, map *here, const tripoint_bub_ms &pos, start_type type );


        float light_mod( map *here, const tripoint_bub_ms &pos ) const;

        int moves_cost_by_fuel( const tripoint_bub_ms &pos ) const;

        explicit firestarter_actor( const std::string &type = "firestarter" ) : iuse_actor( type ) {}

        ~firestarter_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &spos ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &p, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;





        bool npc_start_fire( npc &who, item &tool,
                             const tripoint_bub_ms &fire_pos ) const;
};




class salvage_actor : public iuse_actor
{
    public:

        int moves_per_part = 25;

        std::optional<int> try_to_cut_up( Character &p, item &cutter, item_location &cut ) const;
        int time_to_cut_up( const item &it ) const;
        bool valid_to_cut_up( const Character *p, const item &it ) const;






        static std::map<itype_id, int> salvage_results( item_location cut, double efficiency );

        explicit salvage_actor( const std::string &type = "salvage" ) : iuse_actor( type ) {}

        ~salvage_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        std::optional<int> use( Character *, item &, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
    private:
        void cut_up( Character &p, item_location &cut ) const;
};




class inscribe_actor : public iuse_actor
{
    public:

        bool on_items = true;
        bool on_terrain = false;


        bool material_restricted = true;


        std::set<material_id> material_whitelist;


        translation verb;
        translation gerund;

        bool item_inscription( item &tool, item &cut ) const;

        explicit inscribe_actor( const std::string &type = "inscribe" ) : iuse_actor( type ) {}

        ~inscribe_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};





class fireweapon_off_actor : public iuse_actor
{
    public:
        itype_id target_id;
        translation success_message;
        translation failure_message;
        int noise = 0;
        int moves = 0;

        int success_chance = INT_MIN;

        fireweapon_off_actor() : iuse_actor( "fireweapon_off" ) {}

        ~fireweapon_off_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here, const tripoint_bub_ms & ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &p, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class fireweapon_on_actor : public iuse_actor
{
    public:
        translation noise_message;
        translation charges_extinguish_message;
        translation water_extinguish_message;
        translation auto_extinguish_message;
        int noise_chance = 1;

        explicit fireweapon_on_actor( const std::string &type = "fireweapon_on" ) : iuse_actor( type ) {}

        ~fireweapon_on_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *, item &, map *here, const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class manualnoise_actor : public iuse_actor
{
    public:
        translation use_message;
        translation noise_message;
        std::string noise_id;
        std::string noise_variant;
        int noise = 0;
        int moves = 0;

        explicit manualnoise_actor( const std::string &type = "manualnoise" ) : iuse_actor( type ) {}

        ~manualnoise_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &, map *here, const tripoint_bub_ms & ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &p, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

class play_instrument_iuse : public iuse_actor
{
    public:
        explicit play_instrument_iuse( const std::string &type = "play_instrument" ) : iuse_actor( type ) {}

        ~play_instrument_iuse() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &p, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class musical_instrument_actor : public iuse_actor
{
    public:



        int speed_penalty = 0;



        int volume = 0;



        int fun = 0;



        int fun_bonus = 0;



        std::vector<translation> player_descriptions;



        std::vector<translation> npc_descriptions;



        time_duration description_frequency = 0_turns;

        explicit musical_instrument_actor( const std::string &type = "musical_instrument" ) : iuse_actor(
                type ) {}

        ~musical_instrument_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here, const tripoint_bub_ms & ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &p, const item &, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class learn_spell_actor : public iuse_actor
{
    public:

        std::vector<std::string> spells;

        explicit learn_spell_actor( const std::string &type = "learn_spell" ) : iuse_actor( type ) {}

        ~learn_spell_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};




class cast_spell_actor : public iuse_actor
{
    public:

        bool no_fail = false;

        spell_id item_spell;
        int spell_level = 0;

        bool need_worn = false;

        bool need_wielding = false;

        bool mundane = false;

        explicit cast_spell_actor( const std::string &type = "cast_spell" ) : iuse_actor( type ) {}

        ~cast_spell_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map * ,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
        std::string get_name() const override;
};




class holster_actor : public iuse_actor
{
    public:

        translation holster_prompt;

        translation holster_msg;


        bool can_holster( const item &holster, const item &obj ) const;


        bool store( Character &you, item &holster, item &obj ) const;

        explicit holster_actor( const std::string &type = "holster" ) : iuse_actor( type ) {}

        ~holster_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *you, item &it, map *here,
                                const tripoint_bub_ms &p ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

class ammobelt_actor : public iuse_actor
{
    public:
        itype_id belt;

        ammobelt_actor() : iuse_actor( "ammobelt" ) {}

        ~ammobelt_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};




class repair_item_actor : public iuse_actor
{
    public:

        std::set<material_id> materials;

        skill_id used_skill;

        int trains_skill_to = 0;




        float cost_scaling = 1.0f;

        int tool_quality = 0;

        int move_cost = 0;

        enum attempt_hint : int {
            AS_SUCCESS = 0,
            AS_RETRY,
            AS_FAILURE,
            AS_DESTROYED,
            AS_CANT,
            AS_CANT_USE_TOOL,
            AS_CANT_YET
        };

        enum repair_type : int {
            RT_NOTHING = 0,
            RT_REPAIR = 1,
            RT_REFIT = 2,
            RT_DOWNSIZING = 3,
            RT_UPSIZING = 4,

            RT_PRACTICE = 6,
            NUM_REPAIR_TYPES = 7
        };


        attempt_hint repair( Character &pl, item &tool, item_location &fix, bool refit_only = false ) const;







        bool can_repair_target( Character &pl, const item &fix, bool print_msg,
                                bool check_consumed_available ) const;

        bool can_use_tool( const Character &p, const item &tool, bool print_msg ) const;


        std::set<itype_id> get_valid_repair_materials( const item &fix ) const;







        bool handle_components( Character &pl, const item &fix, bool print_msg, bool just_check,
                                bool check_consumed_available ) const;

        std::pair<float, float> repair_chance(
            const Character &pl, const item &fix, repair_type action_type ) const;

        repair_type default_action( const item &fix, int current_skill_level ) const;





        int repair_recipe_difficulty( const item &fix ) const;

        static std::string action_description( repair_type );

        explicit repair_item_actor( const std::string &type = "repair_item" ) : iuse_actor( type ) {}

        ~repair_item_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        std::string get_name() const override;
        std::string get_description() const override;
};

class heal_actor : public iuse_actor
{
    public:

        float limb_power = 0.0f;

        float head_power = 0.0f;

        float torso_power = 0.0f;

        float bandages_power = 0.0f;

        float bandages_scaling = 0.0f;

        float disinfectant_power = 0.0f;

        float disinfectant_scaling = 0.0f;

        int bleed = 0;

        float bite = 0.0f;

        float infect = 0.0f;

        int move_cost = 100;

        float limb_scaling = 0.0f;

        float head_scaling = 0.0f;

        float torso_scaling = 0.0f;

        std::vector<iuse::effect_data> effects;





        itype_id used_up_item_id;
        int used_up_item_quantity = 1;
        int used_up_item_charges = 1;
        std::set<flag_id> used_up_item_flags;


        int get_heal_value( const Character &healer, bodypart_id healed ) const;

        int get_bandaged_level( const Character &healer ) const;

        int get_disinfected_level( const Character &healer ) const;

        int get_stopbleed_level( const Character &healer ) const;

        int finish_using( Character &healer, Character &patient, item &it, bodypart_id healed ) const;

        bodypart_id use_healing_item( Character &healer, Character &patient, item &it, bool force ) const;

        explicit heal_actor( const std::string &type = "heal" ) : iuse_actor( type ) {}

        ~heal_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

class place_trap_actor : public iuse_actor
{
    public:
        struct data {
            data();
            trap_str_id trap;

            translation done_message;

            int practice = 0;

            int moves = 100;

            int move_cost( const Character &you ) const;
            void load( const JsonObject &obj );
            void deserialize( const JsonObject &obj ) {
                load( obj );
            }
        };

        bool allow_underwater = false;

        bool allow_under_player = false;

        bool needs_solid_neighbor = false;




        ter_str_id needs_neighbor_terrain;

        data unburied_data;



        translation bury_question;

        data buried_data;



        trap_str_id outer_layer_trap;
        bool is_allowed( Character &p, const tripoint_bub_ms &pos, const std::string &name ) const;

        explicit place_trap_actor( const std::string &type = "place_trap" );
        ~place_trap_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

class emit_actor : public iuse_actor
{
    public:
        std::set<emit_id> emits;

        bool scale_qty = false;

        explicit emit_actor( const std::string &type = "emit_actor" ) : iuse_actor( type ) {}
        ~emit_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class saw_barrel_actor : public iuse_actor
{
    public:
        explicit saw_barrel_actor( const std::string &type = "saw_barrel" ) : iuse_actor( type ) {}

        void load( const JsonObject &jo, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        ret_val<void> can_use_on( const Character &you, const item &it, const item &target ) const;
};

class saw_stock_actor : public iuse_actor
{
    public:
        explicit saw_stock_actor( const std::string &type = "saw_stock" ) : iuse_actor( type ) {}

        void load( const JsonObject &jo, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        ret_val<void> can_use_on( const Character &you, const item &it, const item &target ) const;
};


class molle_attach_actor : public iuse_actor
{
    public:
        explicit molle_attach_actor( const std::string &type = "attach_molle" ) : iuse_actor( type ) {}

        int size;
        int moves;

        void load( const JsonObject &jo, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};


class molle_detach_actor : public iuse_actor
{
    public:
        explicit molle_detach_actor( const std::string &type = "detach_molle" ) : iuse_actor( type ) {}

        int moves;

        void load( const JsonObject &jo, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

class install_bionic_actor : public iuse_actor
{
    public:
        explicit install_bionic_actor( const std::string &type = "install_bionic" ) : iuse_actor( type ) {}

        void load( const JsonObject &, const std::string & ) override {}
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class detach_gunmods_actor : public iuse_actor
{
    public:
        explicit detach_gunmods_actor( const std::string &type = "detach_gunmods" ) : iuse_actor( type ) {}

        void load( const JsonObject &, const std::string & ) override {}
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *, const tripoint_bub_ms & ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class modify_gunmods_actor : public iuse_actor
{
    public:
        explicit modify_gunmods_actor( const std::string &type = "modify_gunmods" ) : iuse_actor( type ) {}

        void load( const JsonObject &, const std::string & ) override {}
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map * ,
                                const tripoint_bub_ms &pos ) const override;
        using iuse_actor::can_use;
        ret_val<void> can_use( const Character &, const item &it, map *,
                               const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class link_up_actor : public iuse_actor
{
    public:

        int cable_length = -1;

        units::power charge_rate = 0_W;

        float efficiency = 0.85f;

        int move_cost = 5;

        translation menu_text;

        std::set<link_state> targets;
        std::set<std::string> can_extend;

        std::optional<int> link_to_veh_app( Character *p, item &it, bool to_ports ) const;
        std::optional<int> link_tow_cable( Character *p, item &it, bool to_towing ) const;
        std::optional<int> link_extend_cable( Character *p, item &it, map *here,
                                              const tripoint_bub_ms &pnt ) const;
        std::optional<int> remove_extensions( Character *p, item &it ) const;

        link_up_actor() : iuse_actor( "link_up" ) {}

        ~link_up_actor() override = default;
        void load( const JsonObject &jo, const std::string & ) override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
};

class deploy_tent_actor : public iuse_actor
{
    public:
        string_id<furn_t> wall;
        string_id<furn_t> floor;
        std::optional<string_id<furn_t>> floor_center;
        string_id<furn_t> door_opened;
        string_id<furn_t> door_closed;
        int radius = 1;
        std::optional<itype_id> broken_type;

        deploy_tent_actor() : iuse_actor( "deploy_tent" ) {}

        ~deploy_tent_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *, item &, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        bool check_intact( const tripoint_bub_ms &center ) const;
};




class weigh_self_actor : public iuse_actor
{
    public:

        units::mass max_weight;

        explicit weigh_self_actor( const std::string &type = "weigh_self" ) : iuse_actor( type ) {}

        ~weigh_self_actor() override = default;
        void load( const JsonObject &jo, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &, map *, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};




class sew_advanced_actor : public iuse_actor
{
    public:

        std::set<material_id> materials;

        std::vector<clothing_mod_id> clothing_mods;

        skill_id used_skill;

        explicit sew_advanced_actor( const std::string &type = "sew_advanced" ) : iuse_actor( type ) {}

        ~sew_advanced_actor() override = default;
        void load( const JsonObject &obj, const std::string & ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here, const tripoint_bub_ms & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};




class effect_on_conditions_actor : public iuse_actor
{
    public:
        std::vector<effect_on_condition_id> eocs;
        translation description;
        translation menu_text;

        bool need_worn = false;

        bool need_wielding = false;
        explicit effect_on_conditions_actor( const std::string &type = "effect_on_conditions" ) :
            iuse_actor(
                type ) {}
        bool consume;

        ~effect_on_conditions_actor() override = default;
        void load( const JsonObject &obj, const std::string &src ) override;
        using iuse_actor::use;
        std::optional<int> use( Character *p, item &it, map *here,
                                const tripoint_bub_ms &pos ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};
#endif
