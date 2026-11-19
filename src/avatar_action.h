#pragma once
#ifndef CATA_SRC_AVATAR_ACTION_H
#define CATA_SRC_AVATAR_ACTION_H

#include <optional>
#include <string>

#include "coordinates.h"

class Character;
class avatar;
class item;
class item_location;
class map;
class turret_data;

namespace avatar_action
{


void eat( avatar &you, item_location &loc );



bool eat_here( avatar &you );
void eat_or_use( avatar &you, item_location loc );



bool move( avatar &you, map &m, const tripoint_rel_ms &d );

void swim( map &m, avatar &you, const tripoint_bub_ms &p );

void autoattack( avatar &you, map &m );

void mend( avatar &you, item_location loc );






bool can_fire_weapon( avatar &you, const map &m, const item &weapon );


void fire_wielded_weapon( avatar &you );


void fire_ranged_mutation( Character &you, const item &fake_gun );


void fire_ranged_bionic( avatar &you, const item &fake_gun );







bool fire_turret_manual( avatar &you, map &m, turret_data &turret );


void plthrow( avatar &you, item_location loc,
              const std::optional<tripoint_bub_ms> &blind_throw_from_pos = std::nullopt );


void plthrow_wielded( avatar &you,
                      const std::optional<tripoint_bub_ms> &blind_throw_from_pos = std::nullopt );





void unload( avatar &you );


void use_item( avatar &you, item_location &loc, std::string const &method = {} );
void use_item( avatar &you );


bool check_stealing( Character &who, item &weapon );
}

#endif
