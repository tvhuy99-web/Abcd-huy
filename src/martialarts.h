#pragma once
#ifndef CATA_SRC_MARTIALARTS_H
#define CATA_SRC_MARTIALARTS_H

#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bonuses.h"
#include "calendar.h"
#include "flat_set.h"
#include "translation.h"
#include "type_id.h"
#include "uilist.h"

class Character;
class JsonObject;
class effect;
class item;
class item_location;
struct const_dialogue;
struct itype;
template <typename T> class generic_factory;
enum class bp_type;

const matec_id tec_none( "tec_none" );

class weapon_category
{
    public:
        static void load_weapon_categories( const JsonObject &jo, const std::string &src );
        static void verify_weapon_categories();
        static void finalize_all();
        static void reset();

        void load( const JsonObject &jo, std::string_view src );
        void check() const;

        static const std::vector<weapon_category> &get_all();

        const weapon_category_id &getId() const {
            return id;
        }

        const translation &name() const {
            return name_;
        }

        const std::vector<proficiency_id> &category_proficiencies() const {
            return proficiencies_;
        }

    private:
        friend class generic_factory<weapon_category>;
        friend struct mod_tracker;

        weapon_category_id id;
        std::vector<std::pair<weapon_category_id, mod_id>> src;
        std::vector<proficiency_id> proficiencies_;
        bool was_loaded = false;

        translation name_;
};

matype_id martial_art_learned_from( const itype & );

struct attack_vector {
    attack_vector_id id;


    bool weapon = false;


    std::vector<bodypart_str_id> limbs;

    bool strict_limb_definition = false;

    std::vector<sub_bodypart_str_id> contact_area;

    std::vector<std::pair<bp_type, int>> limb_req;

    bool armor_bonus = true;

    cata::flat_set<flag_id> required_limb_flags;
    cata::flat_set<flag_id> forbidden_limb_flags;


    int encumbrance_limit = 100;

    int bp_hp_limit = 10;

    bool was_loaded = false;

    static void load_attack_vectors( const JsonObject &jo, const std::string &src );
    static void reset();
    void load( const JsonObject &jo, std::string_view );
};

struct ma_requirements {
    bool was_loaded = false;

    bool unarmed_allowed;
    bool melee_allowed;
    bool strictly_unarmed;
    bool wall_adjacent;


    std::vector<weapon_category_id> weapon_categories_allowed;


    std::vector<std::pair<skill_id, int>> min_skill;




    std::vector<std::pair<damage_type_id, int>> min_damage;

    std::set<mabuff_id> req_buffs_all;
    std::set<mabuff_id> req_buffs_any;
    std::set<mabuff_id> forbid_buffs_all;
    std::set<mabuff_id> forbid_buffs_any;

    std::set<flag_id> req_flags;
    cata::flat_set<json_character_flag> req_char_flags;
    cata::flat_set<json_character_flag> req_char_flags_all;
    cata::flat_set<json_character_flag> forbidden_char_flags;

    ma_requirements() {
        unarmed_allowed = false;
        melee_allowed = false;
        strictly_unarmed = false;
        wall_adjacent = false;
    }

    std::string get_description( bool buff = false ) const;

    bool buff_requirements_satisfied( const Character &u ) const;
    bool is_valid_character( const Character &u ) const;
    bool is_valid_weapon( const item &i ) const;

    void load( const JsonObject &jo, std::string_view src );
};

struct tech_effect_data {
    efftype_id id;
    int duration;
    bool permanent;
    bool on_damage;
    int chance;
    std::string message;

    std::function<bool( const_dialogue const & )> condition;
    std::string condition_desc;
    bool has_condition = false;

    tech_effect_data() = default;
};

class ma_technique
{
    public:
        ma_technique();

        void load( const JsonObject &jo, std::string_view src );
        static void verify_ma_techniques();
        static void finalize_all();
        void check() const;

        matec_id id;
        std::vector<std::pair<matec_id, mod_id>> src;
        bool was_loaded = false;
        translation name;

        translation description;
        std::string get_description() const;

        std::string goal;


        bool is_valid_character( const Character &u ) const;

