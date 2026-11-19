#pragma once
#ifndef CATA_SRC_ITEM_TRANSFORMATION_H
#define CATA_SRC_ITEM_TRANSFORMATION_H

#include <string>
#include <vector>

#include "calendar.h"
#include "type_id.h"

class item;
class Character;
class JsonObject;





struct item_transformation {

    itype_id target;


    item_group_id target_group;


    itype_id container;


    std::string variant_type = "<any>";


    bool sealed = true;


    int ammo_qty = -1;


    std::vector<int> random_ammo_qty;


    time_duration target_timer = 0_seconds;


    itype_id ammo_type;

    void deserialize( const JsonObject &jo );



    void transform( Character *carrier, item &it, bool dont_take_off = false ) const;
};

#endif
