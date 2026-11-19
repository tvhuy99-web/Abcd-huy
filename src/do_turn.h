#pragma once
#ifndef CATA_SRC_DO_TURN_H
#define CATA_SRC_DO_TURN_H

class map;
class npc;

void handle_key_blocking_activity();
void close_blocking_activity_reader();

struct npc_turn_result {

    bool spun_out = false;
};


npc_turn_result process_npc_turn( map &m, npc &guy );

#endif
