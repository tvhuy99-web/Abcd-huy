#pragma once
#ifndef CATA_SRC_MTYPE_H
#define CATA_SRC_MTYPE_H

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "behavior.h"
#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "enum_bitset.h"
#include "magic.h"
#include "mattack_common.h"
#include "pathfinding.h"
#include "shearing.h"
#include "translation.h"
#include "type_id.h"
#include "units.h" // IWYU pragma: keep
#include "weakpoint.h"

class Creature;
class JsonValue;
class monster;
enum class creature_size : int;
enum class phase_id : int;

struct const_dialogue;
struct dealt_projectile_attack;
template <typename E> struct enum_traits;

using mon_action_death  = void ( * )( monster & );
using mon_action_attack = bool ( * )( monster * );
using mon_action_defend = void ( * )( monster &, Creature *, dealt_projectile_attack const * );
using bodytype_id = std::string;
class JsonObject;



enum class mon_trigger : int {
    STALK,
    HOSTILE_WEAK,
    HOSTILE_CLOSE,
    HOSTILE_SEEN,
    HURT,
    FIRE,
    FRIEND_DIED,
    FRIEND_ATTACKED,
    SOUND,
    PLAYER_NEAR_BABY,
    MATING_SEASON,
    BRIGHT_LIGHT,

    LAST
};

template<>
struct enum_traits<mon_trigger> {
    static constexpr mon_trigger last = mon_trigger::LAST;
};

struct mon_flag {
    mon_flag_str_id id = mon_flag_str_id::NULL_ID();
    bool was_loaded = false;

    void load( const JsonObject &jo, std::string_view src );
    static void load_mon_flags( const JsonObject &jo, const std::string &src );
    static void finalize_all();
    static void reset();
    static const std::vector<mon_flag> &get_all();
};




