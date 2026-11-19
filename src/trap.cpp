#include "trap.h"

#include <cmath>
#include <typeinfo>
#include <vector>

#include "bodypart.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "effect_on_condition.h"
#include "event.h"
#include "event_bus.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"

static const flag_id json_flag_ECHOLOCATION_DETECTABLE( "ECHOLOCATION_DETECTABLE" );
static const flag_id json_flag_SONAR_DETECTABLE( "SONAR_DETECTABLE" );

static const proficiency_id proficiency_prof_spotting( "prof_spotting" );
static const proficiency_id proficiency_prof_traps( "prof_traps" );
static const proficiency_id proficiency_prof_trapsetting( "prof_trapsetting" );

static const skill_id skill_traps( "traps" );

static const update_mapgen_id update_mapgen_none( "none" );

namespace
{

generic_factory<trap> trap_factory( "trap" );

}


template<>
inline bool int_id<trap>::is_valid() const
{
    return trap_factory.is_valid( *this );
}


template<>
const trap &int_id<trap>::obj() const
{
    return trap_factory.obj( *this );
}


template<>
const string_id<trap> &int_id<trap>::id() const
{
    return trap_factory.convert( *this );
}


template<>
int_id<trap> string_id<trap>::id() const
{
    return trap_factory.convert( *this, tr_null );
}


template<>
int_id<trap>::int_id( const string_id<trap> &id ) : _id( id.id() )
{
}


template<>
const trap &string_id<trap>::obj() const
{
    return trap_factory.obj( *this );
}


template<>
bool string_id<trap>::is_valid() const
{
    return trap_factory.is_valid( *this );
}

static std::vector<const trap *> funnel_traps;

const std::vector<const trap *> &trap::get_funnels()
{
    return funnel_traps;
}

static std::vector<const trap *> sound_triggered_traps;

const std::vector<const trap *> &trap::get_sound_triggered_traps()
{
    return sound_triggered_traps;
}

size_t trap::count()
{
    return trap_factory.size();
}

void trap::load_trap( const JsonObject &jo, const std::string &src )
{
    trap_factory.load( jo, src );
}

void trap::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "color", color, nc_color_reader{} );
    mandatory( jo, was_loaded, "symbol", sym, one_char_symbol_reader );
    mandatory( jo, was_loaded, "visibility", visibility );
    mandatory( jo, was_loaded, "avoidance", avoidance );
    mandatory( jo, was_loaded, "difficulty", difficulty );

    optional( jo, was_loaded, "memorial_male", memorial_male );
    optional( jo, was_loaded, "memorial_female", memorial_female );

    optional( jo, was_loaded, "trigger_message_u", trigger_message_u );
    optional( jo, was_loaded, "trigger_message_npc", trigger_message_npc );


    if( !!memorial_male != !!memorial_female ) {
        jo.throw_error_at(
            id.str(),
            "Only one gender of memorial message specified for trap " + id.str() + ", but none "
            "or both required." );
    }

    optional( jo, was_loaded, "flags", _flags );
    optional( jo, was_loaded, "trap_radius", trap_radius, 0 );

    if( jo.has_string( "action" ) ) {
        act = trap_function_from_string( jo.get_string( "action" ) );
    } else if( !was_loaded ) {
        jo.throw_error( "Missing mandatory member \"action\"" );
    }
    optional( jo, was_loaded, "map_regen", map_regen, update_mapgen_none );
    optional( jo, was_loaded, "benign", benign, false );
    optional( jo, was_loaded, "always_invisible", always_invisible, false );
    optional( jo, was_loaded, "funnel_radius", funnel_radius_mm, 0 );
    optional( jo, was_loaded, "comfort", comfort, 0 );
    int legacy_floor_bedding_warmth = units::to_legacy_bodypart_temp_delta( floor_bedding_warmth );
    optional( jo, was_loaded, "floor_bedding_warmth", legacy_floor_bedding_warmth, 0 );
    floor_bedding_warmth = units::from_legacy_bodypart_temp_delta( legacy_floor_bedding_warmth );
    if( jo.has_member( "spell_data" ) ) {

        if( act.target_type() != trap_function_from_string( "spell" ).target_type() ) {
            jo.throw_error_at( "spell_data",
                               R"(Can't use "spell_data" without specifying "action": "spell")" );
        }
        optional( jo, was_loaded, "spell_data", spell_data );
    }

    if( jo.has_member( "eocs" ) ) {
        if( act.target_type() != trap_function_from_string( "eocs" ).target_type() ) {
            jo.throw_error_at( "eocs", R"(Can't use "eocs" without specifying "action": "eocs")" );
        }
        for( JsonValue jv : jo.get_array( "eocs" ) ) {
            eocs.push_back( effect_on_conditions::load_inline_eoc( jv, "" ) );
        }
    }
    optional( jo, was_loaded, "trigger_weight", trigger_weight );
    optional( jo, was_loaded, "sound_threshold", sound_threshold );
    optional( jo, was_loaded, "drops", components );
    optional( jo, was_loaded, "vehicle_data", vehicle_data );
}

