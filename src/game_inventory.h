#pragma once
#ifndef CATA_SRC_GAME_INVENTORY_H
#define CATA_SRC_GAME_INVENTORY_H

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "cata_imgui.h"
#include "coordinates.h"
#include "input_context.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_location.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class repair_item_actor;
class salvage_actor;
enum efile_action : int;

using item_filter = std::function<bool( const item & )>;
using item_location_filter = std::function<bool ( const item_location & )>;

namespace Pickup
{
struct pick_info;
}





class inventory_filter_preset : public inventory_selector_preset
{
    public:
        explicit inventory_filter_preset( const item_location_filter &filter );

        bool is_shown( const item_location &location ) const override;

    private:
        item_location_filter filter;
};
namespace game_menus
{

namespace inv
{

item_location titled_menu( Character &you, const std::string &title,
                           const std::string &none_message = "" );

item_location titled_filter_menu( const item_filter &filter, Character &you,
                                  const std::string &title, int radius = -1, const std::string &none_message = "" );
item_location titled_filter_menu( const item_location_filter &filter, Character &you,
                                  const std::string &title, int radius = -1, const std::string &none_message = "" );
drop_locations titled_multi_filter_menu( const item_location_filter &filter, Character &you,
        const std::string &title, int radius = -1, const std::string &none_message = "" );












void common();
void common( item_location &loc );
void compare( const std::optional<tripoint_rel_ms> &offset );
void reassign_letter( item &it );
void swap_letters();







class compare_item_menu : public cataimgui::window
{
    public:
        compare_item_menu( const item &first, const item &second, const std::string &confirm_message = "" );
        bool show();

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;

    private:
        item first;
        item second;
        const std::string confirm_message;
        input_context ctxt;
        std::vector<iteminfo> first_info;
        std::vector<iteminfo> second_info;
        cataimgui::scroll s = cataimgui::scroll::none;
};
bool compare_items( const item &first, const item &second,
                    const std::string &confirm_message = "" );





drop_locations multidrop( Character &you );






drop_locations pickup( const std::set<tripoint_bub_ms> &targets = {} );
drop_locations pickup( const std::set<tripoint_bub_ms> &targets,
                       const std::vector<drop_location> &selection );
drop_locations pickup( const std::set<tripoint_bub_ms> &targets,
                       const std::vector<drop_location> &selection,
                       Pickup::pick_info &info );

drop_locations smoke_food( Character &you, units::volume total_capacity,
                           units::volume used_capacity );





item_location consume( const std::string &comestible_type_filter = std::string(),
                       const item_location &loc = item_location() );


item_location container_for( Character &you, const item &liquid, int radius = 0,
                             const item *avoid = nullptr );

item_location disassemble( Character &you );

item_location gun_to_modify( Character &you, const item &gunmod );

item_location gunmod_to_remove( Character &you, item &gun );

item_location read( Character &you );

item_location ereader_to_use( Character &you );

item_location ebookread( Character &you, item_location &ereader );

drop_locations ebooksave( Character &who, item_location &ereader );





drop_locations edevice_select( Character &who, item_location &used_edevice, bool browse_equals,
                               bool auto_include_used_edevice, bool unusable_only, efile_action action );

drop_locations efile_select( Character &who, item_location &used_edevice,
                             const std::vector<item_location> &target_edevices, efile_action action, bool from_used_edevice );

item_location steal( Character &victim );

item_location use();

item_location wield();

drop_locations holster( const item_location &holster );
void insert_items( item_location &holster );
drop_locations unload_container();

item_location saw_barrel( Character &you, item &tool );

item_location saw_stock( Character &you, item &tool );

item_location pick_target_practice_gun( Character &you );

item_location molle_attach( Character &you, item &tool );

item_location veh_tool_attach( Character &you, const std::string &vp_name,
                               const std::set<itype_id> &allowed_types );

item_location wear( Character &you, const bodypart_id &bp = bodypart_str_id::NULL_ID() );

item_location take_off();

item_location salvage( Character &you, const salvage_actor *actor );

item_location repair( Character &you, const repair_item_actor *actor, const item *main_tool );

item_location install_bionic( Character &installer, Character &patron, Character &patient,
                              bool surgeon = false );

item_location sterilize_cbm( Character &you );

item_location change_sprite( Character &you );

std::pair<item_location, bool> unload( Character &you );
item::reload_option select_ammo( Character &you, const item_location &loc, bool prompt = false,
                                 bool empty = true );


}

}

#endif
