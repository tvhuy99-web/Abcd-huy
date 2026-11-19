#pragma once
#ifndef CATA_SRC_GATES_H
#define CATA_SRC_GATES_H

#include <string>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class Character;
class Creature;
class JsonObject;
class map;

namespace gates
{

void load( const JsonObject &jo, const std::string &src );
void finalize();
void check();
void reset();


void open_gate( const tripoint_bub_ms &pos, Character &p );

void open_gate( const tripoint_bub_ms &pos );

}

namespace doors
{





bool check_mon_blocking_door( const Creature &who, const tripoint_abs_ms &p );














bool forced_door_closing( const tripoint_bub_ms &p, std::vector<tripoint_bub_ms> affected_tiles,
                          const ter_id &door_type, int bash_dmg );









bool lock_door( map &m, Creature &who, const tripoint_bub_ms &lockp );









bool unlock_door( map &m, Creature &who, const tripoint_bub_ms &lockp );




bool can_lock_door( const map &m, const Creature &who, const tripoint_bub_ms &lockp );



bool can_unlock_door( const map &m, const Creature &who, const tripoint_bub_ms &lockp );

}

#endif