void vehicle_handle_trap_data::deserialize( const JsonObject &jo )
{
    optional( jo, false, "do_explosion", do_explosion, false );
    optional( jo, false, "is_falling", is_falling, false );
    optional( jo, false, "chance", chance, 100 );
    optional( jo, false, "damage", damage, 0 );
    optional( jo, false, "shrapnel", shrapnel, 0 );
    optional( jo, false, "sound_volume", sound_volume, 0 );
    optional( jo, false, "sound", sound );
    optional( jo, false, "sound_type", sound_type, "" );
    optional( jo, false, "sound_variant", sound_variant, "" );
    optional( jo, false, "spawn_items", spawn_items, named_pair_reader<itype_id, double> { "id", "chance", 1.0 } );
    optional( jo, false, "set_trap", set_trap, trap_str_id::NULL_ID() );
    if( set_trap != trap_str_id::NULL_ID() ) {
        remove_trap = false;
    } else {
        optional( jo, false, "remove_trap", remove_trap, false );
    }
}

void trap::comp::deserialize( const JsonValue &jv )
{
    if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        mandatory( jo, false, "item", item_type );
        optional( jo, false, "quantity", quantity, 1 );
        optional( jo, false, "charges", charges, 1 );
    } else {
        jv.read( item_type, true );
        quantity = 1;
        charges = 1;
    }
}

std::string trap::name() const
{
    return name_.translated();
}

update_mapgen_id trap::map_regen_target() const
{
    return map_regen;
}

void trap::reset()
{
    funnel_traps.clear();
    sound_triggered_traps.clear();
    trap_factory.reset();
}

bool trap::is_trivial_to_spot() const
{

    return visibility <= 0 && !is_always_invisible();
}


bool trap::detected_by_ground_sonar() const
{
    return has_flag( json_flag_SONAR_DETECTABLE );
}




bool trap::detected_by_echolocation() const
{
    return has_flag( json_flag_ECHOLOCATION_DETECTABLE );
}

bool trap::detect_trap( const tripoint_bub_ms &pos, const Character &p ) const
{








    const float traps_skill_level = p.get_skill_level( skill_traps );



    const float weighted_stat_average = ( 4.0f * p.get_per() + p.get_int() ) / 5.0f;


    const float encumbrance_penalty = p.encumb( bodypart_id( "eyes" ) ) / 10.0f;



    const int distance_penalty = rl_dist( p.pos_bub(), pos ) - 1;

    int proficiency_effect = -1;

    if( p.has_proficiency( proficiency_prof_traps ) ) {
        proficiency_effect += 1;

    }
    if( p.has_proficiency( proficiency_prof_spotting ) ) {
        proficiency_effect += 3;

    }
    if( p.has_proficiency( proficiency_prof_trapsetting ) ) {
        proficiency_effect += 1;

    }



    const float sleepiness_penalty = p.get_sleep_deprivation() / 1000.0f;

    const float mean_roll = weighted_stat_average + ( traps_skill_level / 3.0f ) +
                            proficiency_effect - distance_penalty - sleepiness_penalty - encumbrance_penalty;

    const int roll = std::round( normal_roll( mean_roll, 3 ) );

    add_msg_debug( debugmode::DF_CHARACTER,
                   "Character %s rolling to detect trap %s. Actual roll: %i, average roll: %f, roll required to detect: %i.",
                   p.get_name(), name(), roll, mean_roll, visibility );

    return roll > visibility;
}


