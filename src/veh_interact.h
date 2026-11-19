#pragma once
#ifndef CATA_SRC_VEH_INTERACT_H
#define CATA_SRC_VEH_INTERACT_H

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "input_context.h"
#include "input_enums.h"
#include "item_location.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "veh_utils.h"
#include "vpart_position.h"

class Character;
class map;
class temp_crafting_inventory;
class time_duration;
class vpart_info;
struct requirement_data;


enum class task_reason : int {
    UNKNOWN_TASK = -1,
    CAN_DO,
    INVALID_TARGET,
    LACK_TOOLS,
    NOT_FREE,
    LACK_SKILL,
    MOVING_VEHICLE,
    LOW_MORALE,
    LOW_LIGHT
};

class ui_adaptor;
class vehicle;
struct vehicle_part;

template <typename T> struct enum_traits;


const std::string leak_marker = "<color_red>*</color>";


enum vehicle_action {
    VEHICLE_INSTALL,
    VEHICLE_REPAIR,
    VEHICLE_REFILL,
    VEHICLE_REMOVE,
    VEHICLE_REMOVE_APPLIANCE,
    VEHICLE_UNPLUG,
    VEHICLE_MEND_FAULTS,
    VEHICLE_SIPHON,
    VEHICLE_UNLOAD,

    VEHICLE_SHAPE,
    VEHICLE_ASSIGN_CREW,
    VEHICLE_RELABEL,
    VEHICLE_QUIT,
    VEHICLE_LAST
};

namespace io
{
template<>
std::string enum_to_string<vehicle_action>( vehicle_action stage );
}

template<>
struct enum_traits<vehicle_action> {
    static constexpr vehicle_action last = vehicle_action::VEHICLE_LAST;
};

class veh_interact
{
        using part_selector = std::function<bool( const map &here, const vehicle_part &pt )>;

    public:
        static void run( map &here,  vehicle &veh, const point_rel_ms &p );


        static std::optional<vpart_reference> select_part( map &here, const vehicle &veh,
                const part_selector &sel,
                const std::string &title = std::string() );

    private:
        explicit veh_interact( map &here, vehicle &veh, const point_rel_ms &p = point_rel_ms::zero );
        ~veh_interact();




        point_rel_ms cursor_vp_mount = point_rel_ms::zero;

        int start_at = 0;
        int start_limit = 0;

        int overview_offset = 0;
        int overview_limit = 0;

        int w_msg_scroll_offset = 0;

        int fuel_index = 0;


        item_location refill_target;

        const vehicle_part *sel_vehicle_part = nullptr;
        const vpart_info *sel_vpart_info = nullptr;


        vehicle_action sel_cmd = VEHICLE_QUIT;

        int cpart = -1;
        int page_size = 0;

        const int stats_h = 8;

        int disp_w = 26;
        int pane_w = 25;
        catacurses::window w_border;
        catacurses::window w_mode;
        catacurses::window w_msg;
        catacurses::window w_disp;
        catacurses::window w_parts;
        catacurses::window w_stats;
        catacurses::window w_stats_1;
        catacurses::window w_stats_2;
        catacurses::window w_stats_3;
        catacurses::window w_list;
        catacurses::window w_details;
        catacurses::window w_name;

        weak_ptr_fast<ui_adaptor> ui;

        std::optional<std::string> title;
        std::optional<std::string> msg;

        bool ui_hidden = false;

        int highlight_part = -1;

        struct install_info_t;

        std::unique_ptr<install_info_t> install_info;




        class reader_session;

        std::unique_ptr<reader_session> reader;




        int repair_pos = 0;

        struct remove_info_t;

        std::unique_ptr<remove_info_t> remove_info;

        vehicle *veh;
        const temp_crafting_inventory *crafting_inv;

        veh_utils::install_candidates install_options;
        input_context main_context;


        units::mass max_lift;

        units::mass max_jack;

        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor( map &here );
        void hide_ui( map &here, bool hide );

        void assign_activity( map &here );


        bool format_reqs( std::string &msg, const requirement_data &reqs,
                          const std::map<skill_id, int> &skills, time_duration time ) const;

        int part_at( const point_rel_ms &d );
        void move_cursor( map &here, const point_rel_ms &d, int dstart_at = 0 );
        task_reason cant_do( const map &here, vehicle_action mode );








        bool move_in_list( int &pos, const std::string &action, int size,
                           int header = 0 ) const;
        void move_fuel_cursor( map &here, int delta );









        void do_install( map &here );
        void do_repair( map &here );
        void do_mend( map &here );
        void do_refill( map &here );
        void do_remove( map &here );
        void do_rename();
        void do_siphon( map &here );

        bool do_unload( map &here );
        void do_assign_crew( map &here );
        void do_relabel( const map &here );







        std::pair<bool, std::string> calc_lift_requirements( map &here, const vpart_info &sel_vpart_info );

        void display_grid();
        void display_veh( map &here );
        void display_stats( map &here ) const;
        void display_name();
        void display_mode( const map &here );
        void display_list( size_t pos, const std::vector<const vpart_info *> &list, int header = 0 );
        void display_details( const vpart_info *part );

        struct part_option {
            part_option( const std::string &key, vehicle_part *part, bool selectable, const input_event &hotkey,
                         std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details ) :
                key( key ), part( part ), selectable( selectable ), hotkey( hotkey ),
                details( std::move( details ) ) {}

            part_option( const std::string &key, vehicle_part *part, bool selectable, const input_event &hotkey,
                         std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details,
                         std::function<void( const vehicle_part &pt )> message ) :
                key( key ), part( part ), selectable( selectable ), hotkey( hotkey ),
                details( std::move( details ) ),
                message( std::move( message ) ) {}

            std::string key;
            vehicle_part *part;


            bool selectable;


            input_event hotkey;


            std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details;


            std::function<int( const vehicle_part &pt )> extra_rows;


            std::function<void( const vehicle_part &pt )> message;
        };
        std::vector<part_option> overview_opts;
        std::map<std::string, std::function<void( const catacurses::window &, int )>> overview_headers;
        using overview_enable_t = std::function<bool( const map &here, const vehicle_part &pt )>;
        using overview_action_t = std::function<void( map &here, vehicle_part &pt )>;
        overview_enable_t overview_enable;
        overview_action_t overview_action;
        int overview_pos = -1;

        void calc_overview( map &here );
        void display_overview( const map &here );








        void overview( map &here, const overview_enable_t &enable = {},
                       const overview_action_t &action = {} );
        void move_overview_line( int );

        void count_durability();

        nc_color total_durability_color;
        std::string total_durability_text;


        vehicle_part *get_most_damaged_part() const;




        vehicle_part *get_most_repairable_part() const;


        bool can_remove_part( map &here, int idx, const Character &you );

        bool update_part_requirements( map &here );




        std::vector<const vpart_info *> can_mount;






        std::vector<int> need_repair;




        std::vector<int> parts_here;



        ter_t terrain_here;

        void cache_tool_availability();
        void allocate_windows();
        void do_main_loop( map &here );

        void cache_tool_availability_update_lifting( const tripoint_bub_ms &world_cursor_pos );


        bool can_self_jack( map &here );
};

void act_vehicle_siphon( map &here, vehicle *veh );

void orient_part( map &here, vehicle *veh, const vpart_info &vpinfo, int partnum,
                  const std::optional<point_rel_ms> &part_placement = std::nullopt );

#endif
