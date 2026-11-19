#include "character_oracle.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include "basecamp.h"
#include "behavior.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "npc.h"
#include "npc_class.h"
#include "overmapbuffer.h"
#include "point.h"
#include "ret_val.h"
#include "stomach.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

static const efftype_id effect_meth( "meth" );
static const efftype_id effect_npc_run_away( "npc_run_away" );
static const json_character_flag json_flag_CANNOT_MOVE( "CANNOT_MOVE" );
static const trait_id trait_IGNORE_SOUND( "IGNORE_SOUND" );



static const vehicle *player_moving_vehicle()
{
    const Character &p = get_player_character();
    if( !p.in_vehicle ) {
        return nullptr;
    }
    const optional_vpart_position vp = get_map().veh_at( p.pos_bub() );
    if( !vp ) {
        return nullptr;
    }
    const vehicle &veh = vp->vehicle();
    return veh.velocity != 0 ? &veh : nullptr;
}




static bool npc_within_camp( const npc &n )
{
    if( !n.assigned_camp ) {
        return false;
    }
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( n.assigned_camp->xy() );
    if( !bcp || !*bcp ) {

        return n.pos_abs_omt() == *n.assigned_camp;
    }
    return ( *bcp )->point_within_camp( n.pos_abs_omt() );
}

namespace behavior
{




status_t character_oracle_t::needs_warmth_badly( std::string_view ) const
{

    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        if( subject->get_part_temp_conv( bp ) <= BODYTEMP_VERY_COLD ) {
            return status_t::running;
        }
    }
    return status_t::success;
}

status_t character_oracle_t::needs_water_badly( std::string_view ) const
{

    if( !subject->needs_food() ) {
        return status_t::success;
    }

    if( subject->get_thirst() > 520 ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::needs_food_badly( std::string_view ) const
{

    if( !subject->needs_food() ) {
        return status_t::success;
    }

    if( subject->get_hunger() >= 300 && subject->get_starvation() > base_metabolic_rate ) {
        return status_t::running;
    }

    if( subject->get_stored_kcal() + subject->stomach.get_calories() <
        subject->get_healthy_kcal() * 3 / 4 ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::can_wear_warmer_clothes( std::string_view ) const
{


    bool found_clothes = subject->has_item_with( [this]( const item & candidate ) {
        return candidate.get_warmth() > 0 && subject->can_wear( candidate ).success() &&
               !subject->is_worn( candidate );
    } );
    return found_clothes ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_make_fire( std::string_view ) const
{



    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return const_cast<npc *>( n )->find_fire_spot().has_value()
           ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_take_shelter( std::string_view ) const
{


    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return n->find_nearby_shelters().empty() ? status_t::failure : status_t::running;
}

status_t character_oracle_t::has_water( std::string_view ) const
{

    bool found_water = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->quench > 0;
    } );
    return found_water ? status_t::running : status_t::failure;
}

status_t character_oracle_t::has_food( std::string_view ) const
{

    bool found_food = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->has_calories();
    } );
    return found_food ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_obtain_food( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }




    return const_cast<npc *>( n )->find_food_candidates().empty()
           ? status_t::failure : status_t::running;
}

status_t character_oracle_t::can_obtain_water( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }

    return const_cast<npc *>( n )->find_water_candidates().empty()
           ? status_t::failure : status_t::running;
}

status_t character_oracle_t::can_obtain_warmth( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }



    if( subject->has_item_with( [this]( const item & it ) {
    return it.get_warmth() > 0 && subject->can_wear( it ).success() &&
               !subject->is_worn( it );
    } ) ) {
        return status_t::running;
    }


    if( !const_cast<npc *>( n )->find_warmth_candidates().empty() ) {
        return status_t::running;
    }




    if( get_map().has_flag( ter_furn_flag::TFLAG_INDOORS, n->pos_bub() ) &&
        n->get_warmth_indoor_hold_turns() < 60 ) {
        return status_t::running;
    }
    return status_t::failure;
}

