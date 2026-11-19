#include "cube_direction.h" // IWYU pragma: associated
#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "auto_note.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "character_id.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "filesystem.h"
#include "flood_fill.h"
#include "game.h"
#include "horde_entity.h"
#include "line.h"
#include "map.h"
#include "map_extras.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapgen_post_process.h"
#include "math_defines.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "overmap_connection.h"
#include "overmap_map_data_cache.h"
#include "overmap_noise.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "regional_settings.h"
#include "rng.h"
#include "rotatable_symbols.h"
#include "sets_intersect.h"
#include "simple_pathfinding.h"
#include "string_formatter.h"
#include "talker.h"
#include "text_snippets.h"
#include "translations.h"
#include "worldfactory.h"
#include "zzip.h"

static const mongroup_id GROUP_NEMESIS( "GROUP_NEMESIS" );
static const mongroup_id GROUP_OCEAN_DEEP( "GROUP_OCEAN_DEEP" );
static const mongroup_id GROUP_OCEAN_SHORE( "GROUP_OCEAN_SHORE" );
static const mongroup_id GROUP_RIVER( "GROUP_RIVER" );
static const mongroup_id GROUP_SUBWAY_CITY( "GROUP_SUBWAY_CITY" );
static const mongroup_id GROUP_SWAMP( "GROUP_SWAMP" );
static const mongroup_id GROUP_ZOMBIE_HORDE( "GROUP_ZOMBIE_HORDE" );

static const oter_str_id oter_empty_rock( "empty_rock" );
static const oter_str_id oter_field( "field" );
static const oter_str_id oter_forest( "forest" );
static const oter_str_id oter_forest_thick( "forest_thick" );
static const oter_str_id oter_forest_water( "forest_water" );
static const oter_str_id oter_sewer_end_north( "sewer_end_north" );
static const oter_str_id oter_sewer_isolated( "sewer_isolated" );
static const oter_str_id oter_sewer_sub_station( "sewer_sub_station" );
static const oter_str_id oter_solid_earth( "solid_earth" );
static const oter_str_id oter_subway_end_north( "subway_end_north" );
static const oter_str_id oter_subway_isolated( "subway_isolated" );
static const oter_str_id oter_underground_sub_station( "underground_sub_station" );

static const oter_type_str_id oter_type_bridge( "bridge" );
static const oter_type_str_id oter_type_microlab_sub_connector( "microlab_sub_connector" );
static const oter_type_str_id oter_type_railroad_bridge( "railroad_bridge" );
static const oter_type_str_id oter_type_road( "road" );
static const oter_type_str_id oter_type_road_nesw_manhole( "road_nesw_manhole" );
static const oter_type_str_id oter_type_sewer_connector( "sewer_connector" );
static const oter_type_str_id oter_type_sub_station( "sub_station" );

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

template struct pos_dir<tripoint_om_omt>;
template struct pos_dir<tripoint_rel_omt>;


static oter_id ot_null;

namespace io
{

template<>
std::string enum_to_string<overmap_special_subtype>( overmap_special_subtype data )
{
    switch( data ) {

        case overmap_special_subtype::fixed:
            return "fixed";
        case overmap_special_subtype::mutable_:
            return "mutable";

        case overmap_special_subtype::last:
            break;
    }
    cata_fatal( "Invalid overmap_special_subtype" );
}

template<>
std::string enum_to_string<om_direction::type>( om_direction::type d )
{
    switch( d ) {

        case om_direction::type::north:
            return "north";
        case om_direction::type::east:
            return "east";
        case om_direction::type::south:
            return "south";
        case om_direction::type::west:
            return "west";

        case om_direction::type::invalid:
        case om_direction::type::last:
            break;
    }
    cata_fatal( "Invalid om_direction" );
}

template<>
std::string enum_to_string<cube_direction>( cube_direction data )
{
    switch( data ) {

        case cube_direction::north:
            return "north";
        case cube_direction::east:
            return "east";
        case cube_direction::south:
            return "south";
        case cube_direction::west:
            return "west";
        case cube_direction::above:
            return "above";
        case cube_direction::below:
            return "below";

        case cube_direction::last:
            break;
    }
    cata_fatal( "Invalid cube_direction" );
}

}

size_t om_lines::from_dir( om_direction::type dir )
{
    switch( dir ) {
        case om_direction::type::north:
        case om_direction::type::south:
            return 5;
        case om_direction::type::east:
        case om_direction::type::west:
            return 10;
        case om_direction::type::invalid:
        case om_direction::type::last:
            debugmsg( "Can't retrieve a line from the invalid direction." );
    }

    return 0;
}

std::map<radio_type, std::string> radio_type_names =
{{ {radio_type::MESSAGE_BROADCAST, "broadcast"}, {radio_type::WEATHER_RADIO, "weather"} }};

void city_buildings::load( const JsonObject &jo, const std::string &src )
{

    overmap_specials::load( jo, src );
}

cube_direction operator+( const cube_direction l, const om_direction::type r )
{
    return l + static_cast<int>( r );
}

cube_direction operator+( const cube_direction d, int i )
{
    switch( d ) {
        case cube_direction::north:
        case cube_direction::east:
        case cube_direction::south:
        case cube_direction::west:
            return static_cast<cube_direction>( ( static_cast<int>( d ) + i ) % 4 );
        case cube_direction::above:
        case cube_direction::below:
            return d;
        case cube_direction::last:
            break;
    }
    constexpr_fatal( cube_direction::last, "Invalid cube_direction" );
}

cube_direction operator-( const cube_direction l, const om_direction::type r )
{
    return l - static_cast<int>( r );
}

cube_direction operator-( const cube_direction d, int i )
{
    switch( d ) {
        case cube_direction::north:
        case cube_direction::east:
        case cube_direction::south:
        case cube_direction::west:
            return static_cast<cube_direction>( ( static_cast<int>( d ) - i + 4 ) % 4 );
        case cube_direction::above:
        case cube_direction::below:
            return d;
        case cube_direction::last:
            break;
    }
    constexpr_fatal( cube_direction::last, "Invalid cube_direction" );
}

tripoint displace( cube_direction d )
{
    switch( d ) {
        case cube_direction::north:
            return tripoint::north;
        case cube_direction::east:
            return tripoint::east;
        case cube_direction::south:
            return tripoint::south;
        case cube_direction::west:
            return tripoint::west;
        case cube_direction::above:
            return tripoint::above;
        case cube_direction::below:
            return tripoint::below;
        case cube_direction::last:
            break;
    }
    cata_fatal( "Invalid cube_direction" );
}


overmap::overmap( const point_abs_om &p ) : loc( p )
{
    settings = overmap_buffer.get_overmap_region( tripoint_abs_om( p, 0 ) );
    init_layers();
    hordes.set_location( loc );
}

overmap::~overmap() = default;

void overmap::populate( overmap_special_batch &enabled_specials )
{

    const region_settings_feature_flag &overmap_feature_flag = settings->overmap_feature_flag;
    const bool should_blacklist = !overmap_feature_flag.blacklist.empty();
    const bool should_whitelist = !overmap_feature_flag.whitelist.empty();





    if( should_blacklist ) {
        for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
            if( cata::sets_intersect( overmap_feature_flag.blacklist,
                                      it->special_details->get_flags() ) ) {
                it = enabled_specials.erase( it );
            } else {
                ++it;
            }
        }
    }


    if( should_whitelist ) {
        for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
            if( cata::sets_intersect( overmap_feature_flag.whitelist,
                                      it->special_details->get_flags() ) ) {
                ++it;
            } else {
                it = enabled_specials.erase( it );
            }
        }
    }
    try {
        open( enabled_specials );
    } catch( const std::exception &err ) {
        debugmsg( "overmap (%d,%d) failed to load: %s", loc.x(), loc.y(), err.what() );
    }
}

void overmap::populate()
{
    overmap_special_batch enabled_specials = overmap_specials::get_default_batch( loc,
            settings->get_settings_city().city_size );
    populate( enabled_specials );
}

oter_id overmap::get_default_terrain( int z ) const
{
    return settings->default_oter[OVERMAP_DEPTH + z].id();
}


static map_data_summary impassable_omt{};
static map_data_summary passable_omt{ ~impassable_omt.passable, true };

void overmap::init_layers()
{
    for( int k = 0; k < OVERMAP_LAYERS; ++k ) {
        const oter_id tid = get_default_terrain( k - OVERMAP_DEPTH );
        map_layer &l = layer[k];
        l.terrain.fill( tid );
        l.visible.fill( om_vision_level::unseen );
        l.explored.fill( false );

        l.map_cache.fill( std::shared_ptr<map_data_summary> { std::shared_ptr<void>(), &passable_omt } );
    }
}

void overmap::ter_set( const tripoint_om_omt &p, const oter_id &id )
{
    if( !inbounds( p ) ) {

        return;
    }

    oter_id &current_oter = layer[p.z() + OVERMAP_DEPTH].terrain[p.xy()];
    const oter_type_str_id &current_type_id = current_oter->get_type_id();
    const oter_type_str_id &incoming_type_id = id->get_type_id();
    const bool current_type_same = current_type_id == incoming_type_id;







    if( id->has_flag( oter_flags::requires_predecessor ) ) {
        std::vector<oter_id> &om_predecessors = predecessors_[p];
        if( om_predecessors.empty() || ( !current_oter->is_linear() && !current_type_same ) ) {


            om_predecessors.push_back( current_oter );
        } else if( !current_type_same ) {



            oter_id &last_predecessor = om_predecessors.back();
            if( last_predecessor->get_type_id() == current_type_id ) {
                last_predecessor = current_oter;
            } else {
                om_predecessors.push_back( current_oter );
            }
        }


    }

    set_passable( project_combine( loc, p ), id->get_type_id()->default_map_data );
    current_oter = id;
}

const oter_id &overmap::ter( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {

        return ot_null;
    }

    return ter_unsafe( p );
}

const oter_id &overmap::ter_unsafe( const tripoint_om_omt &p ) const
{
    return layer[p.z() + OVERMAP_DEPTH].terrain[p.xy()];
}

std::optional<mapgen_arguments> *overmap::mapgen_args( const tripoint_om_omt &p )
{
    auto it = mapgen_args_index.find( p );
    if( it == mapgen_args_index.end() ) {
        return nullptr;
    }
    return it->second;
}

std::vector<pp_resolved_generator> *overmap::pp_decisions( const tripoint_om_omt &p )
{
    auto it = pp_decisions_index.find( p );
    if( it == pp_decisions_index.end() ) {
        return nullptr;
    }
    return it->second;
}

std::string *overmap::join_used_at( const om_pos_dir &p )
{
    auto it = joins_used.find( p );
    if( it == joins_used.end() ) {
        return nullptr;
    }
    return &it->second;
}

std::vector<oter_id> overmap::predecessors( const tripoint_om_omt &p )
{
    auto it = predecessors_.find( p );
    if( it == predecessors_.end() ) {
        return {};
    }
    return it->second;
}

void overmap::set_seen( const tripoint_om_omt &p, om_vision_level val, bool force )
{
    if( !inbounds( p ) ) {
        return;
    }

    if( !force && seen( p ) >= val ) {
        return;
    }

    layer[p.z() + OVERMAP_DEPTH].visible[p.xy()] = val;

    add_extra_note( p );
}

om_vision_level overmap::seen( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return om_vision_level::unseen;
    }
    return layer[p.z() + OVERMAP_DEPTH].visible[p.xy()];
}

bool &overmap::explored( const tripoint_om_omt &p )
{
    if( !inbounds( p ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[p.z() + OVERMAP_DEPTH].explored[p.xy()];
}

bool overmap::is_explored( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    return layer[p.z() + OVERMAP_DEPTH].explored[p.xy()];
}

bool overmap::mongroup_check( const mongroup &candidate ) const
{
    tripoint_om_sm relp = candidate.rel_pos();
    const auto matching_range = zg.equal_range( relp );
    return std::find_if( matching_range.first, matching_range.second,
    [candidate]( const std::pair<tripoint_om_sm, mongroup> &match ) {

        return candidate.type == match.second.type && candidate.abs_pos == match.second.abs_pos &&
               candidate.population == match.second.population &&
               candidate.target == match.second.target &&
               candidate.interest == match.second.interest &&
               candidate.dying == match.second.dying &&
               candidate.horde == match.second.horde;
    } ) != matching_range.second;
}

void overmap::insert_npc( const shared_ptr_fast<npc> &who )
{
    npcs.push_back( who );
    g->set_npcs_dirty();
}

shared_ptr_fast<npc> overmap::erase_npc( const character_id &id )
{
    const auto iter = std::find_if( npcs.begin(),
    npcs.end(), [id]( const shared_ptr_fast<npc> &n ) {
        return n->getID() == id;
    } );
    if( iter == npcs.end() ) {
        return nullptr;
    }
    auto ptr = *iter;
    npcs.erase( iter );
    g->set_npcs_dirty();
    return ptr;
}

std::vector<shared_ptr_fast<npc>> overmap::get_npcs( const
                               std::function<bool( const npc & )>
                               &predicate ) const
{
    std::vector<shared_ptr_fast<npc>> result;
    for( const auto &g : npcs ) {
        if( predicate( *g ) ) {
            result.push_back( g );
        }
    }
    return result;
}

bool overmap::has_note( const tripoint_om_omt &p ) const
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return false;
    }

    for( const om_note &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
        if( i.p == p.xy() ) {
            return true;
        }
    }
    return false;
}

bool overmap::is_marked_dangerous( const tripoint_om_omt &p ) const
{
    for( const om_note &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
        if( !i.dangerous ) {
            continue;
        } else if( p.xy() == i.p ) {
            return true;
        }
        const int radius = i.danger_radius;
        if( i.danger_radius == 0 && i.p != p.xy() ) {
            continue;
        }

        int dist = rl_dist( i.p, p.xy() );
        if( dist <= radius ) {
            return true;
        }
    }
    return false;
}

point_om_omt overmap::get_fallback_road_connection_point() const
{
    if( fallback_road_connection_point ) {
        return *fallback_road_connection_point;
    } else {
        return point_om_omt( rng( OMAPX / 4, ( 3 * OMAPX ) / 4 ),
                             rng( OMAPY / 4, ( 3 * OMAPY ) / 4 ) );
    }
}

const std::string &overmap::note( const tripoint_om_omt &p ) const
{
    static const std::string fallback {};

    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return fallback;
    }

    const auto &notes = layer[p.z() + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.p == p.xy();
    } );

    return ( it != std::end( notes ) ) ? it->text : fallback;
}

void overmap::add_note( const tripoint_om_omt &p, std::string message )
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        debugmsg( "Attempting to add not to overmap for blank layer %d", p.z() );
        return;
    }

    auto &notes = layer[p.z() + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.p == p.xy();
    } );

    if( it == std::end( notes ) ) {
        notes.emplace_back( om_note{ std::move( message ), p.xy() } );
    } else if( !message.empty() ) {
        it->text = std::move( message );
    } else {
        notes.erase( it );
    }
}

void overmap::mark_note_dangerous( const tripoint_om_omt &p, int radius, bool is_dangerous )
{
    for( om_note &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
        if( p.xy() == i.p ) {
            i.dangerous = is_dangerous;
            i.danger_radius = radius;
            return;
        }
    }
}

