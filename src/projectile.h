#pragma once
#ifndef CATA_SRC_PROJECTILE_H
#define CATA_SRC_PROJECTILE_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "compatibility.h"
#include "coordinates.h"
#include "damage.h"
#include "point.h"
#include "type_id.h"

class Creature;
class item;
struct explosion_data;

struct projectile {
        damage_instance impact;


        int speed = 0;
        int range = 0;

        int count = 1;

        bool multishot = false;

        int shot_spread = 0;

        damage_instance shot_impact;
        float critical_multiplier = 0.0f;

        std::set<ammo_effect_str_id> proj_effects;





        const item &get_drop() const;

        void set_drop( const item &it );
        void set_drop( item &&it );
        void unset_drop();

        const explosion_data &get_custom_explosion() const;
        void set_custom_explosion( const explosion_data &ex );
        void unset_custom_explosion();


        void apply_effects_damage( Creature &target, Creature *source,
                                   const dealt_damage_instance &dealt_dam,
                                   bool critical ) const;

        void apply_effects_nodamage( Creature &target, const bodypart_id &bp_hit,
                                     bool soaked_through = false ) const;

        projectile();
        projectile( const projectile & );
        projectile( projectile && ) noexcept( set_is_noexcept );
        projectile &operator=( const projectile & );
        ~projectile();

    private:


        std::unique_ptr<item> drop;
        std::unique_ptr<explosion_data> custom_explosion;
};

struct dealt_projectile_attack {
    projectile proj;
    Creature *last_hit_critter;
    dealt_damage_instance dealt_dam;
    tripoint_bub_ms end_point;
    double missed_by;
    bool headshot = false;
    bool shrapnel = false;

    std::map<Creature *, std::pair<int, int>> targets_hit;
};

void apply_ammo_effects( Creature *source, const tripoint_bub_ms &p,
                         const std::set<ammo_effect_str_id> &effects, int dealt_damage );
int max_aoe_size( const std::set<ammo_effect_str_id> &tags );

void multi_projectile_hit_message( Creature *critter, int hit_count, int damage_taken,
                                   const std::string &projectile_name );

#endif
