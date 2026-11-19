#pragma once
#ifndef CATA_SRC_HANDLE_LIQUID_H
#define CATA_SRC_HANDLE_LIQUID_H

#include "coordinates.h"
#include "item_location.h"
#include "point.h"

class Character;
class item;
class monster;
class vehicle;

enum liquid_dest : int {
    LD_NULL,
    LD_CONSUME,
    LD_ITEM,
    LD_VEH,
    LD_KEG,
    LD_GROUND
};

struct liquid_dest_opt {
    liquid_dest dest_opt = LD_NULL;
    tripoint_bub_ms pos;
    item_location item_loc;
    vehicle *veh = nullptr;
};


namespace liquid_handler
{












void handle_all_liquid( item liquid, int radius, const item *avoid = nullptr );







void handle_npc_liquid( item liquid, Character &who );


void handle_all_or_npc_liquid( Character &p, item &newit, int radius = 0,
                               const item *avoid = nullptr );











bool consume_liquid( item &liquid, int radius = 0, const item *avoid = nullptr );









bool handle_all_liquids_from_container( item_location &container, int radius = 0 );

bool can_handle_liquid( const item &liquid );





















bool handle_liquid( item &liquid, liquid_dest_opt &liquid_target, const item *source = nullptr,
                    int radius = 0,
                    const tripoint_bub_ms *source_pos = nullptr,
                    const vehicle *source_veh = nullptr, int part_num = -1,
                    const monster *source_mon = nullptr, bool silent = true );
bool handle_liquid( item_location &liquid, const item *source = nullptr, int radius = 0 );


bool perform_liquid_transfer( item &liquid, const tripoint_bub_ms *source_pos,
                              const vehicle *source_veh, int part_num,
                              const monster *, liquid_dest_opt &target, bool silent );
bool perform_liquid_transfer( item_location &liquid, liquid_dest_opt &target );


liquid_dest_opt select_liquid_target( item &liquid, int radius );

}

#endif
