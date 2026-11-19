#pragma once
#ifndef CATA_SRC_VEH_APPLIANCE_H
#define CATA_SRC_VEH_APPLIANCE_H

#include <functional>
#include <optional>
#include <vector>

#include "coordinates.h"
#include "cursesdef.h"
#include "input_context.h"
#include "item.h"
#include "memory_fast.h"
#include "player_activity.h"
#include "point.h"
#include "type_id.h"
#include "uilist.h"

class Character;
class map;
class ui_adaptor;
class vehicle;

vpart_id vpart_appliance_from_item( const itype_id &item_id );
bool place_appliance( map &here, const tripoint_bub_ms &p, const vpart_id &vpart,
                      const Character &owner, const std::optional<item> &base = std::nullopt );















class veh_app_interact
{
    public:






        static void run( map &here, vehicle &veh, const point_rel_ms &p = point_rel_ms::zero );

    private:
        explicit veh_app_interact( vehicle &veh, const point_rel_ms &p );
        ~veh_app_interact() = default;


        point_rel_ms a_point = point_rel_ms::zero;

        vehicle *veh;



        input_context ctxt;
        weak_ptr_fast<ui_adaptor> ui;

        std::vector<std::function<void()>> app_actions;



        uilist imenu;







        void init_ui_windows( map &here );



        std::string draw_info_imgui_header( map &here );





        void populate_app_actions( map &here );



        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor( map &here );




        bool can_refill( );





        bool can_siphon();






        void refill( );






        void siphon( map &here );





        void rename();




        void remove( map &here );





        bool can_disconnect();




        void disconnect( map &here );




        void plug( map &here );




        void hide();





        void app_loop( map &here );
};

#endif
