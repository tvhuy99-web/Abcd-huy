#pragma once
#ifndef CATA_SRC_RANGED_H
#define CATA_SRC_RANGED_H

#include <optional>
#include <string>
#include <vector>

#include "coordinates.h"

class Character;
class aim_activity_actor;
class avatar;
class gun_mode;
class item;
class item_location;
class map;
class spell;
class turret_data;
class vehicle;
enum class creature_size : int;
struct vehicle_part;


constexpr double MIN_RECOIL_IMPROVEMENT = 0.01;

namespace target_handler
{


bool combat_precise_aim_enabled();
void toggle_combat_precise_aim();


void queue_accessible_combat_shot( int direction );
std::optional<int> take_accessible_combat_shot();
bool take_accessible_combat_precise_aim();
bool take_accessible_combat_shot_completed();


using trajectory = std::vector<tripoint_bub_ms>;


trajectory mode_select_only( avatar &you, int range );




trajectory mode_fire( avatar &you, aim_activity_actor &activity );


trajectory mode_throw( avatar &you, item &relevant, bool blind_throwing );


trajectory mode_reach( avatar &you, item_location weapon );


trajectory mode_unarmed_reach( avatar &you );


trajectory mode_turret_manual( avatar &you, turret_data &turret );


trajectory mode_turrets( avatar &you, vehicle &veh, const std::vector<vehicle_part *> &turrets );


trajectory mode_spell( avatar &you, spell &casting, bool no_fail, bool no_mana );
}

void practice_archery_proficiency( Character &p, const item &relevant );

int range_with_even_chance_of_good_hit( int dispersion );






bool gunmode_checks_common( avatar &you, const map &m, std::vector<std::string> &messages,
                            const gun_mode &gmode );






bool gunmode_checks_weapon( avatar &you, const map &m, std::vector<std::string> &messages,
                            const gun_mode &gmode );

int throw_cost( const Character &c, const item &to_throw );


double calc_steadiness( const Character &you, const item &weapon, const tripoint_bub_ms &pos,
                        double predicted_recoil );

double calculate_aim_cap( const Character &you, const tripoint_bub_ms &target );

double occupied_tile_fraction( creature_size target_size );

struct Target_attributes {
    int range = 1;
    double size = 0.5;
    double size_in_moa = 10800.0;
    float light = 0.0f;
    bool visible = true;
    explicit Target_attributes() = default;
    explicit Target_attributes( tripoint_bub_ms src, tripoint_bub_ms target );
    explicit Target_attributes( int rng, double size, float l, bool can_see );
};

#endif