// NOLINTNEXTLINE(cata-static-int_id-constants)
extern mon_flag_id mon_flag_ACIDPROOF,
       mon_flag_ACIDTRAIL,
       mon_flag_ACID_BLOOD,
       mon_flag_ALL_SEEING,
       mon_flag_ALWAYS_SEES_YOU,
       mon_flag_ALWAYS_VISIBLE,
       mon_flag_ANIMAL,
       mon_flag_APPEARS_NEUTRAL,
       mon_flag_AQUATIC,
       mon_flag_ARTHROPOD_BLOOD,
       mon_flag_ATTACKMON,
       mon_flag_ATTACK_LOWER,
       mon_flag_ATTACK_UPPER,
       mon_flag_BADVENOM,
       mon_flag_BASHES,
       mon_flag_BILE_BLOOD,
       mon_flag_BORES,
       mon_flag_CAMOUFLAGE,
       mon_flag_CANPLAY,
       mon_flag_CAN_BE_CULLED,
       mon_flag_CAN_DIG,
       mon_flag_CAN_OPEN_DOORS,
       mon_flag_CLIMBS,
       mon_flag_CLUMSY_ATTACKS,
       mon_flag_COMBAT_MOUNT,
       mon_flag_CONSOLE_DESPAWN,
       mon_flag_CONVERSATION,
       mon_flag_COPY_SUMMONER_LOOK,
       mon_flag_COPY_AVATAR_LOOK,
       mon_flag_CORNERED_FIGHTER,
       mon_flag_DEADLY_VIRUS,
       mon_flag_DESTROYS,
       mon_flag_DIGS,
       mon_flag_DOGFOOD,
       mon_flag_DORMANT,
       mon_flag_DRACULIN_IMMUNE,
       mon_flag_GEN_DORMANT,
       mon_flag_DRIPS_GASOLINE,
       mon_flag_DRIPS_NAPALM,
       mon_flag_DROPS_AMMO,
       mon_flag_EATS,
       mon_flag_ELECTRIC,
       mon_flag_ELECTRIC_FIELD,
       mon_flag_ELECTRONIC,
       mon_flag_FAE_CREATURE,
       mon_flag_FILTHY,
       mon_flag_FIREPROOF,
       mon_flag_FIREY,
       mon_flag_FISHABLE,
       mon_flag_FLASHBANGPROOF,
       mon_flag_FLIES,
       mon_flag_GOODHEARING,
       mon_flag_GRABS,
       mon_flag_GROUP_BASH,
       mon_flag_GROUP_MORALE,
       mon_flag_GUILT_ANIMAL,
       mon_flag_GUILT_CHILD,
       mon_flag_GUILT_HUMAN,
       mon_flag_GUILT_OTHERS,
       mon_flag_HARDTOSHOOT,
       mon_flag_HAS_MIND,
       mon_flag_HEARS,
       mon_flag_HIT_AND_RUN,
       mon_flag_HUMAN,
       mon_flag_ID_CARD_DESPAWN,
       mon_flag_IMMOBILE,
       mon_flag_INFRARED_VISION,
       mon_flag_INSECTICIDEPROOF,
       mon_flag_INTERIOR_AMMO,
       mon_flag_KEENNOSE,
       mon_flag_KEEP_DISTANCE,
       mon_flag_LOCKS_ON,
       mon_flag_LOUDMOVES,
       mon_flag_MECH_DEFENSIVE,
       mon_flag_MECH_RECON_VISION,
       mon_flag_MILKABLE,
       mon_flag_MIND_SEEING,
       mon_flag_NEMESIS,
       mon_flag_NEVER_WANDER,
       mon_flag_NIGHT_INVISIBILITY,
       mon_flag_NOGIB,
       mon_flag_NOHEAD,
       mon_flag_NOT_HALLUCINATION,
       mon_flag_NO_BREATHE,
       mon_flag_NO_BREED,
       mon_flag_NO_FUNG_DMG,
       mon_flag_NO_NECRO,
       mon_flag_ONE_DIMENSIONAL_X,
       mon_flag_ONE_DIMENSIONAL_Y,
       mon_flag_ONE_DIMENSIONAL_Z,
       mon_flag_PACIFIST,
       mon_flag_PARALYZEVENOM,
       mon_flag_PATH_AVOID_DANGER,
       mon_flag_PATH_AVOID_FALL,
       mon_flag_PATH_AVOID_FIRE,
       mon_flag_PAY_BOT,
       mon_flag_PERMANENT_INVISIBILITY,
       mon_flag_PET_HARNESSABLE,
       mon_flag_PET_MOUNTABLE,
       mon_flag_PET_WONT_FOLLOW,
       mon_flag_PHOTOPHOBIC,
       mon_flag_PLANT_BLOOD,
       mon_flag_PLASTIC,
       mon_flag_POISON,
       mon_flag_PRIORITIZE_TARGETS,
       mon_flag_PULP_PRYING,
       mon_flag_PUSH_MON,
       mon_flag_PUSH_VEH,
       mon_flag_QUEEN,
       mon_flag_QUIETDEATH,
       mon_flag_QUIETMOVES,
       mon_flag_RANGED_ATTACKER,
       mon_flag_REVIVES,
       mon_flag_REVIVES_HEALTHY,
       mon_flag_RIDEABLE_MECH,
       mon_flag_SEES,
       mon_flag_SHORTACIDTRAIL,
       mon_flag_SILENT_DISAPPEAR,
       mon_flag_SILENTMOVES,
       mon_flag_SLUDGEPROOF,
       mon_flag_SLUDGETRAIL,
       mon_flag_SMALLSLUDGETRAIL,
       mon_flag_SMALL_HIDER,
       mon_flag_SMELLS,
       mon_flag_STUMBLES,
       mon_flag_STUN_IMMUNE,
       mon_flag_SUNDEATH,
       mon_flag_SWARMS,
       mon_flag_SWIMS,
       mon_flag_TEEP_IMMUNE,
       mon_flag_TRUESIGHT,
       mon_flag_UNBREAKABLE_MORALE,
       mon_flag_VAMP_VIRUS,
       mon_flag_VENOM,
       mon_flag_WARM,
       mon_flag_WATER_CAMOUFLAGE,
       mon_flag_WEBWALK,
       mon_flag_WIELDED_WEAPON;