int overmap::note_danger_radius( const tripoint_om_omt &p ) const
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return -1;
    }

    const auto &notes = layer[p.z() + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.p == p.xy();
    } );

    return ( it != std::end( notes ) ) && it->dangerous ? it->danger_radius : -1;
}

void overmap::delete_note( const tripoint_om_omt &p )
{
    add_note( p, std::string{} );
}

std::vector<point_abs_omt> overmap::find_notes( const int z, const std::string &text )
{
    std::vector<point_abs_omt> note_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( const om_note &note : this_layer.notes ) {
        if( match_include_exclude( note.text, text ) ) {
            note_locations.push_back( project_combine( pos(), note.p ) );
        }
    }
    return note_locations;
}

bool overmap::has_extra( const tripoint_om_omt &p ) const
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return false;
    }

    for( const om_map_extra &i : layer[p.z() + OVERMAP_DEPTH].extras ) {
        if( i.p == p.xy() ) {
            return true;
        }
    }
    return false;
}

const map_extra_id &overmap::extra( const tripoint_om_omt &p ) const
{
    static const map_extra_id fallback{};

    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return fallback;
    }

    const auto &extras = layer[p.z() + OVERMAP_DEPTH].extras;
    const auto it = std::find_if( begin( extras ),
    end( extras ), [&]( const om_map_extra & n ) {
        return n.p == p.xy();
    } );

    return ( it != std::end( extras ) ) ? it->id : fallback;
}

void overmap::add_extra( const tripoint_om_omt &p, const map_extra_id &id )
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        debugmsg( "Attempting to add not to overmap for blank layer %d", p.z() );
        return;
    }

    auto &extras = layer[p.z() + OVERMAP_DEPTH].extras;
    const auto it = std::find_if( begin( extras ),
    end( extras ), [&]( const om_map_extra & n ) {
        return n.p == p.xy();
    } );

    if( it == std::end( extras ) ) {
        extras.emplace_back( om_map_extra{ id, p.xy() } );
        add_extra_note( p );
    } else if( !id.is_null() ) {
        it->id = id;
        add_extra_note( p );
    } else {
        extras.erase( it );
    }
}

void overmap::add_extra_note( const tripoint_om_omt &p, bool force_add )
{


    const std::vector<om_map_extra> &layer_extras = layer[p.z() + OVERMAP_DEPTH].extras;
    auto extrait = std::find_if( layer_extras.begin(),
    layer_extras.end(), [&p]( const om_map_extra & extra ) {
        return extra.p == p.xy();
    } );
    if( extrait == layer_extras.end() ) {
        return;
    }
    const map_extra_id &extra = extrait->id;

    if( !force_add && !extra->potentially_visible_at( seen( p ) ) ) {
        return;
    }

    if( !force_add && extra->visibility == map_extra_visibility::same_tile ) {
        tripoint_abs_omt target = project_combine( this->pos(), p );
        if( target != get_avatar().pos_abs_omt() ) {
            return;
        }
    }


    auto_notes::auto_note_settings &auto_note_settings = get_auto_notes_settings();


    auto_note_settings.set_discovered( extra );

    if( get_option<bool>( "AUTO_NOTES" ) && get_option<bool>( "AUTO_NOTES_MAP_EXTRAS" ) ) {

        if( !auto_note_settings.has_auto_note_enabled( extra, true ) ) {
            return;
        }

        const std::optional<auto_notes::custom_symbol> &symbol =
            auto_note_settings.get_custom_symbol( extra );
        const std::string note_symbol = symbol ? ( *symbol ).get_symbol_string() : extra->get_symbol();
        const nc_color note_color = symbol ? ( *symbol ).get_color() : extra->color;
        const std::string mx_note =
            string_format( "%s:%s;<color_yellow>%s</color>: <color_white>%s</color>",
                           note_symbol,
                           get_note_string_from_color( note_color ),
                           extra->name(),
                           extra->description() );
        add_note( p, mx_note );
    }
}

void overmap::delete_extra( const tripoint_om_omt &p )
{
    add_extra( p, map_extra_id::NULL_ID() );
}

std::vector<point_abs_omt> overmap::find_extras( const int z, const std::string &text )
{
    std::vector<point_abs_omt> extra_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( const om_map_extra &extra : this_layer.extras ) {
        const std::string extra_text = extra.id.c_str();
        if( match_include_exclude( extra_text, text ) ) {
            extra_locations.push_back( project_combine( pos(), extra.p ) );
        }
    }
    return extra_locations;
}

std::optional<mapgen_arguments> overmap::get_existing_omt_stack_arguments(
    const point_abs_omt &p ) const
{
    auto it_args = omt_stack_arguments_map.find( p );
    if( it_args != omt_stack_arguments_map.end() ) {
        return it_args->second;
    }
    return std::nullopt;
}

std::optional<cata_variant> overmap::get_existing_omt_stack_argument( const point_abs_omt &p,
        const std::string &param_name ) const
{
    auto it_args = omt_stack_arguments_map.find( p );
    if( it_args != omt_stack_arguments_map.end() ) {
        const std::unordered_map<std::string, cata_variant> &args_map = it_args->second.map;
        auto it_arg = args_map.find( param_name );
        if( it_arg != args_map.end() ) {
            return it_arg->second;
        }
    }
    return std::nullopt;
}

void overmap::add_omt_stack_argument( const point_abs_omt &p, const std::string &param_name,
                                      const cata_variant &value )
{
    omt_stack_arguments_map[p].add( param_name, value );
}

bool overmap::passable( const tripoint_om_ms &p )
{
    point_om_omt omt_origin;
    tripoint_omt_ms index;
    std::tie( omt_origin, index ) = project_remain<coords::omt>( p );
    std::shared_ptr<map_data_summary> &ptr = layer[index.z() +
            OVERMAP_DEPTH].map_cache[omt_origin];
    if( !ptr ) {


        return false;
    }
    if( !ptr->passable[index.y() * 24 + index.x()] ) {
        return false;
    }
    return hordes.entity_at( p ) == nullptr;
}

std::shared_ptr<map_data_summary> overmap::get_omt_summary( const tripoint_om_omt &p )
{
    std::shared_ptr<map_data_summary> &ptr = layer[p.z() +
            OVERMAP_DEPTH].map_cache[p.xy()];
    if( !ptr ) {


        return std::shared_ptr<map_data_summary>();
    }
    return ptr;
}

void overmap::set_passable( const tripoint_om_ms &p, bool new_passable )
{
    point_om_omt omt_origin;
    tripoint_omt_ms index;
    std::tie( omt_origin, index ) = project_remain<coords::omt>( p );
    std::shared_ptr<map_data_summary> &ptr = layer[index.z() +
            OVERMAP_DEPTH].map_cache[omt_origin];
    if( !ptr ) {


        return;
    }
    if( ptr->placeholder ) {

        ptr = std::make_shared<map_data_summary>( ptr->passable );
    }
    ptr->passable[index.y() * 24 + index.x()] = new_passable;
}


void overmap::set_passable( const tripoint_abs_omt &p,
                            std::shared_ptr<map_data_summary> new_passable )
{
    point_abs_om overmap_coord;
    tripoint_om_omt omt_coord;
    std::tie( overmap_coord, omt_coord ) = project_remain<coords::om>( p );
    if( overmap_coord != loc ) {
        return;
    }
    layer[omt_coord.z() + OVERMAP_DEPTH].map_cache[omt_coord.xy()] = std::move( new_passable );
}

void overmap::set_passable( const tripoint_abs_omt &p,
                            string_id<map_data_summary> new_passable )
{
    point_abs_om overmap_coord;
    tripoint_om_omt omt_coord;
    std::tie( overmap_coord, omt_coord ) = project_remain<coords::om>( p );
    if( overmap_coord != loc ) {
        return;
    }
    std::shared_ptr<map_data_summary> &ptr = layer[omt_coord.z() +
            OVERMAP_DEPTH].map_cache[omt_coord.xy()];



    ptr = std::const_pointer_cast<map_data_summary>( map_data_placeholders::get_ptr(
                new_passable ) );
}

void overmap::set_passable( const tripoint_abs_omt &p, const std::bitset<24 * 24> &new_passable )
{
    point_abs_om overmap_coord;
    tripoint_om_omt omt_coord;
    std::tie( overmap_coord, omt_coord ) = project_remain<coords::om>( p );
    if( overmap_coord != loc ) {
        return;
    }
    std::shared_ptr<map_data_summary> &ptr = layer[omt_coord.z() +
            OVERMAP_DEPTH].map_cache[omt_coord.xy()];
    if( !ptr ) {


        return;
    }
    ptr = std::make_shared<map_data_summary>( new_passable );
}

bool overmap::inbounds( const tripoint_abs_ms &p )
{
    point_abs_om overmap_coord;
    tripoint_om_omt omt_within_overmap;
    std::tie( overmap_coord, omt_within_overmap ) =
        project_remain<coords::om>( project_to<coords::omt> ( p ) );
    return overmap_coord == loc;
}

bool overmap::inbounds( const tripoint_om_omt &p, int clearance )
{
    static constexpr tripoint_om_omt overmap_boundary_min( 0, 0, -OVERMAP_DEPTH );
    static constexpr tripoint_om_omt overmap_boundary_max( OMAPX, OMAPY, OVERMAP_HEIGHT + 1 );

    static constexpr half_open_cuboid<tripoint_om_omt> overmap_boundaries(
        overmap_boundary_min, overmap_boundary_max );
    half_open_cuboid<tripoint_om_omt> stricter_boundaries = overmap_boundaries;
    stricter_boundaries.shrink( tripoint( clearance, clearance, 0 ) );

    return stricter_boundaries.contains( p );
}

const scent_trace &overmap::scent_at( const tripoint_abs_omt &loc ) const
{
    static const scent_trace null_scent;
    const auto &scent_found = scents.find( loc );
    if( scent_found != scents.end() ) {
        return scent_found->second;
    }
    return null_scent;
}

void overmap::set_scent( const tripoint_abs_omt &loc, const scent_trace &new_scent )
{

    scents[loc] = new_scent;
}

void overmap::generate( const std::vector<const overmap *> &neighbor_overmaps,
                        overmap_special_batch &enabled_specials )
{
    dbg( D_INFO ) << "overmap::generate start…";
    const oter_id omt_outside_defined_omap = static_cast<oter_id>
            ( get_option<std::string>( "OUTSIDE_DEFINED_OMAP_OMT" ) );
    const std::string overmap_pregenerated_path =
        get_option<std::string>( "OVERMAP_PREGENERATED_PATH" );
    if( !overmap_pregenerated_path.empty() ) {

#if defined(__ANDROID__)
        static const std::string fname = "%s/overmap_%d_%d.omap";
#else
        static const std::string fname = "%s/overmap_%d_%d.omap.gz";
#endif
        const cata_path fpath = PATH_INFO::moddir() / string_format( fname,
                                overmap_pregenerated_path, pos().x(), pos().y() );
        dbg( D_INFO ) << "trying" << fpath;
        if( !read_from_file_optional_json( fpath, [this, &fpath]( const JsonValue & jv ) {
        unserialize_omap( jv, fpath );
        } ) ) {
            dbg( D_INFO ) << "failed" << fpath;
            int z = 0;
            for( int j = 0; j < OMAPY; j++ ) {
                // NOLINTNEXTLINE(modernize-loop-convert)
                for( int i = 0; i < OMAPX; i++ ) {
                    set_passable( project_combine( loc, tripoint_om_omt( i, j, z ) ),
                                  omt_outside_defined_omap->get_type_id()->default_map_data );
                    layer[z + OVERMAP_DEPTH].terrain[i][j] = omt_outside_defined_omap;
                }
            }
        }
    }

    std::vector<Highway_path> highway_paths;
    calculate_urbanity();
    calculate_forestosity();
    if( settings->neighbor_connections ) {
        populate_connections_out_from_neighbors( neighbor_overmaps );
    }
    if( settings->overmap_river ) {
        place_rivers( neighbor_overmaps );
    }
    if( settings->overmap_lake ) {
        place_lakes( neighbor_overmaps );
    }
    if( settings->overmap_ocean ) {
        place_oceans( neighbor_overmaps );
    }
    if( settings->overmap_forest ) {
        place_forests();
    }
    if( settings->overmap_forest && settings->place_swamps ) {
        place_swamps();
    }
    if( settings->overmap_ravine ) {
        place_ravines();
    }
    if( settings->overmap_river ) {

        polish_river( neighbor_overmaps );
    }
    if( settings->overmap_highway ) {
        highway_paths = place_highways( neighbor_overmaps );
    }
    if( settings->city_spec ) {
        place_cities();
    }
    if( settings->overmap_highway ) {
        place_highway_interchanges( highway_paths );
    }
    if( settings->city_spec ) {
        build_cities();
    }
    if( settings->forest_trail ) {
        place_forest_trails();
    }
    if( settings->place_railroads_before_roads ) {
        if( settings->place_railroads ) {
            place_railroads( neighbor_overmaps );
        }
        if( settings->place_roads ) {
            place_roads( neighbor_overmaps );
        }
    } else {
        if( settings->place_roads ) {
            place_roads( neighbor_overmaps );
        }
        if( settings->place_railroads ) {
            place_railroads( neighbor_overmaps );
        }
    }
    if( settings->place_specials ) {
        place_specials( enabled_specials );
    }
    if( settings->overmap_highway ) {
        finalize_highways( highway_paths );
    }
    if( settings->forest_trail ) {
        place_forest_trailheads();
    }
    if( settings->overmap_river ) {
        polish_river( neighbor_overmaps );
    }





    int z = -1;
    bool requires_sub = false;
    do {
        requires_sub = generate_sub( z );
    } while( requires_sub && ( --z >= -OVERMAP_DEPTH ) );


    z = 1;
    bool requires_over = false;
    do {
        requires_over = generate_over( z );
    } while( requires_over && ( ++z <= OVERMAP_HEIGHT ) );


    place_mongroups();
    place_radios();
    dbg( D_INFO ) << "overmap::generate done";
}

