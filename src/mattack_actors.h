#pragma once
#ifndef CATA_SRC_MATTACK_ACTORS_H
#define CATA_SRC_MATTACK_ACTORS_H

#include <climits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "coords_fwd.h"
#include "damage.h"
#include "magic.h"
#include "mattack_common.h"
#include "mtype.h"
#include "translation.h"
#include "type_id.h"
#include "weighted_list.h"

class Creature;
class JsonObject;
class monster;

enum class bp_type;

class invalid_mattack_actor : public mattack_actor
{
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
        void load_internal( const JsonObject &, const std::string & ) override {}
};

class leap_actor : public mattack_actor
{
    public:
        float max_range = 0.0f;

        float min_range = 0.0f;

        bool allow_no_target = false;

        bool prefer_leap = false;

        bool random_leap = false;


        bool ignore_dest_terrain = false;


        bool ignore_dest_danger = false;
        int move_cost = 0;

        float min_consider_range = 0.0f;

        float max_consider_range = 0.0f;

        std::vector<mon_effect_data> self_effects;

        translation message;

        leap_actor() = default;
        ~leap_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class mon_spellcasting_actor : public mattack_actor
{
    public:
        fake_spell spell_data;

        bool allow_no_target = false;

        mon_spellcasting_actor() = default;
        ~mon_spellcasting_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class mon_eoc_actor : public mattack_actor
{
    public:
        int range = 1;
        bool allow_no_target = false;
        std::vector<effect_on_condition_id> eoc;

        mon_eoc_actor() = default;
        ~mon_eoc_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;

};

struct grab {

    int grab_strength;

    int pull_chance;

    float pull_weight_ratio;


    efftype_id grab_effect;

    bool exclusive_grab = false;

    bool respect_seatbelts = true;

    int drag_distance;

    int drag_deviation;

    int drag_grab_break_distance;

    float drag_movecost_mod;

    translation pull_msg_u;
    translation pull_fail_msg_u;
    translation pull_msg_npc;
    translation pull_fail_msg_npc;
    void load_grab( const JsonObject &jo );
    bool was_loaded = false;
};

class melee_actor : public mattack_actor
{
    public:

        damage_instance damage_max_instance;

        float min_mul = 0.5f;

        float max_mul = 1.0f;

        int move_cost = 100;


        int accuracy = INT_MIN;

        int range = 1;

        bool no_adjacent = false;

        bool dodgeable = true;

        bool uncanny_dodgeable = true;

        bool blockable = true;

        bool effects_require_dmg = true;

        bool effects_require_organic = false;

        int throw_strength = 0;

        int hitsize_min = -1;
        int hitsize_max = -1;
        bool attack_upper = true;
        grab grab_data;
        bool is_grab = false;
        std::pair<int, int> attack_amount = {1, 1};
        bool spread_damage = false;

        std::vector<effect_on_condition_id> eoc;






        weighted_int_list<bp_type> body_parts_types;


        std::vector<mon_effect_data> effects;

        std::vector<mon_effect_data> self_effects_always;
        std::vector<mon_effect_data> self_effects_onhit;
        std::vector<mon_effect_data> self_effects_ondmg;


        translation miss_msg_u;

        translation no_dmg_msg_u;

        translation hit_dmg_u;

        translation throw_msg_u;


        translation miss_msg_npc;

        translation no_dmg_msg_npc;

        translation hit_dmg_npc;

        translation throw_msg_npc;

        melee_actor() = default;
        ~melee_actor() override = default;

        virtual Creature *find_target( monster &z ) const;
        virtual void on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const;

        void load_internal( const JsonObject &obj, const std::string &src ) override;


        int do_grab( monster &, Creature *target, bodypart_id bp_id ) const;

        std::vector<bodypart_id> pick_bodyparts_hit( const Creature *target, int hitspread ) const;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class bite_actor : public melee_actor
{
    public:


        int infection_chance = 0;

        bite_actor();
        ~bite_actor() override = default;

        void on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const override;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class gun_actor : public mattack_actor
{
    public:

        itype_id gun_type;


        itype_id ammo_type = itype_id::NULL_ID();



        std::map<skill_id, int> fake_skills;
        int fake_str = 16;
        int fake_dex = 8;
        int fake_int = 8;
        int fake_per = 12;



        std::map<std::pair<int, int>, gun_mode_id> ranges;

        int max_ammo = INT_MAX;


        translation description;


        translation failure_msg;


        translation no_ammo_sound;


        int move_cost = 150;


        bool target_moving_vehicles = false;



        int targeting_cost = 100;

        bool require_targeting_player = true;
        bool require_targeting_npc = false;
        bool require_targeting_monster = false;

        int targeting_timeout = 8;
        int targeting_timeout_extend = 3;

        translation targeting_sound;
        int targeting_volume = 6;

        bool laser_lock = false;



        bool require_sunlight = false;

        bool try_target( monster &z, Creature &target ) const;
        bool shoot( monster &z, const tripoint_bub_ms &target, const gun_mode_id &mode,
                    int inital_recoil = 0 ) const;
        int get_max_range() const;

        gun_actor();
        ~gun_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class polymorph_special : public mattack_actor
{
    public:
        mtype_id mon_id;
        bool keep_speed = true;
        bool keep_hp = true;
        bool keep_anger = true;

        polymorph_special() = default;
        ~polymorph_special() override = default;

        void load_internal( const JsonObject &jo, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

#endif
