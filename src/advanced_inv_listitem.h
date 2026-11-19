#pragma once
#ifndef CATA_SRC_ADVANCED_INV_LISTITEM_H
#define CATA_SRC_ADVANCED_INV_LISTITEM_H

#include <string>
#include <vector>

#include "item_location.h"
#include "type_id.h"
#include "units.h"

class item_category;

enum aim_location : char;





class advanced_inv_listitem
{
    public:



        int idx = 0;



        aim_location area;

        itype_id id;

        std::vector<item_location> items;



        std::string name;



        std::string name_without_prefix;
        unsigned int contents_count;



        bool autopickup = false;




        int stacks = 0;



        units::volume volume;



        units::mass weight = 0_gram;



        const item_category *cat;



        bool from_vehicle = false;








        advanced_inv_listitem( const item_location &an_item, int index, int count,
                               aim_location area, bool from_vehicle );







        advanced_inv_listitem( const std::vector<item_location> &list, int index,
                               aim_location area, bool from_vehicle );
};
#endif