bool overmap::generate_sub( const int z )
{
    cata_assert( z < 0 );


    bool requires_sub = true;
    std::vector<point_om_omt> subway_points;
    std::vector<point_om_omt> sewer_points;

    std::vector<city> ant_points;
    std::vector<city> goo_points;

    std::unordered_map<oter_type_id, std::function<void( const tripoint_om_omt &p )>>
    oter_above_actions = {
        {
            oter_type_road_nesw_manhole.id(),
            [&]( const tripoint_om_omt & p )
            {
                ter_set( p, oter_sewer_isolated.id() );
                sewer_points.emplace_back( p.xy() );
            }
        },
    };

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt p( i, j, z );
            const oter_id oter_id_here = ter_unsafe( p );
            const oter_t &oter_here = *oter_id_here;
            const oter_id oter_above = ter_unsafe( p + tripoint::above );
            const oter_id oter_ground = ter_unsafe( tripoint_om_omt( p.xy(), 0 ) );

            if( oter_here.get_type_id() == oter_type_sewer_connector ) {
                om_direction::type rotation = oter_here.get_dir();
                ter_set( p, oter_sewer_end_north.id()->get_rotated( rotation ) );
                sewer_points.emplace_back( p.xy() );
            }

            if( oter_here.get_type_id() == oter_type_microlab_sub_connector ) {
                om_direction::type rotation = oter_here.get_dir();
                ter_set( p, oter_subway_end_north.id()->get_rotated( rotation ) );
                subway_points.emplace_back( p.xy() );
            }

            if( oter_ground->get_type_id() == oter_type_sub_station ) {
                if( z == -1 ) {
                    ter_set( p, oter_sewer_sub_station.id() );
                    requires_sub = true;
                    continue;
                } else if( z == -2 ) {
                    ter_set( p, oter_subway_isolated.id() );
                    subway_points.emplace_back( i, j - 1 );
                    subway_points.emplace_back( i, j );
                    subway_points.emplace_back( i, j + 1 );
                    continue;
                }
            }

            auto above_action_it = oter_above_actions.find( oter_above->get_type_id().id() );

            if( above_action_it != oter_above_actions.end() ) {
                above_action_it->second( p );
            }
        }
    }
    const overmap_connection_id &overmap_connection_sewer_tunnel =
        settings->overmap_connection.sewer_connection;
    connect_closest_points( sewer_points, z, *overmap_connection_sewer_tunnel );

    const overmap_connection_id &overmap_connection_subway_tunnel =
        settings->overmap_connection.subway_connection;
    connect_closest_points( subway_points, z, *overmap_connection_subway_tunnel );

    for( auto &i : subway_points ) {
        if( ( ter( tripoint_om_omt( i, z + 2 ) )->get_type_id() == oter_type_sub_station ) ) {
            ter_set( tripoint_om_omt( i, z ), oter_underground_sub_station.id() );
        }
    }

    for( city &i : cities ) {
        tripoint_om_omt omt_pos( i.pos, z );
        tripoint_om_sm sm_pos = project_to<coords::sm>( omt_pos );
        tripoint_abs_sm abs_pos = project_combine( pos(), sm_pos );

        if( z == -2 ) {
            spawn_mon_group( mongroup( GROUP_SUBWAY_CITY, abs_pos, i.size * i.size * 2 ),
                             i.size * 2 );
        }
    }

    return requires_sub;
}

bool overmap::generate_over( const int z )
{
    bool requires_over = false;
    std::vector<point_om_omt> bridge_points;
    std::vector<point_om_omt> railroad_bridge_points;


    const std::set<oter_id> skip_below = {
        oter_empty_rock.id(), oter_forest.id(), oter_field.id(),
        oter_forest_thick.id(), oter_forest_water.id(), oter_solid_earth.id()
    };

    if( z == 1 ) {
        for( int i = 0; i < OMAPX; i++ ) {
            for( int j = 0; j < OMAPY; j++ ) {
                tripoint_om_omt p( i, j, z );
                const oter_id oter_below = ter( p + tripoint::below );
                const oter_id oter_ground = ter( tripoint_om_omt( p.xy(), 0 ) );


                if( skip_below.find( oter_below ) != skip_below.end() ) {
                    continue;
                }

                if( oter_ground->get_type_id() == oter_type_bridge ) {
                    ter_set( p, oter_id( "bridge_road" + oter_get_rotation_string( oter_ground ) ) );
                    bridge_points.push_back( p.xy() );
                    tripoint_om_omt support_point = p + tripoint::below;
                    int support_z = 0;

                    while( ter( support_point ) -> is_water() && --support_z >= -OVERMAP_DEPTH ) {
                        ter_set( support_point, oter_id( "bridge" + oter_get_rotation_string( oter_ground ) ) );
                        support_point += tripoint::below;
                    }
                }
                if( oter_ground->get_type_id() == oter_type_railroad_bridge ) {
                    ter_set( p, oter_id( "railroad_bridge_overpass" + oter_get_rotation_string( oter_ground ) ) );
                    railroad_bridge_points.push_back( p.xy() );
                }
            }
        }
    }

    generate_bridgeheads( bridge_points, oter_type_bridge,
                          "bridgehead_ground", "bridgehead_ramp" );
    generate_bridgeheads( railroad_bridge_points, oter_type_railroad_bridge,
                          "railroad_bridgehead_ground", "railroad_bridgehead_ramp" );

    return requires_over;
}

void overmap::generate_bridgeheads( const std::vector<point_om_omt> &bridge_points,
                                    const oter_type_str_id bridge_type,
                                    const std::string &bridgehead_ground,
                                    const std::string &bridgehead_ramp )
{
    std::vector<std::pair<point_om_omt, std::string>> bridgehead_points;
    for( const point_om_omt &bp : bridge_points ) {

        const oter_id oter_ground_north = ter( tripoint_om_omt( bp, 0 ) + tripoint::north );
        const oter_id oter_ground_south = ter( tripoint_om_omt( bp, 0 ) + tripoint::south );
        const oter_id oter_ground_east = ter( tripoint_om_omt( bp, 0 ) + tripoint::east );
        const oter_id oter_ground_west = ter( tripoint_om_omt( bp, 0 ) + tripoint::west );
        const bool is_bridge_north = ( oter_ground_north->get_type_id() == bridge_type )
                                     && ( oter_get_rotation( oter_ground_north ) % 2 == 0 );
        const bool is_bridge_south = ( oter_ground_south->get_type_id() == bridge_type )
                                     && ( oter_get_rotation( oter_ground_south ) % 2 == 0 );
        const bool is_bridge_east = ( oter_ground_east->get_type_id() == bridge_type )
                                    && ( oter_get_rotation( oter_ground_east ) % 2 == 1 );
        const bool is_bridge_west = ( oter_ground_west->get_type_id() == bridge_type )
                                    && ( oter_get_rotation( oter_ground_west ) % 2 == 1 );

        if( is_bridge_north ^ is_bridge_south || is_bridge_east ^ is_bridge_west ) {
            std::string ramp_facing;
            if( is_bridge_north ) {
                ramp_facing = "_south";
            } else if( is_bridge_south ) {
                ramp_facing = "_north";
            } else if( is_bridge_east ) {
                ramp_facing = "_west";
            } else {
                ramp_facing = "_east";
            }
            bridgehead_points.emplace_back( bp, ramp_facing );
        }
    }
    for( const std::pair<point_om_omt, std::string> &bhp : bridgehead_points ) {
        tripoint_om_omt p( bhp.first, 0 );
        ter_set( p, oter_id( bridgehead_ground + bhp.second ) );
        ter_set( p + tripoint::above, oter_id( bridgehead_ramp + bhp.second ) );
    }
}

std::vector<point_abs_omt> overmap::find_terrain( std::string_view term, int zlevel ) const
{
    std::vector<point_abs_omt> found;
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            tripoint_om_omt p( x, y, zlevel );
            om_vision_level vision = seen( p );
            if( vision != om_vision_level::unseen &&
                lcmatch( ter( p )->get_name( vision ), term ) ) {
                found.push_back( project_combine( pos(), p.xy() ) );
            }
        }
    }
    return found;
}

const city &overmap::get_nearest_city( const tripoint_om_omt &p ) const
{
    int distance = 999;
    const city *res = nullptr;
    for( const city &elem : cities ) {
        const int dist = elem.get_distance_from( p );
        if( dist < distance ) {
            distance = dist;
            res = &elem;
        }
    }
    if( res != nullptr ) {
        return *res;
    }
    static city invalid_city;
    return invalid_city;
}

const city &overmap::get_invalid_city() const
{
    static city invalid_city;
    return invalid_city;
}

tripoint_om_omt overmap::find_random_omt( const std::pair<std::string, ot_match_type> &target,
        std::optional<city> target_city ) const
{
    const bool check_nearest_city = target_city.has_value();
    std::vector<tripoint_om_omt> valid;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                tripoint_om_omt p( i, j, k );
                if( is_ot_match( target.first, ter( p ), target.second ) ) {
                    if( !check_nearest_city || get_nearest_city( p ) == target_city.value() ) {
                        valid.push_back( p );
                    }
                }
            }
        }
    }
    return random_entry( valid, tripoint_om_omt::invalid );
}

void overmap::process_mongroups()
{
    for( auto it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( mg.dying ) {
            mg.population = ( mg.population * 4 ) / 5;
        }
        if( mg.empty() ) {
            zg.erase( it++ );
        } else {
            ++it;
        }
    }
}

void overmap::clear_mon_groups()
{
    zg.clear();
}

void overmap::clear_overmap_special_placements()
{
    overmap_special_placements.clear();
}

void overmap::clear_connections_out()
{
    connections_out.clear();
}

static std::map<std::string, std::string> oter_id_migrations;
static std::map<oter_type_str_id, std::pair<translation, faction_id>> camp_migration_map;

void overmap::load_oter_id_migration( const JsonObject &jo )
{
    for( const JsonMember &kv : jo.get_object( "oter_ids" ) ) {
        const std::string old_id = kv.name();
        const std::string new_id = kv.get_string();

        if( old_id == new_id ) {
            if( auto it = oter_id_migrations.find( old_id ); it != oter_id_migrations.end() ) {
                oter_id_migrations.erase( it );
            }
        } else {

            oter_id_migrations.insert_or_assign( old_id, new_id );
        }
    }
}

void overmap::load_oter_id_camp_migration( const JsonObject &jo )
{
    std::string name;
    oter_type_str_id oter;
    faction_id owner;
    JsonObject jsobj = jo.get_object( "camp_migrations" );
    jsobj.read( "name", name );
    jsobj.read( "overmap_terrain", oter );
    jsobj.read( "faction", owner );
    camp_migration_map.emplace( oter,
                                std::pair<translation, faction_id>( translation::to_translation( name ), owner ) );
}

void overmap::reset_oter_id_camp_migrations()
{
    camp_migration_map.clear();
}

void overmap::reset_oter_id_migrations()
{
    oter_id_migrations.clear();
}

bool overmap::oter_id_should_have_camp( const oter_type_str_id &oter )
{
    return camp_migration_map.count( oter ) > 0;
}

bool overmap::is_oter_id_obsolete( const std::string &oterid )
{
    return oter_id_migrations.count( oterid ) > 0;
}

void overmap::migrate_oter_ids( const std::unordered_map<tripoint_om_omt, std::string> &points )
{
    for( const auto&[pos, old_id] : points ) {
        const oter_str_id new_id = oter_str_id( oter_id_migrations.at( old_id ) );
        const tripoint_abs_sm pos_abs = project_to<coords::sm>( project_combine( this->pos(), pos ) );

        if( new_id.is_valid() ) {
            DebugLog( D_WARNING, DC_ALL ) << "migrated oter_id '" << old_id << "' at " << pos_abs
                                          << " to '" << new_id.str() << "'";

            ter_set( pos, new_id );
        } else {
            debugmsg( "oter_id migration defined from '%s' to invalid ter_id '%s'", old_id, new_id.str() );
        }
    }
}

void overmap::migrate_camps( const std::vector<tripoint_abs_omt> &points ) const
{
    for( const tripoint_abs_omt &point : points ) {




        if( camps.find( point.xy() ) != camps.end() ) {
            continue;
        }
        std::optional<basecamp *> bcp = overmap_buffer.find_camp( point.xy() );
        if( bcp ) {
            continue;
        }
        get_map().add_camp( point, "faction_camp", false );
        bcp = overmap_buffer.find_camp( point.xy() );
        if( !bcp ) {
            debugmsg( "Camp placement failed during migration?!" );
            continue;
        }
        basecamp *temp_camp = *bcp;
        const oter_type_str_id &keyvalue = ter( project_remain<coords::om>
                                                ( point ).remainder_tripoint )->get_type_id();
        temp_camp->set_owner( camp_migration_map[keyvalue].second );
        temp_camp->set_name( camp_migration_map[keyvalue].first.translated() );
        temp_camp->define_camp( point, "faction_base_bare_bones_NPC_camp_0", false );
    }
}

oter_id overmap::get_or_migrate_oter( const std::string &oterid )
{
    auto migration = oter_id_migrations.find( oterid );
    if( migration != oter_id_migrations.end() ) {
        return oter_id( migration->second );
    } else {
        return oter_id( oterid );
    }
}

void overmap::place_special_forced( const overmap_special_id &special_id,
                                    const tripoint_om_omt &p,
                                    om_direction::type dir )
{


    static const city invalid_city( "" );
    place_special( *special_id, p, dir, invalid_city, false, true );
}



void mongroup::wander( const overmap &om )
{
    const city *target_city = nullptr;
    int target_distance = 0;

    point_om_sm rel_p = rel_pos().xy();

    if( behaviour == horde_behaviour::city ) {

        for( const city &check_city : om.cities ) {

            int distance = rl_dist( project_to<coords::sm>( check_city.pos ), rel_p );
            if( !target_city || distance < target_distance ) {
                target_distance = distance;
                target_city = &check_city;
            }
        }
    }

    if( target_city ) {



        point_abs_sm target_abs =
            project_to<coords::sm>( project_combine( om.pos(), target_city->pos ) );
        int range = target_city->size * 2;
        point_rel_sm delta( rng( -range, range ), rng( -range, range ) );
        target = target_abs + delta;
        interest = 100;
    } else {


        target = abs_pos.xy() + point_rel_sm( rng( -10, 10 ), rng( -10, 10 ) );
        interest = 30;
    }
}

horde_entity &overmap::spawn_monster( const tripoint_abs_ms &p, mtype_id id )
{
    return ( *hordes.spawn_entity( p, id ) )->second;
}




std::optional<tripoint_om_ms> overmap::find_open_space_in_submap( const tripoint_om_ms
        &submap_origin, point_rel_ms &cursor )
{
    for( ; cursor.y() < SEEX; cursor.y()++ ) {
        do {
            if( passable( submap_origin + cursor ) ) {
                return { submap_origin + cursor };
            }


            if( ++cursor.x() >= SEEX ) {
                cursor.x() = 0;
                break;
            }
        } while( true );
    }

    return std::optional<tripoint_om_ms>();
}

void overmap::spawn_monsters( const tripoint_om_sm &p, std::vector<monster> &monsters )
{
    tripoint_om_ms submap_origin = project_to<coords::ms>( p );
    point_rel_ms cursor{ 0, 0 };
    for( monster &mon_to_spawn : monsters ) {
        std::optional<tripoint_om_ms> open_space =
            find_open_space_in_submap( submap_origin, cursor );
        if( !open_space ) {

            return;
        }
        hordes.spawn_entity( project_combine( pos(), *open_space ), mon_to_spawn );
    }
}

void overmap::spawn_mongroup( const tripoint_om_sm &p, const mongroup_id &type, int count )
{
    tripoint_om_ms submap_origin = project_to<coords::ms>( p );
    point_rel_ms cursor{ 0, 0 };
    while( count > 0 ) {
        for( MonsterGroupResult &result : MonsterGroupManager::GetResultFromGroup( type, &count ) ) {
            for( int i = 0; i < result.pack_size; ++i ) {

                std::optional<tripoint_om_ms> open_space =
                    find_open_space_in_submap( submap_origin, cursor );
                if( !open_space ) {

                    return;
                }
                hordes.spawn_entity( project_combine( pos(), *open_space ), result.id );
            }
        }
    }
}