status_t character_oracle_t::needs_sleep_badly( std::string_view ) const
{


    if( subject->get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) ) {
        return status_t::running;
    }
    return status_t::success;
}

float character_oracle_t::thirst_urgency( std::string_view ) const
{
    if( !subject->needs_food() ) {
        return 0.0f;
    }

    static constexpr float death_threshold = 1200.0f;
    return std::clamp( subject->get_thirst() / death_threshold, 0.0f, 1.0f );
}

float character_oracle_t::hunger_urgency( std::string_view ) const
{
    if( !subject->needs_food() ) {
        return 0.0f;
    }

    const int healthy = subject->get_healthy_kcal();
    if( healthy <= 0 ) {
        return 0.0f;
    }
    const float kcal_frac = static_cast<float>( subject->get_stored_kcal() ) / healthy;
    return std::clamp( 1.0f - kcal_frac, 0.0f, 1.0f );
}

float character_oracle_t::warmth_urgency( std::string_view ) const
{


    const float norm = units::to_kelvin( BODYTEMP_NORM );
    const float freeze = units::to_kelvin( BODYTEMP_FREEZING );
    const float range = norm - freeze;
    if( range <= 0.0f ) {
        return 0.0f;
    }
    float coldest = norm;
    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        const float temp = units::to_kelvin( subject->get_part_temp_conv( bp ) );
        coldest = std::min( coldest, temp );
    }
    return std::clamp( ( norm - coldest ) / range, 0.0f, 1.0f );
}

status_t character_oracle_t::can_sleep( std::string_view ) const
{




    if( subject->has_effect( effect_meth ) ) {
        return status_t::failure;
    }
    if( subject->get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) ) {
        return status_t::running;
    }
    return status_t::failure;
}

float character_oracle_t::sleepiness_urgency( std::string_view ) const
{

    static constexpr float cap = 1000.0f;
    return std::clamp( subject->get_sleepiness() / cap, 0.0f, 1.0f );
}




status_t character_oracle_t::in_danger( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return n->get_ai_danger() > 0 ? status_t::running : status_t::failure;
}

status_t character_oracle_t::should_flee( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    if( n->has_effect( effect_npc_run_away ) ) {
        return status_t::running;
    }
    if( n->get_attitude() == NPCATT_FLEE_TEMP ) {
        return status_t::running;
    }
    return status_t::failure;
}

status_t character_oracle_t::has_target( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return n->get_ai_target().lock() ? status_t::running : status_t::failure;
}

status_t character_oracle_t::has_sound_alerts( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }



    if( n->is_walking_with() || n->has_flag( json_flag_CANNOT_MOVE ) ||
        n->has_trait( trait_IGNORE_SOUND ) ) {
        return status_t::failure;
    }
    return n->has_ai_sound_alerts() ? status_t::running : status_t::failure;
}

status_t character_oracle_t::displaced_from_post( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    if( n->has_flag( json_flag_CANNOT_MOVE ) ) {
        return status_t::failure;
    }
    if( n->is_walking_with() ) {
        return status_t::failure;
    }
    std::optional<tripoint_abs_ms> gp = n->get_guard_post();
    if( !gp ) {
        return status_t::failure;
    }
    return n->pos_abs() != *gp ? status_t::running : status_t::failure;
}

status_t character_oracle_t::on_shift( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->get_guard_post() || !n->myclass.is_valid() ) {
        return status_t::failure;
    }

    if( n->is_walking_with() ) {
        return status_t::failure;
    }
    const auto &[start, end] = n->myclass.obj().get_work_hours();
    const int hour = to_hours<int>( time_past_midnight( calendar::turn ) );
    return is_within_work_hours( hour, start, end ) ? status_t::running : status_t::failure;
}