bool trap::can_see( const tripoint_bub_ms &pos, const Character &p ) const
{
    if( is_null() ) {

        return false;
    }
    if( is_always_invisible() ) {
        return false;
    }
    return visibility < 0 || p.knows_trap( pos );
}

void trap::trigger( const tripoint_bub_ms &pos ) const
{
    if( is_null() ) {
        return;
    }
    act( pos, nullptr, nullptr );
}

void trap::trigger( const tripoint_bub_ms &pos, Creature &creature ) const
{
    trigger( pos, &creature, nullptr );
}

void trap::trigger( const tripoint_bub_ms &pos, item &item ) const
{
    trigger( pos, nullptr, &item );
}

void trap::trigger( const tripoint_bub_ms &pos, Creature *creature, item *item ) const
{
    if( is_null() ) {
        return;
    }
    const bool is_real_creature = creature != nullptr && !creature->is_hallucination();
    if( is_real_creature || item != nullptr ) {
        bool triggered = act( pos, creature, item );
        if( triggered && is_real_creature ) {
            if( Character *ch = creature->as_character() ) {
                get_event_bus().send<event_type::character_triggers_trap>( ch->getID(), id );
            }
        }
    }
}

bool trap::is_null() const
{
    return loadid == tr_null;
}

bool trap::triggered_by_item( const item &itm ) const
{
    return !is_null() && itm.weight() >= trigger_weight;
}

bool trap::is_funnel() const
{
    return !is_null() && funnel_radius_mm > 0;
}

bool trap::has_sound_trigger() const
{
    const bool has_sound_thresh = sound_threshold.first > 0 && sound_threshold.second > 0;
    return !is_null() && has_sound_thresh;
}

bool trap::triggered_by_sound( int vol, int dist ) const
{
    const int volume = vol - dist;


    const int sound_min = sound_threshold.first;
    const int sound_max = sound_threshold.second;
    const int sound_range = sound_max - sound_min;
    if( volume < sound_min ) {
        return false;
    }
    int sound_chance = 100;
    if( sound_range > 0 ) {
        sound_chance = 25 + ( 75 * ( volume - sound_min ) / sound_range );
    }

    return !is_null() && ( rng( 0, 100 ) <= sound_chance );
}

void trap::on_disarmed( map &m, const tripoint_bub_ms &p ) const
{
    for( const trap::comp &i : components ) {
        m.spawn_item( p.xy(), i.item_type, i.quantity, i.charges );
    }
    for( const tripoint_bub_ms &dest : m.points_in_radius( p, trap_radius ) ) {
        m.remove_trap( dest );
    }
}



trap_id tr_null;

void trap::check_consistency()
{
    for( const trap &t : trap_factory.get_all() ) {
        for( const trap::comp &i : t.components ) {
            if( !item::type_is_defined( i.item_type ) ) {
                debugmsg( "trap %s has unknown item as component %s", t.id.str(), i.item_type.str() );
            }
        }
        if( t.sound_threshold.first > t.sound_threshold.second ) {
            debugmsg( "trap %s has higher min sound threshold than max and can never trigger", t.id.str() );
        }
        if( ( t.sound_threshold.first > 0 ) != ( t.sound_threshold.second > 0 ) ) {
            debugmsg( "trap %s has bad sound threshold of 0 and will trigger on anything", t.id.str() );
        }
    }
}

bool trap::easy_take_down() const
{
    return avoidance == 0 && difficulty == 0;
}

void trap::set_trap_data( itype_id trap_item_type_id )
{
    trap_item_type = trap_item_type_id;
}

bool trap::can_not_be_disarmed() const
{
    return difficulty >= 99;
}

void trap::finalize()
{

    loadid = id.id();
    if( is_funnel() ) {
        funnel_traps.push_back( this );
    }
    if( has_sound_trigger() ) {
        sound_triggered_traps.push_back( this );
    }
}

void trap::finalize_all()
{
    trap_factory.finalize();
    tr_null = trap_str_id::NULL_ID().id();
}

std::string trap::debug_describe() const
{
    return string_format( _( "Visible: %d\nAvoidance: %d\nDifficulty: %d\nBenign: %s" ), visibility,
                          avoidance, difficulty, is_benign() ? _( "Yes" ) : _( "No" ) );
}