horde_entity *overmap::entity_at( const tripoint_om_ms &p )
{
    return hordes.entity_at( p );
}


std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> overmap::hordes_at(
    const tripoint_om_omt &p, int filter )
{
    return hordes.entity_group_at( p, filter );
}





void overmap::move_hordes()
{


    std::unordered_map<tripoint_abs_ms, horde_entity> migrating_hordes;
    for( horde_map::iterator mon = hordes.get_view( horde_map_flavors::active ).begin(),
         mon_end = hordes.end(); mon != mon_end; ) {




        if( mon->second.last_processed == calendar::turn ) {
            mon++;
            continue;
        }
        mon->second.last_processed = calendar::turn;

        if( mon->second.tracking_intensity > 0 && mon->first != mon->second.destination ) {
            mon->second.tracking_intensity--;
            mon->second.moves += mon->second.type_id->speed;
            if( mon->second.moves <= 0 ) {
                mon++;
                continue;
            }
            std::vector<tripoint_abs_ms> viable_candidates;

            for( const tripoint_abs_ms &candidate :
                 squares_closer_to( mon->first, mon->second.destination ) ) {

                if( candidate.z() == mon->first.z() && overmap_buffer.passable( candidate ) ) {
                    viable_candidates.push_back( candidate );
                }
            }
            if( viable_candidates.empty() ) {


                mon++;
                continue;
            }

            mon->second.moves -= 100;
            if( viable_candidates.front() == mon->second.destination ) {
                mon->second.tracking_intensity = 0;
            }


            if( get_map().inbounds( viable_candidates.front() ) ) {
                monster *placed_monster = nullptr;
                if( mon->second.monster_data ) {
                    placed_monster = g->place_critter_around( make_shared_fast<monster>( *mon->second.monster_data ),
                                     get_map().get_bub( viable_candidates.front() ), 1 );
                } else {
                    placed_monster = g->place_critter_around( mon->second.type_id->id,
                                     get_map().get_bub( viable_candidates.front() ), 1 );
                }
                if( placed_monster == nullptr ) {

                    mon++;
                    continue;
                }

                if( mon->second.tracking_intensity > 0 ) {
                    placed_monster->wander_to( mon->second.destination, mon->second.tracking_intensity );
                }
                mon = hordes.erase( mon );
                continue;
            }

            horde_map::iterator moving_mon = mon;

            mon++;
            auto monster_node = hordes.extract( moving_mon );
            monster_node.key() = viable_candidates.front();
            migrating_hordes.insert( std::move( monster_node ) );
        }
    }
    while( !migrating_hordes.empty() ) {
        auto monster_node = migrating_hordes.extract( migrating_hordes.begin() );
        point_abs_om dest_omp;
        tripoint_om_sm dest_sm;
        std::tie( dest_omp, dest_sm ) = project_remain<coords::om>( project_to<coords::sm>
                                        ( monster_node.key() ) );
        overmap *dest_om = overmap_buffer.get_existing( dest_omp );
        if( dest_om == nullptr ) {
            debugmsg( "A horde entity tried to wander into a non-existent overmap." );
            continue;
        }
        dest_om->hordes.insert( std::move( monster_node ) );
    }
}






void overmap::move_nemesis()
{

    decltype( zg ) tmpzg;

    for( std::multimap<tripoint_om_sm, mongroup>::iterator it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( !mg.horde || mg.behaviour != mongroup::horde_behaviour::nemesis ) {
            ++it;
            continue;
        }

        point_abs_om omp;
        tripoint_om_sm local_sm;
        std::tie( omp, local_sm ) = project_remain<coords::om>( mg.abs_pos );



        const oter_id &walked_into = ter( project_to<coords::omt>( local_sm ) );
        int movement_chance = 25;
        if( is_ot_match( "road", walked_into, ot_match_type::contains ) ) {
            movement_chance = 1;
        } else if( is_ot_match( "field", walked_into, ot_match_type::contains ) ) {
            movement_chance = 3;
        } else if( is_ot_match( "forest", walked_into, ot_match_type::prefix ) ||
                   is_ot_match( "swamp", walked_into, ot_match_type::prefix ) ||
                   is_ot_match( "bridge", walked_into, ot_match_type::prefix ) ) {
            movement_chance = 6;
        } else if( is_ot_match( "river", walked_into, ot_match_type::prefix ) ) {
            movement_chance = 10;
        }


        if( one_in( movement_chance ) && rng( 0, 200 ) < mg.avg_speed() ) {
            if( mg.abs_pos.x() > mg.nemesis_target.x() ) {
                mg.abs_pos.x()--;
            }
            if( mg.abs_pos.x() < mg.nemesis_target.x() ) {
                mg.abs_pos.x()++;
            }
            if( mg.abs_pos.y() > mg.nemesis_target.y() ) {
                mg.abs_pos.y()--;
            }
            if( mg.abs_pos.y() < mg.nemesis_target.y() ) {
                mg.abs_pos.y()++;
            }



            if( project_to<coords::om>( mg.nemesis_target ) == omp ) {


                tmpzg.emplace( mg.rel_pos(), mg );
                zg.erase( it++ );


                break;

            }


            break;

        } else {

            break;
        }
    }

    zg.insert( tmpzg.begin(), tmpzg.end() );

}

bool overmap::remove_nemesis()
{

    for( std::multimap<tripoint_om_sm, mongroup>::iterator it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( mg.behaviour == mongroup::horde_behaviour::nemesis ) {
            zg.erase( it++ );
            return true;
        }
        ++it;
    }
    return false;
}







void overmap::signal_hordes( const tripoint_abs_ms &p, const int sig_power )
{
    tripoint_abs_sm absp = project_to<coords::sm>( p );
    for( auto &elem : zg ) {
        mongroup &mg = elem.second;
        if( !mg.horde ) {
            continue;
        }
        const int dist = rl_dist( absp, mg.abs_pos );
        if( sig_power < dist ) {
            continue;
        }
        if( mg.behaviour == mongroup::horde_behaviour::nemesis ) {

            continue;
        }

        const int inter_per_sig_power = 15;
        const int min_initial_inter = 30;
        const int calculated_inter = ( sig_power + 1 - dist ) * inter_per_sig_power;
        const int roll = rng( 0, mg.interest );

        const int min_capped_inter = std::max( min_initial_inter, calculated_inter );
        if( roll < min_capped_inter ) {

            const int targ_dist = rl_dist( absp.xy(), mg.target );

            if( targ_dist < 5 ) {
                mg.set_target( midpoint( mg.target, absp.xy() ) );
                const int min_inc_inter = 3;
                const int inc_roll = rng( min_inc_inter, calculated_inter );
                mg.inc_interest( inc_roll );
                add_msg_debug( debugmode::DF_OVERMAP, "horde inc interest %d dist %d", inc_roll, dist );
            } else {
                mg.set_target( absp.xy() );
                mg.set_interest( min_capped_inter );
                add_msg_debug( debugmode::DF_OVERMAP, "horde set interest %d dist %d", min_capped_inter, dist );
            }
        }
    }
    hordes.signal_entities( p, sig_power );
}

void overmap::alert_entity( const tripoint_om_ms &location, const tripoint_abs_ms &destination,
                            int intensity )
{
    horde_map::iterator target = hordes.find( location );
    if( target != hordes.end() && intensity > target->second.tracking_intensity ) {
        auto monster_node = hordes.extract( target );
        monster_node.mapped().tracking_intensity = intensity;
        monster_node.mapped().destination = destination;
        hordes.insert( std::move( monster_node ) );
    }
}

void overmap::signal_nemesis( const tripoint_abs_sm &p_abs_sm )
{
    for( std::pair<const tripoint_om_sm, mongroup> &elem : zg ) {
        mongroup &mg = elem.second;

        if( mg.behaviour == mongroup::horde_behaviour::nemesis ) {

            mg.set_target( p_abs_sm.xy() );
            mg.set_nemesis_target( p_abs_sm.xy() );

        }
    }
}

void overmap::populate_connections_out_from_neighbors( const std::vector<const overmap *>
        &neighbor_overmaps )
{
    const auto populate_for_side =
        [&]( const overmap * adjacent,
             const std::function<bool( const tripoint_om_omt & )> &should_include,
    const std::function<tripoint_om_omt( const tripoint_om_omt & )> &build_point ) {
        if( adjacent == nullptr ) {
            return;
        }

        for( const std::pair<const string_id<overmap_connection>, std::vector<tripoint_om_omt>> &kv :
             adjacent->connections_out ) {
            std::vector<tripoint_om_omt> &out = connections_out[kv.first];
            const auto adjacent_out = adjacent->connections_out.find( kv.first );
            if( adjacent_out != adjacent->connections_out.end() ) {
                for( const tripoint_om_omt &p : adjacent_out->second ) {
                    if( should_include( p ) ) {
                        out.push_back( build_point( p ) );
                    }
                }
            }
        }
    };

    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::north )], [](
    const tripoint_om_omt & p ) {
        return p.y() == OMAPY - 1;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( p.x(), 0, p.z() );
    } );
    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::west )], [](
    const tripoint_om_omt & p ) {
        return p.x() == OMAPX - 1;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( 0, p.y(), p.z() );
    } );
    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::south )], [](
    const tripoint_om_omt & p ) {
        return p.y() == 0;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( p.x(), OMAPY - 1, p.z() );
    } );
    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::east )], [](
    const tripoint_om_omt & p ) {
        return p.x() == 0;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( OMAPX - 1, p.y(), p.z() );
    } );
}

void overmap::place_forest_trails()
{
    std::unordered_set<point_om_omt> visited;

    const auto is_forest = [&]( const point_om_omt & p ) {
        if( !inbounds( p, 1 ) ) {
            return false;
        }
        const oter_id current_terrain = ter( tripoint_om_omt( p, 0 ) );
        return current_terrain == oter_forest || current_terrain == oter_forest_thick ||
               current_terrain == oter_forest_water;
    };
    const region_settings_forest_trail &forest_trail = settings->get_settings_forest_trail();

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt seed_point( i, j, 0 );

            oter_id oter = ter( seed_point );
            if( !is_ot_match( "forest", oter, ot_match_type::prefix ) ) {
                continue;
            }



            if( visited.find( seed_point.xy() ) != visited.end() ) {
                continue;
            }


            std::vector<point_om_omt> forest_points =
                ff::point_flood_fill_4_connected<std::vector>( seed_point.xy(), visited, is_forest );


            if( forest_points.empty() ||
                forest_points.size() < static_cast<size_t>
                ( forest_trail.minimum_forest_size ) ) {
                continue;
            }


            if( !one_in( forest_trail.chance ) ) {
                continue;
            }


            auto north_south_most = std::minmax_element( forest_points.begin(),
            forest_points.end(), []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.y() < rhs.y();
            } );


            auto west_east_most = std::minmax_element( forest_points.begin(),
            forest_points.end(), []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.x() < rhs.x();
            } );



            point_om_omt northmost = *north_south_most.first;
            point_om_omt southmost = *north_south_most.second;
            point_om_omt westmost = *west_east_most.first;
            point_om_omt eastmost = *west_east_most.second;




            point_om_omt center( westmost.x() + ( eastmost.x() - westmost.x() ) / 2,
                                 northmost.y() + ( southmost.y() - northmost.y() ) / 2 );

            point_om_omt center_point = center;





            point_om_omt actual_center_point =
                *std::min_element( forest_points.begin(), forest_points.end(),
            [&center_point]( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return square_dist( lhs, center_point ) < square_dist( rhs,
                        center_point );
            } );



            int max_random_points = forest_trail.random_point_min + forest_points.size() /
                                    forest_trail.random_point_size_scalar;
            max_random_points = std::min( max_random_points, forest_trail.random_point_max );


            std::vector<point_om_omt> chosen_points = { actual_center_point };


            int random_point_count = 0;
            std::shuffle( forest_points.begin(), forest_points.end(), rng_get_engine() );
            for( const auto &random_point : forest_points ) {
                if( random_point_count >= max_random_points ) {
                    break;
                }
                random_point_count++;
                chosen_points.emplace_back( random_point );
            }


            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( northmost );
            }
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( southmost );
            }
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( westmost );
            }
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( eastmost );
            }


            const overmap_connection_id &overmap_connection_forest_trail =
                settings->overmap_connection.trail_connection;
            connect_closest_points( chosen_points, 0, *overmap_connection_forest_trail );
        }
    }
}

void overmap::place_forest_trailheads()
{

    const int city_size = settings->get_settings_city().city_size;
    if( city_size <= 0 ) {
        return;
    }

    const region_settings_forest_trail &settings_forest_trail = settings->get_settings_forest_trail();







    const auto trailhead_close_to_road = [&]( const tripoint_om_omt & trailhead ) {
        bool close = false;
        for( const tripoint_om_omt &nearby_point : closest_points_first(
                 trailhead,
                 settings_forest_trail.trailhead_road_distance
             ) ) {
            if( check_ot( "road", ot_match_type::contains, nearby_point ) ) {
                close = true;
            }
        }
        return close;
    };

    const auto try_place_trailhead_special = [&]( const tripoint_om_omt & trail_end,
    const om_direction::type & dir ) {
        overmap_special_id trailhead = settings_forest_trail.trailheads.pick();
        if( one_in( settings_forest_trail.trailhead_chance ) &&
            trailhead_close_to_road( trail_end ) &&
            can_place_special( *trailhead, trail_end, dir, false ) ) {
            const city &nearest_city = get_nearest_city( trail_end );
            place_special( *trailhead, trail_end, dir, nearest_city, false, false );
        }
    };

    for( int i = 2; i < OMAPX - 2; i++ ) {
        for( int j = 2; j < OMAPY - 2; j++ ) {
            const tripoint_om_omt p( i, j, 0 );
            oter_id oter = ter( p );
            if( is_ot_match( "forest_trail_end", oter, ot_match_type::prefix ) ) {
                try_place_trailhead_special( p, static_cast<om_direction::type>( oter->get_rotation() ) );
            }
        }
    }
}

void overmap::place_forests()
{
    const region_settings_forest &settings_forest = settings->get_settings_forest();
    const oter_id default_oter_id( settings->default_oter[OVERMAP_DEPTH] );
    const om_noise::om_noise_layer_forest f( global_base_point(), g->get_seed() );

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt p( x, y, 0 );
            const oter_id &oter = ter( p );



            if( oter != default_oter_id ) {
                continue;
            }

            const float n = f.noise_at( p.xy() );


            if( n + forest_size_adjust > settings_forest.noise_threshold_forest_thick ) {
                ter_set( p, oter_forest_thick );
            } else if( n + forest_size_adjust > settings_forest.noise_threshold_forest ) {
                ter_set( p, oter_forest );
            }
        }
    }
}

bool overmap::omt_lake_noise_threshold( const point_abs_omt &origin, const point_om_omt &offset,
                                        const double noise_threshold )
{
    const om_noise::om_noise_layer_lake noise_func( origin, g->get_seed() );

    bool inbounds = offset.x() > -5 && offset.y() > -5 && offset.x() < OMAPX + 5 &&
                    offset.y() < OMAPY + 5;
    if( !inbounds ) {
        return false;
    }
    return noise_func.noise_at( offset ) > noise_threshold;
}