        std::set<std::string> flags;


        translation avatar_message;
        translation npc_message;

        bool defensive = false;
        bool side_switch = false;
        bool dummy = false;
        bool crit_tec = false;
        bool crit_ok = false;
        bool reach_tec = false;
        bool reach_ok = false;

        ma_requirements reqs;


        std::vector<attack_vector_id> attack_vectors;

        int repeat_min = 1;
        int repeat_max = 1;
        int down_dur = 0;
        int stun_dur = 0;
        int knockback_dist = 0;
        float knockback_spread = 0.0f;
        std::string aoe;
        bool knockback_follow = false;


        bool disarms = false;
        bool take_weapon = false;
        bool dodge_counter = false;
        bool block_counter = false;

        bool miss_recovery = false;
        bool grab_break = false;

        int weighting = 0;


        bool wall_adjacent = false;

        bool needs_ammo = false;


        std::function<bool( const_dialogue const & )> condition;
        std::string condition_desc;
        bool has_condition = false;


        bonus_container bonuses;

        std::vector<tech_effect_data> tech_effects;

        float damage_bonus( const Character &u, const damage_type_id &type ) const;
        float damage_multiplier( const Character &u, const damage_type_id &type ) const;
        float move_cost_multiplier( const Character &u ) const;
        float move_cost_penalty( const Character &u ) const;
        float armor_penetration( const Character &u, const damage_type_id &type ) const;

        std::vector<effect_on_condition_id> eocs;
};

class ma_buff
{
    public:
        ma_buff();



        void apply_buff( Character &u ) const;


        bool is_valid_character( const Character &u ) const;


        void apply_character( Character &u ) const;


        int block_effectiveness_bonus( const Character &u ) const;
        int hit_bonus( const Character &u ) const;
        int critical_hit_chance_bonus( const Character &u ) const;
        int dodge_bonus( const Character &u ) const;
        int speed_bonus( const Character &u ) const;
        int block_bonus( const Character &u ) const;
        int arpen_bonus( const Character &u, const damage_type_id &dt ) const;


        int armor_bonus( const Character &guy, const damage_type_id &dt ) const;


        float damage_bonus( const Character &u, const damage_type_id &dt ) const;



        float damage_mult( const Character &u, const damage_type_id &dt ) const;


        bool is_melee_bash_damage_cap_bonus() const;
        bool is_quiet() const;
        bool can_melee() const;
        bool is_stealthy() const;
        bool has_flag( const json_character_flag &flag ) const;


        efftype_id get_effect_id() const;

        static const ma_buff *from_effect( const effect &eff );

        mabuff_id id;
        std::vector<std::pair<mabuff_id, mod_id>> src;
        bool was_loaded = false;
        translation name;
        translation description;
        std::string get_description( bool passive = false ) const;

        ma_requirements reqs;

        cata::flat_set<json_character_flag> flags;



        time_duration buff_duration = 0_turns;
        int max_stacks = 0;
        bool persists = false;

        int dodges_bonus = 0;
        int free_dodges = 0;
        int blocks_bonus = 0;


        bonus_container bonuses;

        bool quiet = false;
        bool melee_allowed = false;
        bool melee_bash_damage_cap_bonus = false;
        bool strictly_melee = false;
        bool stealthy = false;

        void load( const JsonObject &jo, std::string_view src );
        static void finalize_all();
};

class martialart
{
    public:
        martialart();

        static void finalize_all();

        void load( const JsonObject &jo, std::string_view src );

        void remove_all_buffs( Character &u ) const;


        void apply_static_buffs( Character &u ) const;

        void apply_onmove_buffs( Character &u ) const;

        void apply_onpause_buffs( Character &u ) const;

        void apply_onhit_buffs( Character &u ) const;

        void apply_onattack_buffs( Character &u ) const;

        void apply_ondodge_buffs( Character &u ) const;

        void apply_onblock_buffs( Character &u ) const;

        void apply_ongethit_buffs( Character &u ) const;

        void apply_onmiss_buffs( Character &u ) const;

        void apply_oncrit_buffs( Character &u ) const;