float character_oracle_t::duty_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return 0.0f;
    }
    std::optional<tripoint_abs_ms> gp = n->get_guard_post();
    if( !gp || !n->myclass.is_valid() ) {
        return 0.0f;
    }
    if( n->is_walking_with() ) {
        return 0.0f;
    }
    const auto &[start, end] = n->myclass.obj().get_work_hours();
    const int hour = to_hours<int>( time_past_midnight( calendar::turn ) );
    const bool on = is_within_work_hours( hour, start, end );

    if( n->pos_abs() == *gp ) {


        return on ? 0.45f : 0.0f;
    }


    if( !on ) {
        return 0.0f;
    }


    const int dist = rl_dist( n->pos_abs(), *gp );
    return std::max( 0.45f, std::min( 0.5f, dist * 0.05f ) );
}

status_t character_oracle_t::npc_is_following( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->can_follow_player_now() ) {
        return status_t::failure;
    }

    if( player_moving_vehicle() ) {
        return status_t::failure;
    }
    const Character &player = get_player_character();
    const int dist = rl_dist( n->pos_abs(), player.pos_abs() );
    if( dist <= n->desired_follow_radius() && n->posz() == player.posz() ) {
        return status_t::success;
    }
    return status_t::running;
}

float character_oracle_t::npc_following_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->can_follow_player_now() ) {
        return 0.0f;
    }
    if( player_moving_vehicle() ) {
        return 0.0f;
    }
    const Character &player = get_player_character();
    if( n->posz() != player.posz() ) {
        return 0.6f;
    }
    const int dist = rl_dist( n->pos_abs(), player.pos_abs() );
    const int radius = n->desired_follow_radius();
    if( dist <= radius ) {
        return 0.0f;
    }
    return std::clamp( 0.3f + ( dist - radius ) * 0.015f, 0.3f, 0.6f );
}

status_t character_oracle_t::npc_should_embark( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->is_walking_with() || n->has_flag( json_flag_CANNOT_MOVE ) ) {
        return status_t::failure;
    }
    const Character &player = get_player_character();
    if( !player.in_vehicle ) {
        return status_t::failure;
    }

    if( player_moving_vehicle() ) {
        return status_t::failure;
    }


    return status_t::running;
}

float character_oracle_t::npc_embark_urgency( std::string_view ) const
{
    return npc_should_embark( "" ) == status_t::running ? 0.6f : 0.0f;
}

status_t character_oracle_t::npc_has_goto_order( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->goto_to_this_pos || n->has_flag( json_flag_CANNOT_MOVE ) ) {
        return status_t::failure;
    }
    if( n->pos_abs() == *n->goto_to_this_pos ) {
        return status_t::success;
    }
    return status_t::running;
}

float character_oracle_t::npc_goto_order_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->goto_to_this_pos ) {
        return 0.0f;
    }
    if( n->pos_abs() == *n->goto_to_this_pos ) {
        return 0.0f;
    }


    return 0.65f;
}

status_t character_oracle_t::has_camp_job( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT ) {
        return status_t::failure;
    }
    if( !npc_within_camp( *n ) ) {
        return status_t::failure;
    }
    if( !n->has_job() ) {
        return status_t::failure;
    }
    if( calendar::turn - n->last_job_scan < 10_minutes ) {
        return status_t::failure;
    }
    return status_t::running;
}

status_t character_oracle_t::is_away_from_camp( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT ) {
        return status_t::failure;
    }
    return npc_within_camp( *n ) ? status_t::failure : status_t::running;
}

status_t character_oracle_t::is_camp_idle( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT ) {
        return status_t::failure;
    }
    if( n->get_attitude() == NPCATT_ACTIVITY ) {
        return status_t::failure;
    }
    if( !npc_within_camp( *n ) ) {
        return status_t::failure;
    }
    return status_t::running;
}

float character_oracle_t::camp_work_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT
        || !npc_within_camp( *n ) || !n->has_job() ) {
        return 0.0f;
    }
    return 0.4f;
}

float character_oracle_t::return_to_camp_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT
        || npc_within_camp( *n ) ) {
        return 0.0f;
    }

    return 0.5f;
}

float character_oracle_t::free_time_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT
        || n->get_attitude() == NPCATT_ACTIVITY
        || !npc_within_camp( *n ) ) {
        return 0.0f;
    }
    return 0.35f;
}

}