bool overmap::guess_has_lake( const point_abs_om &p, const double noise_threshold,
                              int max_tile_count )
{
    const point_abs_omt origin = project_to<coords::omt>( p );
    const om_noise::om_noise_layer_lake noise_func( origin, g->get_seed() );

    int lake_tiles = 0;
    for( int i = 0; i < OMAPX && lake_tiles < max_tile_count; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            const point_om_omt seed_point( i, j );
            if( omt_lake_noise_threshold( origin, seed_point, noise_threshold ) ) {
                lake_tiles++;
                if( lake_tiles >= max_tile_count ) {
                    break;
                }
            }
        }
    }
    return lake_tiles >= max_tile_count;
}

void overmap::place_swamps()
{
    const region_settings_forest &settings_forest = settings->get_settings_forest();



    std::unique_ptr<cata::mdarray<int, point_om_omt>> floodptr =
                std::make_unique<cata::mdarray<int, point_om_omt>>( 0 );
    cata::mdarray<int, point_om_omt> &floodplain = *floodptr;
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt pos( x, y, 0 );

            if( is_ot_match( "river", ter_unsafe( pos ), ot_match_type::contains ) ) {
                std::vector<point_om_omt> buffered_points =
                    closest_points_first(
                        pos.xy(),
                        rng( settings_forest.river_floodplain_buffer_distance_min,
                             settings_forest.river_floodplain_buffer_distance_max ) );
                for( const point_om_omt &p : buffered_points )  {
                    if( !inbounds( p ) ) {
                        continue;
                    }
                    floodplain[p] += 1;
                }
            }
        }
    }


    const om_noise::om_noise_layer_floodplain f( global_base_point(), g->get_seed() );

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt pos( x, y, 0 );


            if( !is_ot_match( "forest", ter( pos ), ot_match_type::contains ) ) {
                continue;
            }



            const bool should_flood = ( floodplain[x][y] > 0 && !one_in( floodplain[x][y] ) && f.noise_at( { x, y } )
                                        > settings_forest.noise_threshold_swamp_adjacent_water );



            const bool should_isolated_swamp = f.noise_at( pos.xy() ) >
                                               settings_forest.noise_threshold_swamp_isolated;
            if( should_flood || should_isolated_swamp )  {
                ter_set( pos, oter_forest_water );
            }
        }
    }
}

void overmap::place_roads( const std::vector<const overmap *> &neighbor_overmaps )
{
    int op_city_size = settings->get_settings_city().city_size;
    if( op_city_size <= 0 ) {
        return;
    }
    const overmap_connection_id &overmap_connection_inter_city_road =
        settings->overmap_connection.inter_city_road_connection;
    std::vector<tripoint_om_omt> &roads_out = connections_out[overmap_connection_inter_city_road];


    if( roads_out.size() < 3 ) {
        for( const om_direction::type dir : om_direction::all ) {

            if( neighbor_overmaps[static_cast<int>( dir )] == nullptr ) {

                std::vector<tripoint_om_omt> border = get_border( dir, 0, 10 );
                std::shuffle( border.begin(), border.end(), rng_get_engine() );
                for( const tripoint_om_omt &p : border ) {

                    if( !( is_river( ter( p ) ) ||


                           is_river( ter( p + point_rel_omt(
                                              four_adjacent_offsets[static_cast<int>( om_direction::turn_right( dir ) )] ) ) ) ||
                           is_river( ter( p + point_rel_omt(
                                              four_adjacent_offsets[static_cast<int>( om_direction::turn_left( dir ) )] ) ) ) ) ) {
                        roads_out.push_back( p );
                        break;
                    }
                }
                if( roads_out.size() == 3 ) {
                    break;
                }
            }
        }
    }

    std::vector<point_om_omt> road_points;

    road_points.reserve( roads_out.size() + std::max( 1, static_cast<int>( cities.size() ) ) );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.xy() );
    }
    if( cities.empty() ) {

        fallback_road_connection_point = point_om_omt( rng( OMAPX / 4, ( 3 * OMAPX ) / 4 ),
                                         rng( OMAPY / 4, ( 3 * OMAPY ) / 4 ) );
        road_points.emplace_back( *fallback_road_connection_point );
    } else {
        for( const city &elem : cities ) {
            road_points.emplace_back( elem.pos );
        }
    }


    connect_closest_points( road_points, 0, *overmap_connection_inter_city_road );
}

void overmap::place_railroads( const std::vector<const overmap *> &neighbor_overmaps )
{

    int op_city_size = settings->get_settings_city().city_size;
    if( op_city_size <= 0 ) {
        return;
    }
    const overmap_connection_id &overmap_connection_local_railroad =
        settings->overmap_connection.rail_connection;
    std::vector<tripoint_om_omt> &railroads_out = connections_out[overmap_connection_local_railroad];


    if( railroads_out.size() < 3 ) {




        static constexpr std::array<int, 4> edge_coords_x = {OMAPX - 1, -1, 0, -1};
        static constexpr std::array<int, 4> edge_coords_y = {-1, OMAPY - 1, -1, 0};


        std::array < int, OMAPX - 20 > omap_num;
        for( int i = 0; i < OMAPX - 20; i++ ) {
            omap_num[i] = i + 10;
        }

        std::array < size_t, 4 > dirs = {0, 1, 2, 3};
        std::shuffle( dirs.begin(), dirs.end(), rng_get_engine() );

        for( size_t dir : dirs ) {

            if( neighbor_overmaps[dir] == nullptr ) {
                std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
                for( const int &i : omap_num ) {
                    tripoint_om_omt tmp = tripoint_om_omt(
                                              edge_coords_x[dir] >= 0 ? edge_coords_x[dir] : i,
                                              edge_coords_y[dir] >= 0 ? edge_coords_y[dir] : i,
                                              0 );

                    if( !( is_river( ter( tmp ) ) ||


                           is_river( ter( tmp + point_rel_omt( four_adjacent_offsets[( dir + 1 ) % 4] ) ) ) ||
                           is_river( ter( tmp + point_rel_omt( four_adjacent_offsets[( dir + 3 ) % 4] ) ) ) ) ) {
                        railroads_out.push_back( tmp );
                        break;
                    }
                }
                if( railroads_out.size() == 3 ) {
                    break;
                }
            }
        }
    }

    std::vector<point_om_omt> railroad_points;

    railroad_points.reserve( railroads_out.size() + cities.size() );
    for( const auto &elem : railroads_out ) {
        railroad_points.emplace_back( elem.xy() );
    }
    for( const city &elem : cities ) {

        railroad_points.emplace_back( random_entry(
                                          points_in_radius( tripoint_om_omt( elem.pos, 0 ), elem.size * 4 ) ).xy()
                                    );
    }


    connect_closest_points( railroad_points, 0, *overmap_connection_local_railroad );
}

std::vector<tripoint_om_omt> overmap::get_neighbor_border( const point_rel_om &direction, int z,
        int distance_corner )
{
    const int OMAPX_edge = OMAPX - 1;
    const int OMAPY_edge = OMAPY - 1;

    tripoint_om_omt edges( direction.x() >= 0 ? 0 : OMAPX_edge, direction.y() >= 0 ? 0 : OMAPY_edge,
                           0 );
    bool iterx = direction.x() == 0;
    int corner_upper = iterx ? OMAPX_edge - distance_corner : OMAPY_edge - distance_corner;

    return iterx ?
           line_to( tripoint_om_omt( distance_corner, edges.y(), 0 ), tripoint_om_omt( corner_upper, edges.y(),
                    z ) ) :
           line_to( tripoint_om_omt( edges.x(), distance_corner, 0 ), tripoint_om_omt( edges.x(), corner_upper,
                    z ) );
}

std::vector<tripoint_om_omt> overmap::get_border( const point_rel_om &direction, int z,
        int distance_corner )
{
    point_rel_om flip_direction( -direction.x(), -direction.y() );
    return get_neighbor_border( flip_direction, z, distance_corner );
}

std::vector<tripoint_om_omt> overmap::get_border( const om_direction::type direction, int z,
        int distance_corner )
{
    return get_border( point_rel_om( four_adjacent_offsets[static_cast<int>( direction )] ), z,
                       distance_corner );
}

void overmap::calculate_forestosity()
{
    if( !settings->overmap_forest ) {
        forest_size_adjust = 0;
        forestosity = 0;
        return;
    }
    const region_settings_forest &settings_forest = settings->get_settings_forest();
    float northern_forest_increase = settings_forest.forest_increase[static_cast<int>
                                     ( om_direction::type::north )];
    float eastern_forest_increase = settings_forest.forest_increase[static_cast<int>
                                    ( om_direction::type::east )];
    float western_forest_increase = settings_forest.forest_increase[static_cast<int>
                                    ( om_direction::type::west )];
    float southern_forest_increase = settings_forest.forest_increase[static_cast<int>
                                     ( om_direction::type::south )];
    const point_abs_om this_om = pos();
    if( western_forest_increase != 0 && this_om.x() < 0 ) {
        forest_size_adjust -= this_om.x() * western_forest_increase;
    }
    if( northern_forest_increase != 0 && this_om.y() < 0 ) {
        forest_size_adjust -= this_om.y() * northern_forest_increase;
    }
    if( eastern_forest_increase != 0 && this_om.x() > 0 ) {
        forest_size_adjust += this_om.x() * eastern_forest_increase;
    }
    if( southern_forest_increase != 0 && this_om.y() > 0 ) {
        forest_size_adjust += this_om.y() * southern_forest_increase;
    }
    forestosity = forest_size_adjust * 25.0f;


    forest_size_adjust = std::min<float>( forest_size_adjust,
                                          settings_forest.max_forest - settings_forest.noise_threshold_forest );
}

void overmap::calculate_urbanity()
{
    int op_city_size = settings->get_settings_city().city_size;
    if( op_city_size <= 0 ) {
        return;
    }
    int northern_urban_increase = settings->urban_increase[static_cast<int>
                                  ( om_direction::type::north )];
    int eastern_urban_increase = settings->urban_increase[static_cast<int>( om_direction::type::east )];
    int western_urban_increase = settings->urban_increase[static_cast<int>( om_direction::type::west )];
    int southern_urban_increase = settings->urban_increase[static_cast<int>
                                  ( om_direction::type::south )];
    if( northern_urban_increase == 0 && eastern_urban_increase == 0 && western_urban_increase == 0 &&
        southern_urban_increase == 0 ) {
        return;
    }
    float urbanity_adj = 0.0f;

    const point_abs_om this_om = pos();
    if( northern_urban_increase != 0 && this_om.y() < 0 ) {
        urbanity_adj -= this_om.y() * northern_urban_increase / 10.0f;


        if( this_om.x() < 0 && western_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / -2.0f, 1.0f );
        }
        if( this_om.x() > 0 && eastern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / 2.0f, 1.0f );
        }
    }
    if( eastern_urban_increase != 0 && this_om.x() > 0 ) {
        urbanity_adj += this_om.x() * eastern_urban_increase / 10.0f;
        if( this_om.y() < 0 && northern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.y() / -2.0f, 1.0f );
        }
        if( this_om.y() > 0 && southern_urban_increase == 0 ) {
            urbanity_adj /= std::max( this_om.y() / 2.0f, 1.0f );
        }
    }
    if( western_urban_increase != 0 && this_om.x() < 0 ) {
        urbanity_adj -= this_om.x() * western_urban_increase / 10.0f;
        if( this_om.y() < 0 && northern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.y() / -2.0f, 1.0f );
        }
        if( this_om.y() > 0 && southern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.y() / 2.0f, 1.0f );
        }
    }
    if( southern_urban_increase != 0 && this_om.y() > 0 ) {
        urbanity_adj += this_om.y() * southern_urban_increase / 10.0f;
        if( this_om.x() < 0 && western_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / -2.0f, 1.0f );
        }
        if( this_om.x() > 0 && eastern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / 2.0f, 1.0f );
        }
    }
    urbanity = static_cast<int>( urbanity_adj );

}

void overmap::place_ravines()
{
    const region_settings_ravine &settings_ravine = settings->get_settings_ravine();
    if( settings_ravine.num_ravines == 0 ) {
        return;
    }

    const oter_id rift( "ravine" );
    const oter_id rift_edge( "ravine_edge" );
    const oter_id rift_floor = settings_ravine.ravine_bottom_terrain_str_id.id();
    const oter_id rift_floor_edge( "ravine_floor_edge" );

    std::set<point_om_omt> rift_points;



    const pf::two_node_scoring_fn<point_om_omt> estimate =
    [&]( pf::directed_node<point_om_omt>, const std::optional<pf::directed_node<point_om_omt>> & ) {
        return pf::node_score( 0, rng( 1, 2 ) );
    };



    const int ravine_range = settings_ravine.ravine_range;
    const int ravine_width = settings_ravine.ravine_width;
    const int ravine_depth = settings_ravine.ravine_depth;

    for( int n = 0; n < settings_ravine.num_ravines; n++ ) {
        const point_rel_omt offset( rng( -ravine_range,
                                         ravine_range ),
                                    rng( -ravine_range, ravine_range ) );
        const point_om_omt origin( rng( 0, OMAPX ), rng( 0, OMAPY ) );
        const point_om_omt destination = origin + offset;
        if( !inbounds( destination, ravine_width * 3 ) ) {
            continue;
        }
        const auto path = pf::greedy_path( origin, destination, point_om_omt( OMAPX, OMAPY ), estimate );
        for( const auto &node : path.nodes ) {
            for( int i = 1 - ravine_width; i < ravine_width;
                 i++ ) {
                for( int j = 1 - ravine_width; j < ravine_width;
                     j++ ) {
                    const point_om_omt n = node.pos + point( j, i );
                    if( inbounds( n, 1 ) ) {
                        rift_points.emplace( n );
                    }
                }
            }
        }
    }




    for( const point_om_omt &p : rift_points ) {
        bool edge = false;
        for( int ni = -1; ni <= 1 && !edge; ni++ ) {
            for( int nj = -1; nj <= 1 && !edge; nj++ ) {
                const point_om_omt n = p + point_rel_omt( ni, nj );
                if( rift_points.find( n ) == rift_points.end() || !inbounds( n ) ) {
                    edge = true;
                }
            }
        }
        for( int z = 0; z >= ravine_depth; z-- ) {
            if( z == ravine_depth ) {
                ter_set( tripoint_om_omt( p, z ), edge ? rift_floor_edge : rift_floor );
            } else {
                ter_set( tripoint_om_omt( p, z ), edge ? rift_edge : rift );
            }
        }
    }

}