void set_mon_flag_ids();


struct mon_effect_data {

    efftype_id id;

    float chance;

    bool permanent;
    bool affect_hit_bp;
    bodypart_str_id bp;

    std::pair<int, int> duration;

    std::pair<int, int> intensity;

    std::string message;

    mon_effect_data();
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo ) {
        load( jo );
    }
};


struct pet_food_data {
    std::set<std::string> food;
    std::string pet;
    std::string feed;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

struct monster_block_data {
    int chance = 0;
    int effectiveness = 0;
    int count = 0;
    bool ranged = false;
};


struct move_skills_data {

    const static int max_movemod_penalty = 10;
    std::optional<int> climb;
    std::optional<int> dig;
    std::optional<int> swim;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
    bool handle_extend( const JsonValue &jv );
    bool handle_delete( const JsonValue &jv );
};

enum class mdeath_type {
    NORMAL,
    SPLATTER,
    BROKEN,
    NO_CORPSE,
    LAST
};

template<>
struct enum_traits<mdeath_type> {
    static constexpr mdeath_type last = mdeath_type::LAST;
};

struct monster_death_effect {
    bool was_loaded = false;
    bool has_effect = false;
    fake_spell sp;
    std::optional<effect_on_condition_id> eoc;
    translation death_message;
    mdeath_type corpse_type = mdeath_type::NORMAL;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

struct mount_item_data {



    itype_id tied;



    itype_id tack;



    itype_id armor;



    itype_id storage;

    void deserialize( const JsonObject &jo );
};

struct reproduction_type {
    mtype_id baby_monster = mtype_id::NULL_ID();
    mongroup_id baby_monster_group = mongroup_id::NULL_ID();
    itype_id baby_egg = itype_id::NULL_ID();
    item_group_id baby_egg_group = item_group_id::NULL_ID();
};

struct revive_type {
    std::function<bool( const_dialogue const & )> condition;
    mtype_id revive_mon = mtype_id::NULL_ID();
    mongroup_id revive_monster_group = mongroup_id::NULL_ID();

    void deserialize( const JsonObject &jo );
};

struct mtype {
    private:
        friend class MonsterGenerator;

        enum_bitset<mon_trigger> anger;
        enum_bitset<mon_trigger> fear;
        enum_bitset<mon_trigger> placate;
    public:
        units::mass weight;


        mon_action_defend sp_defense;
    private:
        ascii_art_id picture_id;
        mon_flag_id_set flags;
        std::set<mon_flag_str_id> pre_flags_;
    public:
        mtype_id id;
        mfaction_str_id default_faction;
        harvest_id harvest;
        harvest_id dissect;
        harvest_id decay;
        speed_description_id speed_desc;

        mtype_id upgrade_into;
        mongroup_id upgrade_group;
        mtype_id burn_into;

        std::vector<revive_type> revive_types;

        mtype_id zombify_into;
        mtype_id fungalize_into;

        reproduction_type baby_type;


        itype_id biosig_item;





        itype_id revert_to_itype;





        itype_id broken_itype;



        itype_id mech_weapon;



        itype_id mech_battery;


        std::vector<mon_effect_data> atk_effs;


        std::vector<std::pair<mtype_id, mod_id>> src;
        weakpoint_families families;
    private:
        std::vector<weakpoints_id> weakpoints_deferred;
    public:


        std::vector<material_id> absorb_material;

        std::vector<material_id> no_absorb_material;
        damage_instance melee_damage;
        std::vector<std::string> special_attacks_names;
        std::vector<std::string> chat_topics;
        std::vector<std::string> baby_flags;

        std::string sym;

        std::string looks_like;
        bodytype_id bodytype;
        shearing_data shearing;

        std::map<itype_id, int> starting_ammo;

        item_group_id death_drops;

        std::set<species_id> species;
        std::set<std::string> categories;
        std::map<material_id, int> mat;

