#pragma once
#ifndef CATA_SRC_DEBUG_MONITOR_TARGETS_H
#define CATA_SRC_DEBUG_MONITOR_TARGETS_H

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace debug_menu
{




struct monitor_target_entry {

    const char *label;


    const char *hint;



    bool ( *validate )( std::string_view );



    std::function<std::string()> ( *make_snap )( std::string_view input );
};

const std::vector<monitor_target_entry> &all_monitor_targets();

}

#endif