pf::directed_path<point_om_omt> overmap::lay_out_connection(
    const overmap_connection &connection, const point_om_omt &source, const point_om_omt &dest,
    int z, const bool must_be_unexplored ) const
{
    const pf::two_node_scoring_fn<point_om_omt> estimate =
    [&]( pf::directed_node<point_om_omt> cur, std::optional<pf::directed_node<point_om_omt>> prev ) {
        const oter_id id = ter( tripoint_om_omt( cur.pos, z ) );

        const overmap_connection::subtype *subtype = connection.pick_subtype_for( id );

        if( !subtype ) {
            return pf::node_score::rejected;
        }

        const bool existing_connection = connection.has( id );


        if( must_be_unexplored && !existing_connection ) {

            const bool existing_submap = is_omt_generated( tripoint_om_omt( cur.pos, z ) );



            if( existing_submap ) {
                return pf::node_score::rejected;
            }
        }


        if( subtype->is_perpendicular_crossing() && id->is_rotatable() &&
            om_direction::are_parallel( cur.dir, id->get_dir() ) ) {
            return pf::node_score::rejected;
        }

        if( existing_connection && id->is_rotatable() && cur.dir != om_direction::type::invalid &&
            !om_direction::are_parallel( id->get_dir(), cur.dir ) ) {
            return pf::node_score::rejected;
        }

        if( prev && prev->dir != om_direction::type::invalid && prev->dir != cur.dir ) {

            const oter_id &prev_id = ter( tripoint_om_omt( prev->pos, z ) );
            const overmap_connection::subtype *prev_subtype = connection.pick_subtype_for( prev_id );

            if( !prev_subtype || !prev_subtype->allows_turns() ) {
                return pf::node_score::rejected;
            }
        }

        const int dist = subtype->is_orthogonal() ?
                         manhattan_dist( dest, cur.pos ) :
                         trig_dist( dest, cur.pos );
        const int existency_mult = existing_connection ? 1 : 5;

        return pf::node_score( subtype->basic_cost, existency_mult * dist );
    };

    return pf::greedy_path( source, dest, point_om_omt( OMAPX, OMAPY ), estimate );
}

void overmap::build_connection(
    const overmap_connection &connection, const pf::directed_path<point_om_omt> &path, int z,
    const cube_direction initial_dir )
{
    if( path.nodes.empty() ) {
        return;
    }

    om_direction::type prev_dir =
        om_direction::from_cube( initial_dir, "Up and down connections not yet supported" );

    const pf::directed_node<point_om_omt> start = path.nodes.front();
    const pf::directed_node<point_om_omt> end = path.nodes.back();

    for( const auto &node : path.nodes ) {
        const tripoint_om_omt pos( node.pos, z );
        const oter_id &ter_id = ter( pos );
        const om_direction::type new_dir = node.dir;
        const overmap_connection::subtype *subtype = connection.pick_subtype_for( ter_id );

        if( !subtype ) {
            debugmsg( "No suitable subtype of connection \"%s\" found for \"%s\".", connection.id.c_str(),
                      ter_id.id().c_str() );
            return;
        }

        if( subtype->terrain->is_linear() ) {
            size_t new_line = connection.has( ter_id ) ? ter_id->get_line() : 0;

            if( new_dir != om_direction::type::invalid ) {
                new_line = om_lines::set_segment( new_line, new_dir );
            }

            if( prev_dir != om_direction::type::invalid ) {
                new_line = om_lines::set_segment( new_line, om_direction::opposite( prev_dir ) );
            }

            for( const om_direction::type dir : om_direction::all ) {
                const tripoint_om_omt np( pos + om_direction::displace( dir ) );

                if( inbounds( np ) ) {
                    const oter_id &near_id = ter( np );

                    if( connection.has( near_id ) ) {
                        if( near_id->is_linear() ) {
                            const size_t near_line = near_id->get_line();

                            if( om_lines::is_straight( near_line ) || om_lines::has_segment( near_line, new_dir ) ) {

                                const size_t new_near_line = om_lines::set_segment( near_line, om_direction::opposite( dir ) );
                                ter_set( np, near_id->get_type_id()->get_linear( new_near_line ) );
                                new_line = om_lines::set_segment( new_line, dir );
                            }
                        } else if( near_id->is_rotatable() && om_direction::are_parallel( dir, near_id->get_dir() ) ) {
                            new_line = om_lines::set_segment( new_line, dir );
                        }
                    }
                } else if( pos.xy() == start.pos || pos.xy() == end.pos ) {

                    new_line = om_lines::set_segment( new_line, dir );


                    std::vector<tripoint_om_omt> &outs = connections_out[connection.id];
                    const auto existing_out = std::find_if( outs.begin(),
                    outs.end(), [pos]( const tripoint_om_omt & c ) {
                        return c == pos;
                    } );
                    if( existing_out == outs.end() ) {
                        outs.emplace_back( pos );
                    }
                }
            }

            if( new_line == om_lines::invalid ) {
                debugmsg( "Invalid path for connection \"%s\".", connection.id.c_str() );
                return;
            }

            ter_set( pos, subtype->terrain->get_linear( new_line ) );
        } else if( new_dir != om_direction::type::invalid ) {
            ter_set( pos, subtype->terrain->get_rotated( new_dir ) );
        }

        prev_dir = new_dir;
    }
}

void overmap::build_connection(
    const point_om_omt &source, const point_om_omt &dest, int z,
    const overmap_connection &connection, const bool must_be_unexplored,
    const cube_direction initial_dir )
{
    build_connection(
        connection, lay_out_connection( connection, source, dest, z, must_be_unexplored ),
        z, initial_dir );
}



void overmap::connect_closest_points( const std::vector<point_om_omt> &points, int z,
                                      const overmap_connection &connection )
{

    using edge = std::pair<float, std::pair<size_t, size_t>>;

    if( points.size() < 2 ) {
        return;
    }

    std::vector<edge> edges;


    edges.reserve( points.size() * ( points.size() - 1 ) / 2 );
    for( size_t i = 0; i < points.size() - 1; ++i ) {
        for( size_t j = i + 1; j < points.size(); j++ ) {
            const float distance = trig_dist( points[i], points[j] );
            edges.push_back( {distance, {i, j}} );
        }
    }


    sort( edges.begin(), edges.end() );


    std::vector<int> subgraphs( points.size(), -1 );

    for( const edge &candidate : edges ) {
        const size_t i = candidate.second.first;
        const size_t j = candidate.second.second;
        bool connect = false;
        if( subgraphs[i] < 0 && subgraphs[j] < 0 ) {


            subgraphs[i] = i;
            subgraphs[j] = j;

            connect = true;
        } else if( subgraphs[i] < 0 ) {


            subgraphs[i] = subgraphs[j];

            connect = true;
        } else if( subgraphs[j] < 0 ) {


            subgraphs[j] = subgraphs[i];

            connect = true;
        } else if( subgraphs[i] != subgraphs[j] ) {


            int dead_subtree = subgraphs[j];
            for( size_t k = 0; k < points.size(); k++ ) {
                if( subgraphs[k] == dead_subtree ) {
                    subgraphs[k] = subgraphs[i];
                }
            }

            connect = true;
        } else if( one_in( 10 ) ) {


            connect = true;
        }

        if( connect ) {
            build_connection( points[i], points[j], z, connection, false );
        }
    }
}

void overmap::polish_river( const std::vector<const overmap *> &neighbor_overmaps )
{
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            build_river_shores( neighbor_overmaps, { x, y, 0 } );
        }
    }
}

bool overmap::check_ot( const std::string &otype, ot_match_type match_type,
                        const tripoint_om_omt &p ) const
{

    if( !inbounds( p ) ) {
        return false;
    }
    const oter_id &oter = ter( p );
    return is_ot_match( otype, oter, match_type );
}

bool overmap::check_overmap_special_type( const overmap_special_id &id,
        const tripoint_om_omt &location ) const
{

    auto found_id = overmap_special_placements.find( location );


    if( found_id == overmap_special_placements.end() ) {
        return false;
    }


    return found_id->second == id;
}

std::optional<overmap_special_id> overmap::overmap_special_at( const tripoint_om_omt &p ) const
{
    auto it = overmap_special_placements.find( p );

    if( it == overmap_special_placements.end() ) {
        return std::nullopt;
    }

    return it->second;
}

bool overmap::is_river_node( const point_om_omt &p ) const
{
    return !!get_river_node_at( p );
}

const overmap_river_node *overmap::get_river_node_at( const point_om_omt &p ) const
{
    for( const overmap_river_node &n : rivers ) {
        if( n.river_start == p || n.river_end == p ) {
            return &n;
        }
    }
    return nullptr;
}

std::string om_direction::name( type dir )
{
    static const std::array < std::string, size + 1 > names = {{
            translate_marker( "invalid" ), translate_marker( "north" ),
            translate_marker( "east" ), translate_marker( "south" ),
            translate_marker( "west" )
        }
    };
    return _( names[static_cast<size_t>( dir ) + 1] );
}













point om_direction::rotate( const point &p, type dir )
{
    switch( dir ) {
        case om_direction::type::invalid:
        case om_direction::type::last:
            debugmsg( "Invalid overmap rotation (%d).", static_cast<int>( dir ) );
            [[fallthrough]];
        case om_direction::type::north:
            break;
        case om_direction::type::east:
            return point( -p.y, p.x );
        case om_direction::type::south:
            return point( -p.x, -p.y );
        case om_direction::type::west:
            return point( p.y, -p.x );
    }
    return p;
}

tripoint om_direction::rotate( const tripoint &p, type dir )
{
    return tripoint( rotate( { p.xy() }, dir ), p.z );
}

uint32_t om_direction::rotate_symbol( uint32_t sym, type dir )
{
    return rotatable_symbols::get( sym, static_cast<int>( dir ) );
}

point_rel_omt om_direction::displace( type dir, int dist )
{
    return rotate( point_rel_omt{ 0, -dist }, dir );
}

static om_direction::type rotate_internal( om_direction::type dir, int step )
{
    if( dir == om_direction::type::invalid ) {
        debugmsg( "Can't rotate an invalid overmap rotation." );
        return dir;
    }
    step = step % om_direction::size;
    return static_cast<om_direction::type>( ( static_cast<int>( dir ) + step ) % om_direction::size );
}

om_direction::type om_direction::add( type dir1, type dir2 )
{
    return rotate_internal( dir1, static_cast<int>( dir2 ) );
}

om_direction::type om_direction::turn_left( type dir )
{
    return rotate_internal( dir, -static_cast<int>( size ) / 4 );
}

om_direction::type om_direction::turn_right( type dir )
{
    return rotate_internal( dir, static_cast<int>( size ) / 4 );
}

om_direction::type om_direction::turn_random( type dir )
{
    return rng( 0, 1 ) ? turn_left( dir ) : turn_right( dir );
}

om_direction::type om_direction::opposite( type dir )
{
    return rotate_internal( dir, static_cast<int>( size ) / 2 );
}

om_direction::type om_direction::random()
{
    return static_cast<type>( rng( 0, size - 1 ) );
}

bool om_direction::are_parallel( type dir1, type dir2 )
{
    return dir1 == dir2 || dir1 == opposite( dir2 );
}

om_direction::type om_direction::from_cube( cube_direction c, const std::string &error_msg )
{
    switch( c ) {
        case cube_direction::north:
            return om_direction::type::north;
        case cube_direction::east:
            return om_direction::type::east;
        case cube_direction::south:
            return om_direction::type::south;
        case cube_direction::west:
            return om_direction::type::west;
        case cube_direction::above:
        case cube_direction::below:
            debugmsg( error_msg );
            [[fallthrough]];
        case cube_direction::last:
            return om_direction::type::invalid;
    }
    debugmsg( "Invalid cube_direction" );
    return om_direction::type::invalid;
}

om_direction::type overmap::random_special_rotation( const overmap_special &special,
        const tripoint_om_omt &p, const bool must_be_unexplored ) const
{
    std::vector<om_direction::type> rotations( om_direction::size );
    const auto first = rotations.begin();
    auto last = first;

    int top_score = 0;


    for( om_direction::type r : om_direction::all ) {
        int score = special.score_rotation_at( *this, p, r );

        if( score >= top_score ) {
            if( score > top_score ) {
                top_score = score;
                last = first;
            }
            *last = r;
            ++last;
        }

        if( !special.is_rotatable() ) {
            break;
        }
    }

    std::shuffle( first, last, rng_get_engine() );
    const auto rotation = std::find_if( first, last, [&]( om_direction::type elem ) {
        return can_place_special( special, p, elem, must_be_unexplored );
    } );

    return rotation != last ? *rotation : om_direction::type::invalid;
}

void overmap::log_unique_special( const overmap_special_id &id )
{
    if( contains_unique_special( id ) ) {
        debugmsg( "Overmap unique overmap special placed more than once: %s", id.str() );
    }
    overmap_buffer.log_unique_special( id );
}

bool overmap::contains_unique_special( const overmap_special_id &id ) const
{
    return std::find_if( overmap_special_placements.begin(),
    overmap_special_placements.end(), [&id]( const auto & it ) {
        return it.second == id;
    } ) != overmap_special_placements.end();
}

bool overmap::can_place_special( const overmap_special &special, const tripoint_om_omt &p,
                                 om_direction::type dir, const bool must_be_unexplored ) const
{
    cata_assert( dir != om_direction::type::invalid );

    if( !special.id ) {
        return false;
    }
    if( ( special.has_flag( "GLOBALLY_UNIQUE" ) &&
          overmap_buffer.contains_unique_special( special.id ) ) ||
        ( special.has_flag( "OVERMAP_UNIQUE" ) && contains_unique_special( special.id ) ) ) {
        return false;
    }

    if( special.has_eoc() ) {
        dialogue d( get_talker_for( get_avatar() ), nullptr );
        if( !special.get_eoc()->test_condition( d ) ) {
            return false;
        }
    }




    const overmap_special_spawns &spawns = special.get_monster_spawns();
    if( spawns.group ) {
        const point_abs_omt base = coords::project_to<coords::omt>( this->pos() );
        for( int x = p.x() - spawns.radius.max; x <= p.x() + spawns.radius.max; x++ ) {
            for( int y = p.y() - spawns.radius.max; y <= p.y() + spawns.radius.max; y++ ) {
                const tripoint_abs_omt target = tripoint_abs_omt{ base, p.z() } + point_rel_omt{ x, y };
                if( overmap_buffer.get_existing( coords::project_to<coords::om>( target.xy() ) ) ) {
                    const std::optional<overmap_special_id> spec = overmap_buffer.overmap_special_at( target );
                    if( spec.has_value() &&
                        spec.value().obj().has_flag( "SAFE_AT_WORLDGEN" ) ) {
                        return false;
                    }
                }
            }
        }
    }

    const std::vector<overmap_special_locations> fixed_terrains = special.required_locations();

    return std::all_of( fixed_terrains.begin(), fixed_terrains.end(),
    [&]( const overmap_special_locations & elem ) {
        const tripoint_om_omt rp = p + om_direction::rotate( elem.p, dir );

        if( !inbounds( rp, 1 ) ) {
            return false;
        }

        if( must_be_unexplored ) {

            const bool existing_submap = is_omt_generated( rp );



            if( existing_submap ) {
                return false;
            }
        }

        const oter_id &tid = ter( rp );

        return elem.can_be_placed_on( tid ) || ( rp.z() != 0 && tid == get_default_terrain( rp.z() ) );
    } );
}