        std::map<efftype_id, int> regeneration_modifiers;

        std::set<scenttype_id> scents_tracked;
        std::set<scenttype_id> scents_ignored;

        resistances armor;

        std::set<trap_str_id> trap_avoids;
    private:
        std::set<std::string> weakpoints_deferred_deleted;
    public:

        std::map<std::string, mtype_special_attack> special_attacks;

        std::map<emit_id, time_duration> emit_fields;
        std::optional<resistances> armor_proportional;
        std::optional<resistances> armor_relative;


        mount_item_data mount_items;
    private:

        translation name;
        translation description;
    public:


        pet_food_data petfood;
    private:

        behavior::node_t goals;

    public:
        monster_death_effect mdeath_effect;
        ::weakpoints weakpoints;

    private:
        ::weakpoints weakpoints_deferred_inline;

        int difficulty = 0;

        int difficulty_adjustment = 0;
    public:
        int mat_portion_total = 0;
        units::volume volume;
        creature_size size;
        phase_id phase;


        int get_total_difficulty() const;

        int get_difficulty_adjustment() const;

        int hp = 0;
        int speed = 0;
        move_skills_data move_skills;

        int agro = 0;
        int morale = 0;


        int tracking_distance = 8;


        int regenerates = 0;

        float mountable_weight_ratio = 0.2f;

        int attack_cost = 100;
        int melee_skill = 0;
        int melee_dice = 0;
        int melee_sides = 0;
        int melee_dice_ap = 0;

        monster_block_data
        block;

        int grab_strength = 1;
        int sk_dodge = 0;


        float status_chance_multiplier = 1.0f;


        int bleed_rate = 100;


        int absorb_ml_per_hp = 250;


        int split_move_cost = 200;


        float absorb_move_cost_per_ml = 0.025f;


        int absorb_move_cost_min = 1;

        int absorb_move_cost_max = -1;

        float luminance = 0;

        int vision_day = 40;
        int vision_night = 1;

        unsigned int def_chance;

        std::map<damage_type_id, int> bash_skill;


        int half_life;
        int age_grow;


        int baby_count;




        int mech_str_bonus = 0;


        int melee_training_cap;

        nc_color color = c_white;

        std::optional<int> upgrade_multi_range;


        std::optional<time_duration> baby_timer;


        std::optional<time_duration> biosig_timer;

        pathfinding_settings path_settings;




        bool regenerates_in_dark = false;

        bool regen_morale = false;
        bool upgrade_null_despawn;


        bool was_loaded = false;
        bool upgrades;
        bool reproduces;
        bool biosignatures;


        bool aggro_character = true;

        mtype();





        bool same_species( const mtype &other ) const;


        std::string nname( unsigned int quantity = 1 ) const;
        bool has_special_attack( const std::string &attack_name ) const;
        bool has_flag( const mon_flag_id &flag ) const {
            return flags.contains( flag );
        }
        void set_flag( const mon_flag_id &flag, bool state = true );
        bool made_of( const material_id &material ) const;
        bool made_of_any( const std::set<material_id> &materials ) const;
        bool has_anger_trigger( mon_trigger trigger ) const;
        bool has_fear_trigger( mon_trigger trigger ) const;
        bool has_placate_trigger( mon_trigger trigger ) const;
        bool in_category( const std::string &category ) const;
        bool in_species( const species_id &spec ) const;
        std::vector<std::string> species_descriptions() const;
        field_type_id get_bleed_type() const;

        field_type_id bloodType() const;
        field_type_id gibType() const;


        itype_id get_meat_itype() const;
        int get_meat_chunks_count() const;
        std::string get_description() const;
        ascii_art_id get_picture_id() const;
        std::string get_footsteps() const;
        void set_strategy();
        void add_goal( const std::string &goal_id );
        const behavior::node_t *get_goals() const;
        std::string get_difficulty_description() const;
        std::string get_size_name() const;


        void load( const JsonObject &jo, std::string_view src );
};

#endif
