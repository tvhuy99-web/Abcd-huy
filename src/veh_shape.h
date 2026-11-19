#pragma once
#ifndef CATA_SRC_VEH_SHAPE_H
#define CATA_SRC_VEH_SHAPE_H

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "coordinates.h"
#include "input_context.h"
#include "vpart_position.h"

class map;
class player_activity;
class vehicle;
template <typename T> class ret_val;

class veh_shape
{
    public:
        explicit veh_shape( map &here, vehicle &vehicle );




        player_activity start( const tripoint_bub_ms &pos );

    private:
        input_context ctxt;


        vehicle &veh;


        map &here;
        std::set<tripoint_bub_ms> cursor_allowed;
        tripoint_bub_ms cursor_pos;
        tripoint_bub_ms get_cursor_pos() const;
        bool set_cursor_pos( const tripoint_bub_ms &new_pos );
        bool handle_cursor_movement( const std::string &action );


        std::vector<vpart_reference> parts_under_cursor() const;


        const bool allow_zlevel_shift = false;

        void init_input();


        std::optional<vpart_reference> select_part_at_cursor(
            const std::string &title, const std::string &extra_description,
            const std::function<ret_val<void>( const vpart_reference & )> &predicate,
            const std::optional<vpart_reference> &preselect ) const;

    public:
        void change_part_shape( vpart_reference vpr ) const;
};

#endif
