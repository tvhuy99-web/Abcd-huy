#pragma once
#ifndef CATA_SRC_MONDEATH_H
#define CATA_SRC_MONDEATH_H

#include "item_location.h"

class map;
class monster;

namespace mdeath
{

item_location normal( map *here, monster &z );

item_location splatter( map *here, monster &z );

void disappear( monster &z );

void broken( map *here, monster &z );
}

item_location make_mon_corpse( map *here, monster &z, int damageLvl );

#endif
