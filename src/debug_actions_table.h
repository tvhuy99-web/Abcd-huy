#pragma once
#ifndef CATA_SRC_DEBUG_ACTIONS_TABLE_H
#define CATA_SRC_DEBUG_ACTIONS_TABLE_H

#include <vector>

#include "debug_menu_types.h"

namespace debug_menu
{

struct debug_action_entry {
    debug_menu_index id;
    const char *label;
    const char *search_keys;
    const char *category;



    void ( *execute )();


    const char *tooltip = nullptr;



    bool jump_only = false;
};

const std::vector<debug_action_entry> &all_actions();


const debug_action_entry *find_action( debug_menu_index id );

}

#endif