        void apply_onkill_buffs( Character &u ) const;

        void activate_eocs( Character &u, const std::vector<effect_on_condition_id> &eocs ) const;


        void apply_static_eocs( Character &u ) const;

        void apply_onmove_eocs( Character &u ) const;

        void apply_onpause_eocs( Character &u ) const;

        void apply_onhit_eocs( Character &u ) const;

        void apply_onattack_eocs( Character &u ) const;

        void apply_ondodge_eocs( Character &u ) const;

        void apply_onblock_eocs( Character &u ) const;

        void apply_ongethit_eocs( Character &u ) const;

        void apply_onmiss_eocs( Character &u ) const;

        void apply_oncrit_eocs( Character &u ) const;

        void apply_onkill_eocs( Character &u ) const;


        bool has_technique( const Character &u, const matec_id &tec_id ) const;

        bool has_weapon( const itype_id & ) const;

        bool weapon_valid( const item_location &it ) const;

        std::string get_initiate_avatar_message() const;

        std::string get_initiate_npc_message() const;

        matype_id id;
        std::vector<std::pair<matype_id, mod_id>> src;
        bool was_loaded = false;
        translation name;
        translation description;
        std::vector<translation> initiate;
        int priority = 0;
        std::vector<std::pair<std::string, int>> autolearn_skills;
        skill_id primary_skill;
        bool teachable = true;
        int learn_difficulty = 0;
        int arm_block = 0;
        int leg_block = 0;
        int nonstandard_block = 0;
        bool arm_block_with_bio_armor_arms = false;
        bool leg_block_with_bio_armor_legs = false;
        std::set<matec_id> techniques;
        std::set<itype_id> weapons;
        std::set<weapon_category_id> weapon_category;
        bool strictly_unarmed = false;
        bool strictly_melee = false;
        bool allow_all_weapons = false;
        bool force_unarmed = false;
        bool prevent_weapon_blocking = false;
        std::vector<mabuff_id> static_buffs;
        std::vector<mabuff_id> onmove_buffs;
        std::vector<mabuff_id> onpause_buffs;
        std::vector<mabuff_id> onhit_buffs;
        std::vector<mabuff_id> onattack_buffs;
        std::vector<mabuff_id> ondodge_buffs;
        std::vector<mabuff_id> onblock_buffs;
        std::vector<mabuff_id> ongethit_buffs;
        std::vector<mabuff_id> onmiss_buffs;
        std::vector<mabuff_id> oncrit_buffs;
        std::vector<mabuff_id> onkill_buffs;
        std::vector<effect_on_condition_id> static_eocs;
        std::vector<effect_on_condition_id> onmove_eocs;
        std::vector<effect_on_condition_id> onpause_eocs;
        std::vector<effect_on_condition_id> onhit_eocs;
        std::vector<effect_on_condition_id> onattack_eocs;
        std::vector<effect_on_condition_id> ondodge_eocs;
        std::vector<effect_on_condition_id> onblock_eocs;
        std::vector<effect_on_condition_id> ongethit_eocs;
        std::vector<effect_on_condition_id> onmiss_eocs;
        std::vector<effect_on_condition_id> oncrit_eocs;
        std::vector<effect_on_condition_id> onkill_eocs;
};

class ma_style_callback : public uilist_callback
{
    private:
        size_t offset;
        const std::vector<matype_id> &styles;

    public:
        ma_style_callback( int style_offset, const std::vector<matype_id> &selectable_styles )
            : offset( style_offset )
            , styles( selectable_styles )
        {}

        bool key( const input_context &ctxt, const input_event &event, int entnum, uilist *menu ) override;
        ~ma_style_callback() override = default;
};

tech_effect_data load_tech_effect_data( const JsonObject &e );
void load_technique( const JsonObject &jo, const std::string &src );
void load_martial_art( const JsonObject &jo, const std::string &src );
void check_martialarts();
void clear_techniques_and_martial_arts();
void finalize_martial_arts();
std::string martialart_difficulty( const matype_id &mstyle );

std::vector<matype_id> all_martialart_types();
std::vector<matype_id> autolearn_martialart_types();

#endif
