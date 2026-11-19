#pragma once
#ifndef CATA_TESTS_NPC_TURN_HELPERS_H
#define CATA_TESTS_NPC_TURN_HELPERS_H

#include <functional>

class npc;


void run_npc_turn( npc &guy );


int run_npc_turns( npc &guy, int max_turns, const std::function<bool( const npc & )> &done = {} );

#endif
