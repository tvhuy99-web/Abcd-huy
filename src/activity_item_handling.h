#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_handlers.h"
#include "character.h"
#include "coordinates.h"
#include "point.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "requirements.h"
#include "type_id.h"

class item_location;
class player_activity;
class vpart_reference;
class zone_data;
class zone_manager;
enum zone_activity_stage : int;

struct requirement_failure_reasons {
    void convert_requirement_check_result( requirement_check_result req_res );
    bool check_skip_location() const;

    bool no_path = false;
    bool skip_location_no_zone = false;
    bool skip_location_blocking = false;
    bool skip_location_no_skill = false;
    bool skip_location_unknown_activity = false;
    bool skip_location_no_location = false;
    bool skip_location_no_match = false;
    bool skip_location = false;
    bool no_craft_disassembly_location_route_found = false;
};

namespace zone_sorting
{

using zone_items = std::vector<std::pair<item *, bool>>;

struct unload_sort_options {
    bool unload_mods = false;
    bool unload_molle = false;
    bool unload_always = false;
    bool ignore_favorite = false;
    bool unload_all = false;
    bool unload_corpses = false;
    bool unload_sparse_only = false;
    int unload_sparse_threshold = 0;
};

template <typename T>
bool sorter_out_of_bounds( Character &you, const T &act )
{
    const map &here = get_map();
    if( !here.inbounds( you.pos_bub() ) ) {


        you.cancel_activity();
        you.assign_activity( act );
        you.set_moves( 0 );
        g->reload_npcs();
        return true;
    }
    return false;
}


bool route_to_destination( Character &you, player_activity &act,
                           const tripoint_bub_ms &dest, zone_activity_stage &stage );



bool sort_skip_item( Character &you, const item *it,
                     const std::vector<item_location> &other_activity_items,
                     bool ignore_favorite, const tripoint_abs_ms &src,
                     bool *spillable_skipped = nullptr );






unload_sort_options set_unload_options( Character &you, const tripoint_abs_ms &src,
                                        bool use_zone_type );



zone_items populate_items( const tripoint_bub_ms &src_bub );


bool ignore_contents( Character &you, const tripoint_abs_ms &src );




bool ignore_zone_position( Character &you, const tripoint_abs_ms &src, bool ignore_contents );




bool has_items_to_sort( Character &you, const tripoint_abs_ms &src,
                        zone_sorting::unload_sort_options zone_unload_options,
                        const std::vector<item_location> &other_activity_items,
                        const zone_sorting::zone_items &items, bool *pickup_failure,
                        bool *spillable_skipped = nullptr );
bool dest_has_capacity( const tripoint_abs_ms &dest, const zone_type_id &ztype,
                        const item &sample, const faction_id &fac );



enum class carry_gate_result : int {
    ok = 0,

    no_space,

    knockdown,

    over_capacity,
};




carry_gate_result carry_gate_check( Character &you, const item &it, bool have_staged_items );
bool can_unload( item *it );
void add_item( const std::optional<vpart_reference> &vp, const tripoint_bub_ms &src_bub,
               const item &it );
void remove_item( const std::optional<vpart_reference> &vp, const tripoint_bub_ms &src_bub,
                  item *it );
std::optional<bool> unload_item( Character &you, const tripoint_abs_ms &src,
                                 zone_sorting::unload_sort_options zone_unload_options,
                                 const std::optional<vpart_reference> &vpr_src,
                                 item_location it, const std::unordered_set<tripoint_abs_ms> &dest_set,
                                 int &num_processed );
void move_item( Character &you, const std::optional<vpart_reference> &vpr_src,
                const tripoint_bub_ms &src_bub, const std::unordered_set<tripoint_abs_ms> &dest_set,
                item &it, int &num_processed );




int route_length( const Character &you, const tripoint_bub_ms &dest );




std::optional<tripoint_bub_ms> worst_drag_tile_on_route(
    const Character &who, const std::vector<tripoint_abs_ms> &dropoff_coords );


struct viewport_bbox {
    tripoint_abs_ms min_corner;
    tripoint_abs_ms max_corner;
    tripoint_abs_ms centroid;

    int width() const;
    int height() const;
};



viewport_bbox calc_zone_bbox( const std::unordered_set<tripoint_abs_ms> &tiles );



bool expand_bbox( viewport_bbox &bbox, const tripoint_abs_ms &tile );



bool expand_bbox_raw( tripoint_abs_ms &bbox_min, tripoint_abs_ms &bbox_max,
                      const tripoint_abs_ms &tile );




int calc_target_zoom( int bbox_w, int bbox_h,
                      int visible_w, int visible_h, int current_zoom,
                      int saved_zoom, int padding = 4 );
}


namespace multi_activity_actor
{


bool can_do_in_dark( const activity_id &act_id );
bool activity_must_be_in_zone( activity_id act_id, const tripoint_bub_ms &src_loc );

bool activity_reason_continue( do_activity_reason reason );

bool activity_reason_quit( do_activity_reason reason );

bool activity_reason_picks_up_tools( do_activity_reason reason );
void check_npc_revert( Character &you );

requirement_check_result requirement_fail( Character &you, const do_activity_reason &reason,
        const activity_id &act_id, const zone_data *zone );

requirement_id hash_and_cache_requirement_data( const requirement_data &reqs_data );

requirement_id synthesize_requirements(
    const requirement_data::alter_item_comp_vector &requirement_comp_vector,
    const requirement_data::alter_quali_req_vector &quality_comp_vector,
    const requirement_data::alter_tool_comp_vector &tool_comp_vector );

requirement_id remove_met_requirements( requirement_id base_req_id, Character &you );

std::optional<requirement_id> vehicle_work_requirements( Character &you,
        activity_reason_info &act_info, const tripoint_bub_ms &src_loc );


std::unordered_set<tripoint_abs_ms> generic_locations( Character &you, const activity_id &act_id );
void prune_same_tile_locations( Character &you, std::unordered_set<tripoint_abs_ms> &src_set );
void prune_dark_locations( Character &you, std::unordered_set<tripoint_abs_ms> &src_set,
                           const activity_id &act_id );
void prune_dangerous_field_locations( std::unordered_set<tripoint_abs_ms> &src_set );
std::unordered_set<tripoint_abs_ms> no_same_tile_locations( Character &you,
        const activity_id &act_id );

void revert_npc_post_activity( Character &you, activity_id act_id, bool no_locations );
bool out_of_moves( Character &you );
std::optional<bool> route( Character &you, player_activity &act, const tripoint_bub_ms &src_bub,
                           requirement_failure_reasons &fail_reason, bool check_only );
void activity_failure_message( Character &you, activity_id new_activity,
                               const requirement_failure_reasons &fail_reason, bool no_locations );

zone_type_id get_zone_for_act( const tripoint_bub_ms &src_loc, const zone_manager &mgr,
                               const activity_id &act_id, const faction_id &fac_id );
void add_basecamp_storage_to_loot_zone_list(
    zone_manager &mgr, const tripoint_bub_ms &src_loc, Character &you,
    std::vector<tripoint_bub_ms> &loot_zone_spots, std::vector<tripoint_bub_ms> &combined_spots );
bool are_requirements_nearby(
    const std::vector<tripoint_bub_ms> &loot_spots, const requirement_id &needed_things,
    Character &you, const activity_id &activity_to_restore, bool in_loot_zones,
    const tripoint_bub_ms &src_loc );

}