std::vector<tripoint_om_omt> overmap::place_special(
    const overmap_special &special, const tripoint_om_omt &p, om_direction::type dir,
    const city &cit, const bool must_be_unexplored, const bool force )
{
    cata_assert( dir != om_direction::type::invalid );
    if( !force ) {
        cata_assert( can_place_special( special, p, dir, must_be_unexplored ) );
    }

    if( special.has_flag( "GLOBALLY_UNIQUE" ) ) {
        overmap_buffer.add_unique_special( special.id );



    } else if( special.has_flag( "OVERMAP_UNIQUE" ) ) {
        log_unique_special( special.id );
    }


    const bool is_safe_zone = special.has_flag( "SAFE_AT_WORLDGEN" );

    std::optional<mapgen_arguments> *mapgen_args_p = &*mapgen_arg_storage.emplace();
    special_placement_result result = special.place( *this, p, dir, cit, must_be_unexplored );
    for( const std::pair<om_pos_dir, std::string> &join : result.joins_used ) {
        joins_used[join.first] = join.second;
    }
    for( const tripoint_om_omt &location : result.omts_used ) {
        overmap_special_placements[location] = special.id;
        mapgen_args_index[location] = mapgen_args_p;
        if( is_safe_zone ) {
            safe_at_worldgen.emplace( location );
        }
    }


    std::set<pp_generator_id> special_pp_gens;
    for( const tripoint_om_omt &location : result.omts_used ) {
        for( const pp_generator_id &gen_id : ter( location )->get_post_process_generators() ) {
            special_pp_gens.insert( gen_id );
        }
    }
    if( !special_pp_gens.empty() ) {
        std::vector<pp_resolved_generator> decisions;
        for( const pp_generator_id &gen_id : special_pp_gens ) {
            const pp_generator &gen = gen_id.obj();
            pp_resolved_generator resolved;
            resolved.generator = gen_id;
            std::map<sub_generator_type, uint8_t> type_counts;
            for( const pp_sub_generator &sg : gen.sub_generators() ) {
                const uint8_t ord = type_counts[sg.type];
                if( sg.scope == pp_sub_generator_scope::overmap_special ) {
                    pp_sub_decision dec;
                    dec.type = sg.type;
                    dec.ordinal = ord;
                    dec.st = pp_sub_decision::status::not_evaluated;
                    dec.seed = rng_bits();
                    resolved.sub_decisions.push_back( dec );
                }
                type_counts[sg.type]++;
            }
            if( !resolved.sub_decisions.empty() ) {
                decisions.push_back( std::move( resolved ) );
            }
        }
        if( !decisions.empty() ) {
            std::vector<pp_resolved_generator> *decisions_p =
                &*pp_decision_storage.emplace( std::move( decisions ) );
            for( const tripoint_om_omt &location : result.omts_used ) {
                pp_decisions_index[location] = decisions_p;
            }
        }
    }


    const overmap_special_spawns &spawns = special.get_monster_spawns();
    if( spawns.group ) {
        const int pop = rng( spawns.population.min, spawns.population.max );
        const int rad = rng( spawns.radius.min, spawns.radius.max );
        spawn_mon_group(
            mongroup( spawns.group, project_to<coords::sm>( project_combine( pos(), p ) ), pop ),
            rad );
    }

    if( is_safe_zone ) {
        for( auto it = zg.begin(); it != zg.end(); ) {
            tripoint_om_omt pos = project_to<coords::omt>( it->second.rel_pos() );
            if( safe_at_worldgen.find( pos ) != safe_at_worldgen.end() ) {
                zg.erase( it++ );
            } else {
                ++it;
            }
        }
    }

    return result.omts_used;
}

om_special_sectors get_sectors( const int sector_width )
{
    std::vector<point_om_omt> res;

    res.reserve( static_cast<size_t>( OMAPX / sector_width ) * static_cast<size_t>
                 ( OMAPY / sector_width ) );
    for( int x = 0; x < OMAPX; x += sector_width ) {
        for( int y = 0; y < OMAPY; y += sector_width ) {
            res.emplace_back( x, y );
        }
    }
    std::shuffle( res.begin(), res.end(), rng_get_engine() );
    return om_special_sectors{ res, sector_width };
}

bool overmap::place_special_attempt(
    overmap_special_batch &enabled_specials, const point_om_omt &sector,
    const int sector_width, const bool place_optional, const bool must_be_unexplored )
{
    const point_om_omt p2( sector );

    const tripoint_om_omt p( rng( p2.x(), p2.x() + sector_width - 1 ),
                             rng( p2.y(), p2.y() + sector_width - 1 ), 0 );
    const city &nearest_city = get_nearest_city( p );

    std::shuffle( enabled_specials.begin(), enabled_specials.end(), rng_get_engine() );
    std::set<int> priorities;
    for( const overmap_special_placement &os : enabled_specials ) {
        priorities.emplace( os.special_details->get_priority() );
    }
    for( auto pri_iter = priorities.rbegin(); pri_iter != priorities.rend(); ++pri_iter ) {
        for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ++iter ) {
            const overmap_special &special = *iter->special_details;
            if( *pri_iter != special.get_priority() ) {
                continue;
            }
            const overmap_special_placement_constraints &constraints = special.get_constraints();


            if( iter->instances_placed >= constraints.occurrences.max ) {
                continue;
            }


            if( !place_optional && iter->instances_placed >= constraints.occurrences.min ) {
                continue;
            }

            if( !special.can_belong_to_city( p, nearest_city, *this ) ) {
                continue;
            }

            const om_direction::type rotation = random_special_rotation( special, p, must_be_unexplored );
            if( rotation == om_direction::type::invalid ) {
                continue;
            }

            place_special( special, p, rotation, nearest_city, false, must_be_unexplored );


            if( special.has_flag( "OVERMAP_UNIQUE" ) || special.has_flag( "GLOBALLY_UNIQUE" ) ) {
                overmap_buffer.consume_deck_placement( special.id );
            }

            if( ++iter->instances_placed >= constraints.occurrences.max ) {
                enabled_specials.erase( iter );
            }

            return true;
        }
    }

    return false;
}

void overmap::place_specials_pass(
    overmap_special_batch &enabled_specials, om_special_sectors &sectors,
    const bool place_optional, const bool must_be_unexplored )
{

    std::shuffle( sectors.sectors.begin(), sectors.sectors.end(), rng_get_engine() );
    for( auto it = sectors.sectors.begin(); it != sectors.sectors.end(); ) {
        const size_t attempts = 10;
        bool placed = false;
        for( size_t i = 0; i < attempts; ++i ) {
            if( place_special_attempt( enabled_specials, *it, sectors.sector_width, place_optional,
                                       must_be_unexplored ) ) {
                placed = true;
                it = sectors.sectors.erase( it );
                if( enabled_specials.empty() ) {
                    return;
                }
                break;
            }
        }

        if( !placed ) {
            ++it;
        }
    }
}





void overmap::place_specials( overmap_special_batch &enabled_specials )
{




    bool overmap_has_lake = false;
    bool overmap_has_ocean = false;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT && !overmap_has_lake; z++ ) {
        for( int x = 0; x < OMAPX && !overmap_has_lake; x++ ) {
            for( int y = 0; y < OMAPY && !overmap_has_lake; y++ ) {
                overmap_has_lake = ter_unsafe( { x, y, z } )->is_lake();
            }
        }
    }
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT && !overmap_has_ocean; z++ ) {
        for( int x = 0; x < OMAPX && !overmap_has_ocean; x++ ) {
            for( int y = 0; y < OMAPY && !overmap_has_ocean; y++ ) {
                overmap_has_ocean = ter_unsafe( { x, y, z } )->is_ocean();
            }
        }
    }

    for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ) {



        if( iter->special_details->has_flag( "LAKE" ) && !overmap_has_lake ) {
            iter = enabled_specials.erase( iter );
            continue;
        }
        if( iter->special_details->has_flag( "OCEAN" ) && !overmap_has_ocean ) {
            iter = enabled_specials.erase( iter );
            continue;
        }

        const bool unique = iter->special_details->has_flag( "OVERMAP_UNIQUE" );
        const bool globally_unique = iter->special_details->has_flag( "GLOBALLY_UNIQUE" );
        if( unique || globally_unique ) {
            const overmap_special_id &id = iter->special_details->id;
            const overmap_special_placement_constraints &constraints =
                iter->special_details->get_constraints();
            const int occ_min = constraints.occurrences.min;
            const int occ_max = constraints.occurrences.max;


            if( occ_min <= 0 || occ_max <= 0 ) {
                iter = enabled_specials.erase( iter );
                continue;
            }


            if( globally_unique && overmap_buffer.contains_unique_special( id ) ) {
                iter = enabled_specials.erase( iter );
                continue;
            }

            unique_special_deck_state &deck =
                overmap_buffer.get_deck_state( id, occ_min, occ_max );


            if( deck.to_place <= 0 ) {

                if( deck.successes_remain > 0 && x_in_y( deck.successes_remain, deck.cards_remain ) ) {
                    deck.successes_remain--;
                    deck.cards_remain--;
                    deck.to_place++;
                } else {
                    deck.cards_remain--;
                }


                if( deck.cards_remain <= 0 ) {
                    deck.successes_remain = occ_min;
                    deck.cards_remain = occ_max;
                }
            }

            if( deck.to_place > 0 ) {

                iter->instances_placed = constraints.occurrences.max - 1;
            } else {


                iter->instances_placed = constraints.occurrences.max;
            }
        }
        ++iter;
    }



    const bool any_candidates = std::any_of( enabled_specials.begin(), enabled_specials.end(),
    []( const overmap_special_placement & p ) {
        return p.instances_placed < p.special_details->get_constraints().occurrences.max;
    } );
    if( !any_candidates ) {
        return;
    }
    om_special_sectors sectors = get_sectors( OMSPEC_FREQ );



    place_specials_pass( enabled_specials, sectors, false, false );







    overmap_special_batch custom_overmap_specials = enabled_specials;



    bool any_below_minimum =
        std::any_of( custom_overmap_specials.begin(), custom_overmap_specials.end(),
    []( const overmap_special_placement & placement ) {
        return placement.instances_placed <
               placement.special_details->get_constraints().occurrences.min;
    } );

    if( any_below_minimum ) {

        int previous_distance = 0;
        std::vector<point_abs_om> nearest_candidates;


        for( const point_abs_om &candidate_addr : closest_points_first(
                 custom_overmap_specials.get_origin(), 2 ) ) {
            if( !overmap_buffer.has( candidate_addr ) ) {
                int current_distance = square_dist( pos(), candidate_addr );
                if( nearest_candidates.empty() || current_distance == previous_distance ) {
                    nearest_candidates.push_back( candidate_addr );
                    previous_distance = current_distance;
                } else {
                    break;
                }
            }
        }
        if( !nearest_candidates.empty() ) {
            point_abs_om new_om_addr = random_entry( nearest_candidates );
            overmap_buffer.create_custom_overmap( new_om_addr, custom_overmap_specials );
        } else {
            std::string msg =
                "The following specials could not be placed, some missions may fail to initialize: ";
            int n = 0;
            for( auto iter = custom_overmap_specials.begin(); iter != custom_overmap_specials.end(); ) {
                if( iter->instances_placed < iter->special_details->get_constraints().occurrences.min ) {
                    msg.append( iter->special_details->id.c_str() ).append( ", " );
                    n++;
                }
                ++iter;
            }
            if( n > 0 ) {
                msg = msg.substr( 0, msg.length() - 2 );
            } else {
                msg = msg.append( "<unknown>" );
            }
            add_msg( _( msg ) );
        }
    }

    place_specials_pass( enabled_specials, sectors, true, false );












    std::map<overmap_special_id, int> processed_specials;
    for( overmap_special_placement &elem : custom_overmap_specials ) {
        processed_specials[elem.special_details->id] = elem.instances_placed;
    }


    for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {

        std::map<overmap_special_id, int>::iterator iter = processed_specials.find(
                    it->special_details->id );
        if( iter != processed_specials.end() ) {

            it->instances_placed += ( iter->second - it->instances_placed );



            if( it->instances_placed >= it->special_details->get_constraints().occurrences.max ) {
                it = enabled_specials.erase( it );
            } else {
                ++it;
            }
        } else {


            it = enabled_specials.erase( it );
        }
    }
}

