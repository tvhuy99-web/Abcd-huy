#include "player_activity.h"

#include <algorithm>
#include <memory>

#include "a11y_android.h"
#include "a11y_runtime.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "activity_type.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "event.h"
#include "event_bus.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "messages.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"
#include "translation.h"
#include "translations.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "weather.h"

static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_CHOP_LOGS( "ACT_CHOP_LOGS" );
static const activity_id ACT_CHOP_PLANKS( "ACT_CHOP_PLANKS" );
static const activity_id ACT_CHOP_TREE( "ACT_CHOP_TREE" );
static const activity_id ACT_CLEAR_RUBBLE( "ACT_CLEAR_RUBBLE" );
static const activity_id ACT_HACKSAW( "ACT_HACKSAW" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_INVOKE_ITEM( "ACT_INVOKE_ITEM" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_MIGRATION_CANCEL( "ACT_MIGRATION_CANCEL" );
static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_PICKUP( "ACT_PICKUP" );
static const activity_id ACT_READ( "ACT_READ" );
static const activity_id ACT_TREAT_WOUND( "ACT_TREAT_WOUND" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_WORKOUT_ACTIVE( "ACT_WORKOUT_ACTIVE" );
static const activity_id ACT_WORKOUT_HARD( "ACT_WORKOUT_HARD" );
static const activity_id ACT_WORKOUT_LIGHT( "ACT_WORKOUT_LIGHT" );
static const activity_id ACT_WORKOUT_MODERATE( "ACT_WORKOUT_MODERATE" );

static const efftype_id effect_nausea( "nausea" );

player_activity::player_activity() : type( activity_id::NULL_ID() ) { }

player_activity::player_activity( activity_id t, int turns, int Index, int pos,
                                  const std::string &name_in ) :
    type( t ), moves_total( turns ), moves_left( turns ),
    index( Index ),
    position( pos ), name( name_in ),
    placement( tripoint_abs_ms::min )
{
    if( type != ACT_NULL ) {
        for( const distraction_type dt : type->default_ignored_distractions() ) {
            ignored_distractions.emplace( dt );
        }
    }
}

player_activity::player_activity( const activity_actor &actor ) : type( actor.get_type() ),
    actor( actor.clone() )
{
    if( type != ACT_NULL ) {
        for( const distraction_type dt : type->default_ignored_distractions() ) {
            ignored_distractions.emplace( dt );
        }
    }
}

void player_activity::set_to_null()
{
    type = activity_id::NULL_ID();
    sfx::end_activity_sounds();
}

void player_activity::synchronize_type_with_actor()
{
    if( actor && type != activity_id::NULL_ID() ) {
        type = actor->get_type();
    }
}

bool player_activity::rooted() const
{
    return type->rooted();
}

std::string player_activity::get_stop_phrase() const
{
    return type->stop_phrase();
}

const translation &player_activity::get_verb() const
{
    return type->verb();
}

int player_activity::get_value( size_t index, int def ) const
{
    return index < values.size() ? values[index] : def;
}

bool player_activity::can_resume() const
{
    return type->can_resume();
}

bool player_activity::is_multi_type() const
{
    return type->multi_activity();
}

std::string player_activity::get_str_value( size_t index, std::string_view def ) const
{
    return std::string( index < str_values.size() ? str_values[index] : def );
}

std::optional<std::string> player_activity::get_progress_message( const avatar &u ) const
{
    if( type == ACT_NULL || get_verb().empty() ) {
        return std::optional<std::string>();
    }

    if( type == ACT_AIM ||
        type == ACT_INVOKE_ITEM
      ) {
        return std::nullopt;
    }

    std::string extra_info;
    if( type == ACT_READ ) {
        if( const item *book = targets.front().get_item() ) {
            if( const auto &reading = book->type->book ) {
                const skill_id &skill = reading->skill;
                if( skill && u.get_knowledge_level( skill ) < reading->level &&
                    u.get_skill_level_object( skill ).can_train() && u.has_identified( book->typeId() ) ) {
                    const SkillLevel &skill_level = u.get_skill_level_object( skill );
                    //~ skill_name current_skill_level (% of next level) -> next_skill_level
                    extra_info = string_format( pgettext( "reading progress", "%1s %2d (%4d%%) -> %3d" ),
                                                skill.obj().name(),
                                                skill_level.knowledgeLevel(),
                                                skill_level.knowledgeLevel() + 1,
                                                skill_level.knowledgeExperience() );
                }
            }
        }
    } else if( moves_total > 0 ) {
        if( type == ACT_HACKSAW ||
            type == ACT_JACKHAMMER ||
            type == ACT_PICKAXE ||
            type == ACT_VEHICLE ||
            type == ACT_CHOP_TREE ||
            type == ACT_CHOP_LOGS ||
            type == ACT_CHOP_PLANKS ||
            type == ACT_HEATING
          ) {
            const int percentage = ( ( moves_total - moves_left ) * 100 ) / moves_total;

            extra_info = string_format( "%d%%", percentage );
        }
    }

    if( actor ) {
        extra_info = actor->get_progress_message( *this );
    }

    return extra_info.empty() ? string_format( _( "%s…" ),
            get_verb().translated() ) : string_format( _( "%s: %s" ),
                    get_verb().translated(), extra_info );
}

void player_activity::start_or_resume( Character &who, bool resuming )
{
    if( actor && !resuming ) {
        actor->start( *this, who );
    }
    if( !type.is_null() && rooted() ) {
        who.rooted_message();
    }

    synchronize_type_with_actor();
    get_event_bus().send<event_type::character_starts_activity>( who.getID(), type, resuming );
}

void player_activity::do_turn( Character &you )
{


    if( type == ACT_MIGRATION_CANCEL ) {
        actor->do_turn( *this, you );
        activity_handlers::clean_may_activity_occupancy_items_var_if_is_avatar_and_no_activity_now( you );
        return;
    }

    synchronize_type_with_actor();

    if( *this && type->will_refuel_fires() && have_fire ) {
        have_fire = try_fuel_fire( you );
    }
    if( calendar::once_every( 30_minutes ) ) {
        no_food_nearby_for_auto_consume = false;
        no_drink_nearby_for_auto_consume = false;
    }


    if( calendar::once_every( 2_minutes ) && *this && !you.is_npc() ) {
        if( type->valid_auto_needs() && !you.has_effect( effect_nausea ) ) {
            if( you.stomach.contains() <= you.stomach.capacity( you ) / 4 && you.get_kcal_percent() < 0.95f &&
                !no_food_nearby_for_auto_consume ) {
                int consume_moves = get_auto_consume_moves( you, true );
                moves_left += consume_moves;
                moves_total += consume_moves;
                if( consume_moves == 0 ) {
                    no_food_nearby_for_auto_consume = true;
                }
            }
            if( you.get_thirst() > 130 && !no_drink_nearby_for_auto_consume ) {
                int consume_moves = get_auto_consume_moves( you, false );
                moves_left += consume_moves;
                moves_total += consume_moves;
                if( consume_moves == 0 ) {
                    no_drink_nearby_for_auto_consume = true;
                }
            }
        }
    }
    const float activity_mult = you.exertion_adjusted_move_multiplier( exertion_level() );
    if( type->based_on() == based_on_type::TIME ) {
        if( moves_left >= 100 ) {
            moves_left -= 100 * activity_mult;
            you.set_moves( 0 );
        } else {
            you.mod_moves( -you.get_moves() * moves_left / 100 );
            moves_left = 0;
        }
    } else if( type->based_on() == based_on_type::SPEED ) {
        if( you.get_moves() <= moves_left ) {
            moves_left -= you.get_moves() * activity_mult;
            you.set_moves( 0 );
        } else {
            you.mod_moves( -moves_left );
            moves_left = 0;
        }
    }
    int previous_stamina = you.get_stamina();
    if( you.is_npc() && you.check_outbounds_activity( *this ) ) {



        set_to_null();
        you.drop_invalid_inventory();
        return;
    }
    const bool travel_activity = id() == ACT_TRAVELLING;
    you.set_activity_level( exertion_level() );

    if( !type->do_turn_EOC.is_null() ) {

        dialogue d( get_talker_for( you ), nullptr );
        type->do_turn_EOC->activate_activation_only( d, "player activities" );

        if( type.is_null() ) {
            activity_handlers::clean_may_activity_occupancy_items_var_if_is_avatar_and_no_activity_now( you );
            return;
        }
    }


    if( actor ) {
        const activity_id prior_act_id = id();
        actor->do_turn( *this, you );


        if( *this && prior_act_id != id() ) {
            return;
        }
    } else {

        type->call_do_turn( this, &you );
    }








    const int adjusted_stamina = travel_activity ? you.get_stamina() - 1 : you.get_stamina();
    activity_id act_id = actor ? actor->get_type() : type;
    bool excluded = act_id == ACT_WORKOUT_HARD ||
                    act_id == ACT_WORKOUT_ACTIVE ||
                    act_id == ACT_WORKOUT_MODERATE ||
                    act_id == ACT_WORKOUT_LIGHT;
    if( !excluded && adjusted_stamina < previous_stamina &&
        you.get_stamina() < you.get_stamina_max() / 3 ) {
        if( one_in( 50 ) ) {
            you.add_msg_if_player( _( "You pause for a moment to catch your breath." ) );
        }

        auto_resume = true;
        if( you.is_avatar() && !ignoreQuery ) {
            uilist tired_query;
            tired_query.text = _( "You struggle to continue.  Keep trying?" );
            tired_query.addentry( 1, true, 'c', _( "Continue after a break." ) );
            tired_query.addentry( 2, true, 'm', _( "Maybe later." ) );
            tired_query.addentry( 3, true, 'f', _( "Finish it." ) );
            tired_query.query();
            switch( tired_query.ret ) {
                case UILIST_CANCEL:
                case 2:
                    auto_resume = false;
                    you.cancel_activity();
                    break;
                case 3:
                    ignoreQuery = true;
                    break;
                default:
                    canceled( you );
                    break;
            }
        }
        if( !ignoreQuery && auto_resume ) {
            you.assign_activity( wait_stamina_activity_actor( you.get_stamina_max() ) );
        }
        activity_handlers::clean_may_activity_occupancy_items_var_if_is_avatar_and_no_activity_now( you );
        return;
    }
    if( *this && type->rooted() ) {
        you.rooted();
        you.pause();
    }

    if( *this && moves_left <= 0 ) {




        if( you.is_avatar() ) {
            if( type == ACT_PICKUP ) {
                a11y::android::cue( "pickup" );
            } else if( moves_total >= 1000 ) {


                a11y::android::cue( "activity_done" );
            }
        }


        if( !type->completion_EOC.is_null() ) {

            dialogue d( get_talker_for( you ), nullptr );
            type->completion_EOC->activate_activation_only( d, "player activities" );
        }
        add_msg_debug( debugmode::DF_ACTIVITY, "Setting activity %s to null for %s, no moves left.",
                       type.c_str(), you.name );
        get_event_bus().send<event_type::character_finished_activity>( you.getID(), type, false );
        g->wait_popup_reset();
        if( actor ) {
            actor->finish( *this, you );
        } else {
            if( !type->call_finish( this, &you ) ) {

                set_to_null();
            }
        }
    }
    if( !*this ) {

        you.activity = player_activity();




        if( !you.has_destination() ) {
            you.resume_backlog_activity();
        }


        you.drop_invalid_inventory();
    }




    activity_handlers::clean_may_activity_occupancy_items_var_if_is_avatar_and_no_activity_now( you );
}

void player_activity::canceled( Character &who )
{
    if( *this && actor ) {
        actor->canceled( *this, who );
    }




    if( *this && type == ACT_TREAT_WOUND && who.is_avatar() && a11y::enabled() ) {
        who.add_msg_if_player( m_warning, _( "You stop treating the wound." ) );
    }
    activity_handlers::clean_may_activity_occupancy_items_var( who );
    get_event_bus().send<event_type::character_finished_activity>( who.getID(), type, true );
    g->wait_popup_reset();
}

float player_activity::exertion_level() const
{
    if( actor ) {
        return actor->exertion_level();
    }
    return type->exertion_level();
}

template <typename T>
static bool containers_equal( const T &left, const T &right )
{
    if( left.size() != right.size() ) {
        return false;
    }

    return std::equal( left.begin(), left.end(), right.begin() );
}

bool player_activity::can_resume_with( const player_activity &other, const Character &who ) const
{



    if( !*this || !other || !type->can_resume() ) {
        return false;
    }

    if( id() != other.id() ) {
        return false;
    }



    if( actor && other.actor ) {
        return actor->can_resume_with( *other.actor, who );
    }

    if( id() == ACT_CLEAR_RUBBLE ) {
        if( other.coords.empty() || other.coords[0] != coords[0] ) {
            return false;
        }
    } else if( id() == ACT_VEHICLE ) {
        if( values != other.values || str_values != other.str_values ) {
            return false;
        }
    }

    return !auto_resume && index == other.index &&
           position == other.position && name == other.name && targets == other.targets;
}

void player_activity::set_resume_values( const player_activity &other, const Character &who )
{
    if( !can_resume_with( other, who ) ) {
        return;
    }

    if( actor && other.actor ) {
        actor->set_resume_values( *other.actor, who );
    }
}

bool player_activity::is_interruptible() const
{
    return ( type.is_null() || type->interruptable() ) && interruptable;
}

bool player_activity::is_interruptible_with_kb() const
{
    return ( type.is_null() || type->interruptable_with_kb() ) && interruptable_with_kb;
}

bool player_activity::is_distraction_ignored( distraction_type distraction ) const
{
    return !is_interruptible() || ignore_followup_distractions ||
           ignored_distractions.count( distraction );
}

void player_activity::ignore_distraction( distraction_type type )
{
    ignored_distractions.emplace( type );
}

void player_activity::ignore_followup_interruptions()
{
    ignore_followup_distractions = true;
}

void player_activity::allow_distractions()
{
    ignored_distractions.clear();
    ignore_followup_distractions = false;
}

void player_activity::inherit_distractions( const player_activity &other )
{
    for( const distraction_type &type : other.ignored_distractions ) {
        ignore_distraction( type );
    }
    ignore_followup_distractions = ignore_followup_distractions || other.ignore_followup_distractions;
    dismissed_prompts.insert( other.dismissed_prompts.begin(), other.dismissed_prompts.end() );
}

bool player_activity::is_prompt_dismissed( const std::string &prompt ) const
{
    return dismissed_prompts.count( prompt ) != 0;
}

void player_activity::dismiss_prompt( const std::string &prompt )
{
    dismissed_prompts.insert( prompt );
}

std::map<distraction_type, std::string> player_activity::get_distractions() const
{
    std::map<distraction_type, std::string> res;
    if( moves_left <= 0 ) {
        return res;
    }
    if( uistate.distraction_hostile_close &&
        !is_distraction_ignored( distraction_type::hostile_spotted_near ) ) {
        if( Creature *hostile_critter = g->is_hostile_very_close( true ) ) {
            res.emplace( distraction_type::hostile_spotted_near,
                         string_format( _( "The %s is dangerously close!" ), hostile_critter->get_name() ) );
        }
    }
    if( uistate.distraction_dangerous_field &&
        !is_distraction_ignored( distraction_type::dangerous_field ) ) {
        if( field_entry *field = g->is_in_dangerous_field() ) {
            res.emplace( distraction_type::dangerous_field,
                         string_format( _( "You stand in %s!" ), field->name() ) );
        }
    }
    avatar &u = get_avatar();
    if( uistate.distraction_hunger && !is_distraction_ignored( distraction_type::hunger ) ) {

        if( calendar::once_every( 2_hours ) &&
            u.get_hunger() >= 300 &&
            u.get_starvation() > 5300 ) {
            res.emplace( distraction_type::hunger, _( "You are at risk of starving!" ) );
        }
    }
    if( uistate.distraction_thirst && !is_distraction_ignored( distraction_type::thirst ) ) {
        if( u.get_thirst() > 520 ) {
            res.emplace( distraction_type::thirst, _( "You are dangerously dehydrated!" ) );
        }
    }
    if( uistate.distraction_temperature && !is_distraction_ignored( distraction_type::temperature ) ) {
        for( const bodypart_id &bp : u.get_all_body_parts() ) {
            const units::temperature bp_temp = u.get_part_temp_cur( bp );
            if( bp_temp > BODYTEMP_VERY_HOT ) {
                res.emplace( distraction_type::temperature, _( "You are overheating!" ) );
                break;
            } else if( bp_temp < BODYTEMP_VERY_COLD ) {
                res.emplace( distraction_type::temperature, _( "You are freezing!" ) );
                break;
            }
        }
    }
    return res;
}
