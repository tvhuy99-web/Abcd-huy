#pragma once
#ifndef CATA_SRC_ADVANCED_INV_AREA_H
#define CATA_SRC_ADVANCED_INV_AREA_H

#include <array>
#include <string>
#include <vector>

#include "coordinates.h"
#include "item_location.h"
#include "point.h"
#include "units.h"

enum aim_location : char {
    AIM_INVENTORY = 0,
    AIM_SOUTHWEST,
    AIM_SOUTH,
    AIM_SOUTHEAST,
    AIM_WEST,
    AIM_CENTER,
    AIM_EAST,
    AIM_NORTHWEST,
    AIM_NORTH,
    AIM_NORTHEAST,
    AIM_DRAGGED,
    AIM_ALL,
    AIM_CONTAINER,
    AIM_PARENT,
    AIM_WORN,
    NUM_AIM_LOCATIONS,

    AIM_WIELD,

    AIM_AROUND_BEGIN = AIM_SOUTHWEST,
    AIM_AROUND_END = AIM_NORTHEAST
};

class item;
class vehicle;
class vehicle_stack;




class advanced_inv_area
{
    public:

        using itemstack = std::vector<std::vector<item *> >;

        const aim_location id;

        point hscreen = point::zero;

        tripoint_rel_ms off;

        const std::string name = "fake";


        const std::string shortname = "FK";

        tripoint_bub_ms pos;


        bool canputitemsloc = false;

        vehicle *veh = nullptr;
        int vstor = 0;

        std::array<std::string, 2> desc;

        std::string flags;

        units::volume volume;
        units::volume volume_veh;
        units::mass weight;
        units::mass weight_veh;

        int max_size = 0;

        const std::string minimapname;

        const std::string actionname;

        const aim_location relative_location;

        // NOLINTNEXTLINE(google-explicit-constructor)
        advanced_inv_area( aim_location id ) : id( id ), relative_location( id ) {}
        advanced_inv_area(
            aim_location id, const point &hscreen, tripoint off, const std::string &name,
            const std::string &shortname, std::string minimapname, std::string actionname,
            aim_location relative_location );

        void init();

        template <typename T>
        advanced_inv_area::itemstack i_stacked( T items );
        int get_item_count() const;

        bool is_same( const advanced_inv_area &other ) const;

        bool canputitems( const item_location &container = item_location::nowhere ) const;
        void set_container_position();
        aim_location offset_to_location() const;
        bool can_store_in_vehicle() const;

        vehicle_stack get_vehicle_stack() const;
};
#endif