void overmap::place_mongroups()
{

    int city_spawn_threshold = get_option<int>( "SPAWN_CITY_HORDE_THRESHOLD" );
    if( city_spawn_threshold > -1 ) {
        int city_spawn_chance = get_option<int>( "SPAWN_CITY_HORDE_SMALL_CITY_CHANCE" );
        float city_spawn_scalar = get_option<float>( "SPAWN_CITY_HORDE_SCALAR" );
        float city_spawn_spread = get_option<float>( "SPAWN_CITY_HORDE_SPREAD" );
        float spawn_density = get_option<float>( "SPAWN_DENSITY" );

        for( city &elem : cities ) {
            if( elem.size > city_spawn_threshold || one_in( city_spawn_chance ) ) {



                int desired_zombies = elem.size * city_spawn_scalar * spawn_density;

                float city_effective_radius = elem.size * city_spawn_spread;

                int city_distance_increment = std::ceil( city_effective_radius / 4 );

                tripoint_abs_omt city_center = project_combine( elem.pos_om, tripoint_om_omt( elem.pos, 0 ) );

                std::vector<tripoint_om_sm> submap_list;


                for( tripoint_om_omt const &temp_omt : points_in_radius( tripoint_om_omt( elem.pos, 0 ),
                        static_cast<int>( city_effective_radius ), 0 ) ) {


                    if( inbounds( temp_omt, 2 ) ) {
                        tripoint_abs_omt target_omt = project_combine( elem.pos_om, temp_omt );



                        if( overmap_buffer.ter( target_omt )->get_type_id() == oter_type_road ) {
                            tripoint_om_sm this_sm = project_to<coords::sm>( temp_omt );



                            std::vector<tripoint_om_sm> local_sm_list;
                            local_sm_list.push_back( this_sm );
                            local_sm_list.push_back( this_sm + point::east );
                            local_sm_list.push_back( this_sm + point::south );
                            local_sm_list.push_back( this_sm + point::south_east );




                            int new_size = 4 - ( trig_dist( target_omt, city_center ) / city_distance_increment );
                            if( new_size > 0 ) {
                                std::shuffle( local_sm_list.begin(), local_sm_list.end(), rng_get_engine() );
                                local_sm_list.resize( new_size );

                                submap_list.insert( submap_list.end(), local_sm_list.begin(), local_sm_list.end() );
                            }

                        }
                    }
                }

                if( submap_list.empty() ) {

                    add_msg_debug( debugmode::DF_OVERMAP,
                                   "tried to add zombie hordes to city %s centered at omt %s, but there were no roads!",
                                   elem.name, city_center.to_string_writable() );
                    continue;
                }

                add_msg_debug( debugmode::DF_OVERMAP, "adding %i zombies in hordes to city %s centered at omt %s.",
                               desired_zombies, elem.name, city_center.to_string_writable() );


                while( desired_zombies > 0 ) {
                    std::shuffle( submap_list.begin(), submap_list.end(), rng_get_engine() );
                    for( tripoint_om_sm const &s : submap_list ) {
                        if( desired_zombies <= 0 ) {
                            break;
                        }
                        spawn_mongroup( s, GROUP_ZOMBIE_HORDE, desired_zombies > 10 ? 10 : desired_zombies );
                        desired_zombies -= 10;
                    }
                }
            }
        }
    }

    if( get_option<bool>( "DISABLE_ANIMAL_CLASH" ) ) {

        for( int x = 3; x < OMAPX - 3; x += 7 ) {
            for( int y = 3; y < OMAPY - 3; y += 7 ) {
                int swamp_count = 0;
                for( int sx = x - 3; sx <= x + 3; sx++ ) {
                    for( int sy = y - 3; sy <= y + 3; sy++ ) {
                        if( ter( { sx, sy, 0 } ) == oter_forest_water ) {
                            swamp_count += 2;
                        }
                    }
                }
                if( swamp_count >= 25 ) {
                    tripoint_om_omt p( x, y, 0 );
                    float norm_factor = std::abs( GROUP_SWAMP->freq_total / 1000.0f );
                    unsigned int pop =
                        std::round( norm_factor * rng( swamp_count * 8, swamp_count * 25 ) );
                    spawn_mon_group(
                        mongroup(
                            GROUP_SWAMP, project_combine( pos(), project_to<coords::sm>( p ) ),
                            pop ), 3 );
                }
            }
        }
    }

    if( settings->overmap_river || settings->overmap_lake ) {

        for( int x = 3; x < OMAPX - 3; x += 7 ) {
            for( int y = 3; y < OMAPY - 3; y += 7 ) {
                int river_count = 0;
                for( int sx = x - 3; sx <= x + 3; sx++ ) {
                    for( int sy = y - 3; sy <= y + 3; sy++ ) {
                        if( is_lake_or_river( ter( { sx, sy, 0 } ) ) ) {
                            river_count++;
                        }
                    }
                }
                if( river_count >= 25 && is_lake_or_river( ter( { x, y, 0 } ) ) ) {
                    tripoint_om_omt p( x, y, 0 );
                    float norm_factor = std::abs( GROUP_RIVER->freq_total / 1000.0f );
                    unsigned int pop =
                        std::round( norm_factor * rng( river_count * 8, river_count * 25 ) );
                    spawn_mon_group(
                        mongroup( GROUP_RIVER, project_combine( pos(), project_to<coords::sm>( p ) ),
                                  pop ), 3 );
                }
            }
        }
    }
    if( settings->overmap_ocean ) {

        const region_settings_ocean &settings_ocean = settings->get_settings_ocean();
        const om_noise::om_noise_layer_ocean f( global_base_point(), g->get_seed() );
        const point_abs_om this_om = pos();
        const bool oceans_disabled = !settings_ocean.ocean_start_north.has_value() &&
                                     !settings_ocean.ocean_start_east.has_value() &&
                                     !settings_ocean.ocean_start_west.has_value() && !settings_ocean.ocean_start_south.has_value();


        constexpr float DEEP_OCEAN_THRESHOLD_ADJUST = 1.25;


        const auto is_deep_ocean = [&]( const point_om_omt & p ) {

            if( oceans_disabled ) {

                return false;
            }
            bool inbounds = p.x() > -5 && p.y() > -5 && p.x() < OMAPX + 5 && p.y() < OMAPY + 5;
            if( !inbounds ) {
                return false;
            }
            float ocean_adjust = calculate_ocean_gradient( p, this_om );
            if( ocean_adjust == 0.0f ) {

                return false;
            }
            return f.noise_at( p ) + ocean_adjust > settings_ocean.noise_threshold_ocean *
                   DEEP_OCEAN_THRESHOLD_ADJUST;
        };

        for( int x = 3; x < OMAPX - 3; x += 7 ) {
            for( int y = 3; y < OMAPY - 3; y += 7 ) {
                int ocean_count = 0;
                for( int sx = x - 3; sx <= x + 3; sx++ ) {
                    for( int sy = y - 3; sy <= y + 3; sy++ ) {
                        if( is_ocean( ter( { sx, sy, 0 } ) ) ) {
                            ocean_count++;
                        }
                    }
                }
                bool am_deep = is_deep_ocean( { x, y } );
                if( ocean_count >= 25 ) {
                    tripoint_om_omt p( x, y, 0 );
                    if( am_deep ) {
                        float norm_factor = std::abs( GROUP_OCEAN_DEEP->freq_total / 1000.0f );
                        unsigned int pop =
                            std::round( norm_factor * rng( ocean_count * 8, ocean_count * 25 ) );
                        spawn_mon_group(
                            mongroup( GROUP_OCEAN_DEEP, project_combine( pos(), project_to<coords::sm>( p ) ),
                                      pop ), 3 );
                    } else {
                        float norm_factor = std::abs( GROUP_OCEAN_SHORE->freq_total / 1000.0f );
                        unsigned int pop =
                            std::round( norm_factor * rng( ocean_count * 8, ocean_count * 25 ) );
                        spawn_mon_group(
                            mongroup( GROUP_OCEAN_SHORE, project_combine( pos(), project_to<coords::sm>( p ) ),
                                      pop ), 3 );
                    }
                }
            }
        }
    }
}

void overmap::place_nemesis( const tripoint_abs_omt &p )
{
    tripoint_abs_sm pos_sm = project_to<coords::sm>( p );

    mongroup nemesis = mongroup( GROUP_NEMESIS, pos_sm, 1 );
    nemesis.horde = true;
    nemesis.behaviour = mongroup::horde_behaviour::nemesis;
    add_mon_group( nemesis );
}

point_abs_omt overmap::global_base_point() const
{
    return project_to<coords::omt>( loc );
}

void overmap::place_radios()
{
    auto strength = []() {
        return rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH );
    };
    std::string message;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt pos_omt( i, j, 0 );
            point_om_sm pos_sm = project_to<coords::sm>( pos_omt.xy() );

            if( is_ot_match( "radio_tower", ter( pos_omt ), ot_match_type::prefix ) ) {
                if( one_in( 3 ) ) {
                    radios.emplace_back( pos_sm, strength(), "", radio_type::WEATHER_RADIO );
                } else {
                    message = SNIPPET.expand( SNIPPET.random_from_category( "radio_archive" ).value_or(
                                                  translation() ).translated() );
                    radios.emplace_back( pos_sm, strength(), message );
                }
            } else if( is_ot_match( "lmoe", ter( pos_omt ), ot_match_type::prefix ) ) {
                message = string_format( _( "This is automated emergency shelter beacon %d%d."
                                            "  Supplies, amenities and shelter are stocked." ), i, j );
                radios.emplace_back( pos_sm, strength() / 2, message );
            } else if( is_ot_match( "fema_entrance", ter( pos_omt ), ot_match_type::prefix ) ) {
                message = string_format( _( "This is FEMA camp %d%d."
                                            "  Supplies are limited, please bring supplemental food, water, and bedding."
                                            "  This is FEMA camp %d%d.  A designated long-term emergency shelter." ), i, j, i, j );
                radios.emplace_back( pos_sm, strength(), message );
            }
        }
    }
}

void overmap::open( overmap_special_batch &enabled_specials )
{
    if( world_generator->active_world->has_compression_enabled() ) {
        assure_dir_exist( PATH_INFO::current_dimension_save_path() / zzip_overmap_directory );
        const std::string terfilename = overmapbuffer::terrain_filename( loc );
        const std::filesystem::path terfilename_path = std::filesystem::u8path( terfilename );
        const cata_path zzip_path = PATH_INFO::current_dimension_save_path() / zzip_overmap_directory /
                                    terfilename_path
                                    +
                                    zzip_suffix;
        if( file_exist( zzip_path ) ) {
            std::optional<zzip> z = zzip::load( zzip_path.get_unrelative_path(),
                                                ( PATH_INFO::world_base_save_path() / "overmaps.dict" ).get_unrelative_path()
                                              );

            if( z && read_from_zzip_optional( *z, terfilename_path, [this]( std::string_view sv ) {
            std::istringstream is{ std::string( sv ) };
            unserialize( is );
            } ) ) {
                const cata_path plrfilename = overmapbuffer::player_filename( loc );
                read_from_file_optional( plrfilename, [this, &plrfilename]( std::istream & is ) {
                    unserialize_view( plrfilename, is );
                } );
                return;
            }
        }
    } else {
        const cata_path terfilename = PATH_INFO::current_dimension_save_path() /
                                      overmapbuffer::terrain_filename(
                                          loc );

        if( read_from_file_optional( terfilename, [this, &terfilename]( std::istream & is ) {
        unserialize( terfilename, is );
        } ) ) {
            const cata_path plrfilename = overmapbuffer::player_filename( loc );
            read_from_file_optional( plrfilename, [this, &plrfilename]( std::istream & is ) {
                unserialize_view( plrfilename, is );
            } );
            return;
        }
    }


    std::vector<const overmap *> neighbors;
    neighbors.reserve( four_adjacent_offsets.size() );
    for( const point &adjacent : four_adjacent_offsets ) {
        neighbors.emplace_back( overmap_buffer.get_existing( loc + adjacent ) );
    }
    generate( neighbors, enabled_specials );
}


void overmap::save() const
{
    write_to_file( overmapbuffer::player_filename( loc ), [&]( std::ostream & stream ) {
        serialize_view( stream );
    } );

    if( world_generator->active_world->has_compression_enabled() ) {
        const std::string terfilename = overmapbuffer::terrain_filename( loc );
        const std::filesystem::path terfilename_path = std::filesystem::u8path( terfilename );
        const cata_path overmaps_folder = PATH_INFO::current_dimension_save_path() / zzip_overmap_directory;
        assure_dir_exist( overmaps_folder );
        const cata_path zzip_path = overmaps_folder / terfilename_path + zzip_suffix;
        std::optional<zzip> z = zzip::load( zzip_path.get_unrelative_path(),
                                            ( PATH_INFO::world_base_save_path() / "overmaps.dict" ).get_unrelative_path()
                                          );
        if( !z ) {
            throw std::runtime_error(
                string_format(
                    "Failed to open %s",
                    zzip_path.get_unrelative_path().generic_u8string().c_str()
                )
            );
        }

        std::stringstream s;
        serialize( s );

        if( !z->add_file( terfilename_path, s.str() ) ) {
            throw std::runtime_error( string_format( "Failed to save omap %d.%d to %s", loc.x(),
                                      loc.y(), zzip_path.get_unrelative_path().generic_u8string().c_str() ) );
        }
        cata_path tmp_path = zzip_path + ".tmp";
        if( z->compact_to( tmp_path.get_unrelative_path(), 2.0 ) ) {
            z.reset();
            rename_file( tmp_path, zzip_path );
        }
    } else {
        write_to_file( PATH_INFO::current_dimension_save_path() /
                       overmapbuffer::terrain_filename(
                           loc ), [&](
        std::ostream & stream ) {
            serialize( stream );
        } );
    }
}

void overmap::spawn_mon_group( const mongroup &group, int radius )
{
    tripoint_om_omt pos = project_to<coords::omt>( group.rel_pos() );
    if( safe_at_worldgen.find( pos ) != safe_at_worldgen.end() ) {
        return;
    }
    add_mon_group( group, radius );
}

void overmap::debug_force_add_group( const mongroup &group )
{
    add_mon_group( group, 1 );
}

std::vector<std::reference_wrapper<mongroup>> overmap::debug_unsafe_get_groups_at(
            tripoint_abs_omt &loc )
{
    point_abs_om overmap;
    tripoint_om_omt omt_within_overmap;
    std::tie( overmap, omt_within_overmap ) = project_remain<coords::om>( loc );
    tripoint_om_sm om_sm_pos = project_to<coords::sm>( omt_within_overmap );

    std::vector<std::reference_wrapper <mongroup>> groups_at;
    for( std::pair<const tripoint_om_sm, mongroup> &pair : zg ) {
        if( pair.first == om_sm_pos ) {
            groups_at.emplace_back( pair.second );
        }
    }
    return groups_at;
}

void overmap::add_mon_group( const mongroup &group )
{
    zg.emplace( group.rel_pos(), group );
}

void overmap::add_mon_group( const mongroup &group, int radius )
{

    if( radius <= 1 ) {
        add_mon_group( group );
        return;
    }
    const int rad = std::max<int>( 0, radius );
    const double total_area = rad * rad * M_PI + 1;
    const double pop = std::max<int>( 0, group.population );
    for( int x = -rad; x <= rad; x++ ) {
        for( int y = -rad; y <= rad; y++ ) {
            const int dist = trig_dist( point( x, y ), point::zero );
            if( dist > rad ) {
                continue;
            }

            double pop_here;
            if( rad == 0 ) {
                pop_here = pop;
            } else {


                pop_here = ( static_cast<double>( rad - dist ) / rad ) * pop / total_area;
            }
            if( pop_here > pop || pop_here < 0 ) {
                DebugLog( D_ERROR, D_GAME ) << group.type.str() << ": invalid population here: " << pop_here;
            }
            int p = std::max( 0, static_cast<int>( std::floor( pop_here ) ) );
            if( pop_here - p != 0 ) {



                const int mod = static_cast<int>( 10000.0 * ( pop_here - p ) );
                if( x_in_y( mod, 10000 ) ) {
                    p++;
                }
            }
            if( p == 0 ) {
                continue;
            }


            mongroup tmp( group );
            tmp.abs_pos += point( x, y );
            tmp.population = p;







            add_mon_group( tmp );
        }
    }
}

shared_ptr_fast<npc> overmap::find_npc( const character_id &id ) const
{
    for( const auto &guy : npcs ) {
        if( guy->getID() == id ) {
            return guy;
        }
    }
    return nullptr;
}

shared_ptr_fast<npc> overmap::find_npc_by_unique_id( const std::string &id ) const
{
    for( const auto &guy : npcs ) {
        if( guy->get_unique_id() == id ) {
            return guy;
        }
    }
    return nullptr;
}

void overmap::add_camp( const point_abs_omt &p, const basecamp &camp )
{
    auto it = camps.emplace( p, camp );
    if( !it.second ) {
        debugmsg( "Tried to add a basecamp %s at %s when basecamp %s is already present", camp.camp_name(),
                  p.to_string(), it.first->second.camp_name() );
    }
}

std::optional<basecamp *> overmap::find_camp( const point_abs_omt &p )
{
    const auto it = camps.find( p );
    if( it != camps.end() ) {
        return &( it->second );
    }
    return std::nullopt;
}

void overmap::remove_camp( const point_abs_omt &p )
{
    const auto it = camps.find( p );
    if( it != camps.end() ) {
        camps.erase( it );
    }
}

void overmap::clear_camps()
{
    auto iter = camps.begin();
    while( iter != camps.end() ) {
        iter->second.remove_camp( false );
        iter = camps.erase( iter );
    }
}

bool overmap::is_omt_generated( const tripoint_om_omt &loc ) const
{
    if( !inbounds( loc ) ) {
        return false;
    }



    tripoint_abs_sm global_sm_loc =
        project_to<coords::sm>( project_combine( pos(), loc ) );

    return MAPBUFFER.submap_exists( global_sm_loc );
}
