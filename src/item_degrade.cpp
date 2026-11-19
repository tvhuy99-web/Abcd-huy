



#include "item.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "fault.h"
#include "fire.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "item_category.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "item_transformation.h"
#include "itype.h"
#include "map.h"
#include "material.h"
#include "messages.h"
#include "mtype.h"
#include "options.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"
#include "weather.h"
#include "weather_gen.h"
#include "weighted_list.h"

static const item_category_id item_category_drugs( "drugs" );
static const item_category_id item_category_food( "food" );

namespace item_internal
{
static bool goes_bad_temp_cache = false;
static const item *goes_bad_temp_cache_for = nullptr;
static bool goes_bad_cache_fetch()
{
    return goes_bad_temp_cache;
}
static void goes_bad_cache_set( const item *i )
{
    goes_bad_temp_cache = i->goes_bad();
    goes_bad_temp_cache_for = i;
}
static void goes_bad_cache_unset()
{
    goes_bad_temp_cache = false;
    goes_bad_temp_cache_for = nullptr;
}
static bool goes_bad_cache_is_for( const item *i )
{
    return goes_bad_temp_cache_for == i;
}

namespace
{
struct scoped_goes_bad_cache {
    explicit scoped_goes_bad_cache( item *i ) {
        goes_bad_cache_set( i );
    }
    ~scoped_goes_bad_cache() {
        goes_bad_cache_unset();
    }
};
}
}

static const item *get_most_rotten_component( const item &craft )
{
    const item *most_rotten = nullptr;
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() ) {

            continue;
        }
        for( const item &it : tvp.second ) {
            if( !most_rotten || it.get_relative_rot() > most_rotten->get_relative_rot() ) {
                most_rotten = &it;
            }
        }
    }
    return most_rotten;
}

static time_duration get_shortest_lifespan_from_components( const item &craft )
{
    const item *shortest_lifespan_component = nullptr;
    time_duration shortest_lifespan = 0_turns;
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() ) {

            continue;
        }
        for( const item &it : tvp.second ) {
            time_duration lifespan = it.get_shelf_life() - it.get_rot();
            if( !shortest_lifespan_component || ( lifespan < shortest_lifespan ) ) {
                shortest_lifespan_component = &it;
                shortest_lifespan = shortest_lifespan_component->get_shelf_life() -
                                    shortest_lifespan_component->get_rot();
            }
        }
    }
    return shortest_lifespan;
}

static bool shelf_life_less_than_each_component( const item &craft )
{
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() || !tvp.second.front().is_comestible() ) {

            continue;
        }
        for( const item &it : tvp.second ) {
            if( it.get_shelf_life() < craft.get_shelf_life() ) {
                return false;
            }
        }
    }
    return true;
}












void item::inherit_rot_from_components( item &it )
{
    if( shelf_life_less_than_each_component( it ) ) {
        const time_duration shortest_lifespan = get_shortest_lifespan_from_components( it );
        if( shortest_lifespan > 0_turns && shortest_lifespan < it.get_shelf_life() ) {
            it.set_rot( it.get_shelf_life() - shortest_lifespan );
            return;
        }
        // Fallthrough: shortest_lifespan <= 0_turns (all components are rotten)
    }

    const item *most_rotten = get_most_rotten_component( it );
    if( most_rotten ) {
        it.set_relative_rot( most_rotten->get_relative_rot() );
    }
}

int item::damage() const
{
    return damage_;
}

int item::degradation() const
{
    int ret = degradation_;
    for( fault_id f : faults ) {
        ret += f.obj().degradation_mod();
    }
    return ret;
}

void item::rand_degradation()
{
    if( type->degrade_increments() == 0 ) {
        return;
    }
    set_degradation( rng( 0, damage() ) * 50.0f / type->degrade_increments() );
}

int item::damage_level( bool precise ) const
{
    return precise ? damage_ : type->damage_level( damage_ );
}

bool item::activation_success() const
{

    return rng( 0, 3999 ) - damage_ >= 0;
}

float item::damage_adjusted_melee_weapon_damage( float value, const damage_type_id &dt ) const
{
    if( type->count_by_charges() ) {
        return value;
    }

    for( fault_id fault : faults ) {
        for( const std::tuple<int, float, damage_type_id> &damage_mod : fault.obj().melee_damage_mod() ) {
            const damage_type_id dmg_type_of_fault = std::get<2>( damage_mod );
            if( dt == dmg_type_of_fault ) {
                const int damage_added = std::get<0>( damage_mod );
                const float damage_mult = std::get<1>( damage_mod );
                value = ( value + damage_added ) * damage_mult;
            }
        }
    }

    return std::max( 0.0f, value * ( 1.0f - 0.1f * std::max( 0, damage_level() - 1 ) ) );
}

float item::damage_adjusted_gun_damage( float value ) const
{
    if( type->count_by_charges() ) {
        return value;
    }
    return value - 2 * std::max( 0, damage_level() - 1 );
}

float item::damage_adjusted_armor_resist( float value, const damage_type_id &dmg_type ) const
{
    if( type->count_by_charges() ) {
        return value;
    }

    for( fault_id fault : faults ) {
        for( const std::tuple<int, float, damage_type_id> &damage_mod : fault.obj().armor_mod() ) {
            const damage_type_id dmg_type_of_fault = std::get<2>( damage_mod );
            if( dmg_type == dmg_type_of_fault ) {
                const int damage_added = std::get<0>( damage_mod );
                const float damage_mult = std::get<1>( damage_mod );
                value = ( value + damage_added ) * damage_mult;
            }
        }
    }

    return std::max( 0.0f, value * ( 1.0f - std::max( 0, damage_level() - 1 ) * 0.125f ) );
}

void item::set_damage( int qty )
{
    const int target_damage = std::clamp( qty, degradation_, max_damage() );
    const int delta = target_damage - damage_;
    mod_damage( delta );
}

void item::force_set_damage( int qty )
{
    damage_ = std::clamp( qty, degradation_, max_damage() );
}

void item::set_degradation( int qty )
{
    if( type->degrade_increments() > 0 ) {
        degradation_ = std::clamp( qty, 0, max_damage() );
    } else {
        degradation_ = 0;
    }

    force_set_damage( damage_ );
}

std::string item::degradation_symbol() const
{
    const int inc = max_damage() / 5;
    const int dgr_lvl = degradation() / ( inc > 0 ? inc : 1 );
    std::string dgr_symbol;

    switch( dgr_lvl ) {
        case 0:
            dgr_symbol = colorize( "\u2588", c_light_green );
            break;
        case 1:
            dgr_symbol = colorize( "\u2587", c_yellow );
            break;
        case 2:
            dgr_symbol = colorize( "\u2585", c_magenta );
            break;
        case 3:
            dgr_symbol = colorize( "\u2583", c_light_red );
            break;
        default:
            dgr_symbol = colorize( "\u2581", c_red );
            break;
    }
    return type->degrade_increments() == 0 ? "" : dgr_symbol;
}

bool item::has_fault( const fault_id &fault ) const
{
    return faults.count( fault );
}

bool item::has_fault_of_type( const std::string &fault_type ) const
{
    for( const fault_id &f : faults ) {
        if( f.obj().type() == fault_type ) {
            return true;
        }
    }
    return false;
}

bool item::has_fault_flag( const std::string &searched_flag ) const
{
    for( const fault_id &fault : faults ) {
        if( fault->has_flag( searched_flag ) ) {
            return true;
        }
    }
    return false;
}

std::string item::get_fault_description( const fault_id &f_id ) const
{
    return string_format( f_id.obj().description(), type->nname( 1 ) );
}

bool item::can_have_fault( const fault_id &f_id )
{

    if( type->faults.get_specific_weight( f_id ) == 0 ) {
        return false;
    }


    for( const fault_id &faults_of_item : faults ) {
        for( const fault_id &blocked_fault : faults_of_item.obj().get_block_faults() ) {
            if( f_id == blocked_fault ) {
                return false;
            }
        }
    }

    return true;
}

bool item::set_fault( const fault_id &f_id, bool force, const Character *holder )
{
    if( !force && !can_have_fault( f_id ) ) {
        return false;
    }



    for( const fault_id &f_blocked : f_id.obj().get_block_faults() ) {
        remove_fault( f_blocked );
    }

    if( holder != nullptr && holder->is_avatar() && holder->has_item( *this ) ) {
        const std::string raw = f_id.obj().message();
        if( !raw.empty() ) {
            holder->add_msg_if_player( m_bad, string_format( raw, tname() ) );
        }
    }

    faults.insert( f_id );
    mod_damage( f_id->instant_damage(), holder );
    return true;
}

void item::set_random_fault_of_type( const std::string &fault_type, bool force,
                                     const Character *holder )
{
    if( force ) {
        set_fault( random_entry( faults::all_of_type( fault_type ) ), true, holder );
        return;
    }

    weighted_int_list<fault_id> faults_by_type;
    for( const std::pair<fault_id, int> &f : type->faults ) {
        if( f.first.obj().type() == fault_type && can_have_fault( f.first ) ) {
            faults_by_type.add( f.first, f.second );
        }

    }

    if( !faults_by_type.empty() ) {
        set_fault( *faults_by_type.pick(), force, holder );
    }

}

bool item::remove_fault( const fault_id &fault_id )
{
    size_t count = faults.size();
    faults.erase( fault_id );
    return count != faults.size();
}

void item::remove_single_fault_of_type( const std::string &fault_type )
{
    for( const fault_id f : faults ) {
        if( f.obj().type() == fault_type ) {
            faults.erase( f );
            return;
        }
    }
}

bool item::goes_bad() const
{
    if( item_internal::goes_bad_cache_is_for( this ) ) {
        return item_internal::goes_bad_cache_fetch();
    }
    if( has_flag( flag_PROCESSING ) ) {
        return false;
    }
    if( is_corpse() ) {


        if( corpse->has_flag( mon_flag_DORMANT ) ) {
            return false;
        }
        return made_of_any( materials::get_rotting() );
    }
    return is_comestible() && get_comestible()->spoils != 0_turns;
}

time_duration item::get_shelf_life() const
{
    if( goes_bad() ) {
        if( is_comestible() ) {
            return get_comestible()->spoils;
        } else if( is_corpse() ) {
            return 24_hours;
        }
    }
    return 0_turns;
}

double item::get_relative_rot() const
{
    if( goes_bad() ) {
        return rot / get_shelf_life();
    }
    return 0;
}

void item::set_relative_rot( double val )
{
    if( goes_bad() ) {
        rot = get_shelf_life() * val;



        if( !has_flag( flag_PROCESSING_RESULT ) ) {
            last_temp_check = calendar::turn;
        }
    }
}

void item::set_rot( time_duration val )
{
    rot = val;
}

void item::randomize_rot()
{
    if( is_comestible() && get_comestible()->spoils > 0_turns ) {
        const time_duration age_from_zero = calendar::turn - calendar::start_of_cataclysm;
        const time_duration cap = get_shelf_life() * 2;
        time_duration base_rot = std::min( age_from_zero, cap );
        const double x_input = rng_float( 0.0, 1.0 );
        const double k_rot = ( 1.0 - x_input ) / ( 1.0 + 2.0 * x_input );
        time_duration extra = get_shelf_life() * 0.25 * k_rot;
        set_rot( std::min( base_rot + extra, cap ) );


        if( !has_flag( flag_PROCESSING_RESULT ) ) {
            last_temp_check = calendar::turn;
        }
    }

    for( item_pocket *pocket : contents.get_container_pockets() ) {
        if( pocket->spoil_multiplier() > 0.0f ) {
            for( item *subitem : pocket->all_items_top() ) {
                subitem->randomize_rot();
            }
        }
    }
}

int item::spoilage_sort_order() const
{
    int bottom = std::numeric_limits<int>::max();

    bool any_goes_bad = false;
    time_duration min_spoil_time = calendar::INDEFINITELY_LONG_DURATION;
    visit_items( [&]( const item * node, const item * parent ) {
        if( node && node->goes_bad() ) {
            float spoil_multiplier = 1.0f;
            if( parent != nullptr ) {
                const item_pocket *const parent_pocket = parent->contained_where( *node );
                if( parent_pocket ) {
                    spoil_multiplier = parent_pocket->spoil_multiplier();
                }
            }
            if( spoil_multiplier > 0.0f ) {
                time_duration remaining_shelf_life = node->get_shelf_life() - node->rot;
                if( !any_goes_bad || min_spoil_time * spoil_multiplier > remaining_shelf_life ) {
                    any_goes_bad = true;
                    min_spoil_time = remaining_shelf_life / spoil_multiplier;
                }
            }
        }
        return VisitResponse::NEXT;
    } );
    if( any_goes_bad ) {
        return to_turns<int>( min_spoil_time );
    }

    if( get_comestible() ) {
        if( get_category_shallow().get_id() == item_category_food ) {
            return bottom - 3;
        } else if( get_category_shallow().get_id() == item_category_drugs ) {
            return bottom - 2;
        } else {
            return bottom - 1;
        }
    }
    return bottom;
}









float item::calc_hourly_rotpoints_at_temp( const units::temperature &temp ) const
{
    const units::temperature dropoff = units::from_fahrenheit( 38 );
    const float max_rot_temp = 105;


    if( has_flag( flag_CRYOGENIC_ROT ) && temp > units::from_fahrenheit( -320 ) ) {
        return 3600.f;
    }
    if( temp <= temperatures::freezing ) {
        return 0.f;
    } else if( temp < dropoff ) {

        return 600.f * std::exp2( -27.f / 16.f ) * ( units::to_fahrenheit( temp ) - units::to_fahrenheit(
                    temperatures::freezing ) );
    } else if( temp < units::from_fahrenheit( max_rot_temp ) ) {

        return 3600.f * std::exp2( ( units::to_fahrenheit( temp ) - 65.f ) / 16.f );
    } else {


        return 3600.f * std::exp2( ( max_rot_temp - 65.f ) / 16.f );
    }
}

void item::calc_rot( units::temperature temp, const float spoil_modifier,
                     const time_duration &time_delta )
{




    if( !is_corpse() && get_relative_rot() > 2.0 ) {
        return;
    }

    if( has_own_flag( flag_FROZEN ) ) {
        return;
    }


    float factor = spoil_modifier;
    if( is_corpse() && has_flag( flag_FIELD_DRESS ) ) {
        factor *= 0.75;
    }
    if( has_own_flag( flag_MUSHY ) ) {
        factor *= 3.0;
    }






    if( has_own_flag( flag_IRRADIATED ) ) {
        factor *= 0.25;
    }

    if( has_own_flag( flag_COLD ) ) {
        temp = std::min( temperatures::fridge, temp );
    }

    rot += factor * time_delta / 1_seconds * calc_hourly_rotpoints_at_temp( temp ) * 1_turns /
           ( 1_hours / 1_seconds );
}

void item::calc_rot_while_processing( time_duration processing_duration )
{
    if( !has_own_flag( flag_PROCESSING ) ) {
        debugmsg( "calc_rot_while_processing called on non smoking item: %s", tname() );
        return;
    }


    last_temp_check += processing_duration;
}

bool item::process_decay_in_air( map &here, Character *carrier, const tripoint_bub_ms &pos,
                                 int max_air_exposure_hours,
                                 time_duration time_delta )
{
    if( !has_own_flag( flag_FROZEN ) ) {
        double environment_multiplier = here.is_outside( pos ) ? 2.0 : 1.0;
        time_duration new_air_exposure = time_duration::from_seconds( item_counter ) + time_delta *
                                         rng_normal( 0.9, 1.1 ) * environment_multiplier;
        if( new_air_exposure >= time_duration::from_hours( max_air_exposure_hours ) ) {
            type->transform_into.value().transform( carrier, *this, true );
            return true;
        }
        item_counter = to_seconds<int>( new_air_exposure );
    }
    return false;
}

#if defined(_MSC_VER)

#pragma optimize( "", off )
#endif

bool item::damage_type_can_damage_items( const damage_type_id &dmg_type ) const
{
    return dmg_type->physical;
}

template<typename bodypart_target>
float item::resist( const damage_type_id &dmg_type, const bool to_self,
                    const bodypart_target &bp,
                    const int resist_value ) const
{
    if( is_null() ) {
        return 0.0f;
    }

    if( dmg_type.is_null() ) {
        return 0.0f;
    }

    if( !dmg_type.is_valid() ) {
        debugmsg( "Invalid damage type: %d", dmg_type.c_str() );
        return 0.0f;
    }




    if( to_self && !damage_type_can_damage_items( dmg_type ) ) {
        return std::numeric_limits<float>::max();
    }

    const bool bp_null = bp == bodypart_target();
    const std::vector<const part_material *> &armor_mats = armor_made_of( bp );
    const float avg_thickness = bp_null ? get_thickness() : get_thickness( bp );
    return _resist( dmg_type, to_self, resist_value, bp_null, armor_mats, avg_thickness );
}
template float item::resist<bodypart_id>( const damage_type_id &dmg_type,
        const bool to_self, const bodypart_id &bp, const int resist_value ) const;
template float item::resist<sub_bodypart_id>( const damage_type_id &dmg_type,
        const bool to_self, const sub_bodypart_id &bp, const int resist_value ) const;

float item::_resist( const damage_type_id &dmg_type, bool to_self, int resist_value,
                     const bool bp_null,
                     const std::vector<const part_material *> &armor_mats,
                     const float avg_thickness ) const
{
    if( dmg_type->env ) {
        return _environmental_resist( dmg_type, to_self, resist_value, bp_null, armor_mats );
    }

    std::optional<std::pair<damage_type_id, float>> derived;
    if( !dmg_type->derived_from.first.is_null() ) {
        derived = dmg_type->derived_from;
    }

    float resist = 0.0f;
    float mod = get_clothing_mod_val_for_damage_type( dmg_type );

    const float damage_scale = damage_adjusted_armor_resist( 1.0f, dmg_type );

    if( !bp_null ) {

        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {



                int internal_roll;
                resist_value < 0 ? internal_roll = rng( 0, 99 ) : internal_roll = resist_value;
                if( internal_roll < m->cover ) {
                    float tmp_add = 0.f;
                    if( derived.has_value() && !m->id->has_dedicated_resist( dmg_type ) ) {
                        tmp_add = m->id->resist( derived->first ) * m->thickness * derived->second;
                    } else {
                        tmp_add = m->id->resist( dmg_type ) * m->thickness;
                    }
                    resist += tmp_add;
                }
            }
            return ( resist + mod ) * damage_scale;
        }
    }


    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
        for( const auto &m : mats ) {
            float tmp_add = 0.f;
            if( derived.has_value() && !m.first->has_dedicated_resist( dmg_type ) ) {
                tmp_add = m.first->resist( derived->first ) * m.second * derived->second;
            } else {
                tmp_add = m.first->resist( dmg_type ) * m.second;
            }
            resist += tmp_add;
        }

        resist /= total;
    }

    return ( resist * avg_thickness + mod ) * damage_scale;
}

float item::_environmental_resist( const damage_type_id &dmg_type, const bool to_self,
                                   int base_env_resist,
                                   const bool bp_null,
                                   const std::vector<const part_material *> &armor_mats ) const
{
    if( to_self ) {

        return std::numeric_limits<float>::max();
    }

    std::optional<std::pair<damage_type_id, float>> derived;
    if( !dmg_type->derived_from.first.is_null() ) {
        derived = dmg_type->derived_from;
    }

    float resist = 0.0f;
    float mod = get_clothing_mod_val_for_damage_type( dmg_type );

    if( !bp_null ) {

        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                float tmp_add = 0.f;
                if( derived.has_value() && !m->id->has_dedicated_resist( dmg_type ) ) {
                    tmp_add = m->id->resist( derived->first ) * m->cover * 0.01f * derived->second;
                } else {
                    tmp_add = m->id->resist( dmg_type ) * m->cover * 0.01f;
                }
                resist += tmp_add;
            }
            const int env = get_env_resist( base_env_resist );
            if( env < 10 ) {
                resist *= env / 10.0f;
            }
        }
        return resist + mod;
    }

    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;


        for( const auto &m : mats ) {
            float tmp_add = 0.f;
            if( derived.has_value() && !m.first->has_dedicated_resist( dmg_type ) ) {
                tmp_add = m.first->resist( derived->first ) * m.second * derived->second;
            } else {
                tmp_add = m.first->resist( dmg_type ) * m.second;
            }
            resist += tmp_add;
        }

        resist /= total;
    }

    const int env = get_env_resist( base_env_resist );
    if( env < 10 ) {

        resist *= env / 10.0f;
    }

    return resist + mod;
}

#if defined(_MSC_VER)
#pragma optimize( "", on )
#endif

int item::max_damage() const
{
    return type->damage_max();
}

float item::get_relative_health() const
{
    const int max_dmg = max_damage();
    if( max_dmg == 0 ) {
        return 1.0f;
    }
    return 1.0f - static_cast<float>( damage() ) / max_dmg;
}

static int get_degrade_amount( const item &it, int dmgNow_, int dmgPrev )
{
    const int max_dmg = it.max_damage();
    const int degrade_increments = it.type->degrade_increments();
    if( max_dmg == 0 || degrade_increments == 0 ) {
        return 0;
    }
    const int facNow_ = dmgNow_ == 0 ? -1 : ( dmgNow_ - 1 ) * 5 / max_dmg;
    const int facPrev = dmgPrev == 0 ? -1 : ( dmgPrev - 1 ) * 5 / max_dmg;

    return std::max( facNow_ - facPrev, 0 ) * max_dmg / degrade_increments;
}

bool item::mod_damage( int qty, const Character *holder )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }
    if( count_by_charges() ) {
        charges -= std::min( type->stack_size * qty / itype::damage_scale, charges );
        return charges == 0;
    } else {
        const int dmg_before = damage_;
        const bool destroy = ( damage_ + qty ) > max_damage();
        force_set_damage( damage_ + qty );

        if( qty > 0 && !destroy ) {
            set_degradation( degradation_ + get_degrade_amount( *this, damage_, dmg_before ) );
        }


        if( qty > 0 ) {
            for( int i = 0; i <= qty; i += itype::damage_scale ) {
                set_random_fault_of_type( "mechanical_damage", false, holder );
            }
        }

        return destroy;
    }
}

bool item::inc_damage( const Character *holder )
{
    return mod_damage( itype::damage_scale, holder );
}

int item::repairable_levels() const
{
    const int levels = type->damage_level( damage_ ) - type->damage_level( degradation_ );

    return levels > 0
           ? levels
           : damage() > degradation();
}

item::armor_status item::damage_armor_durability( damage_unit &du, damage_unit &premitigated,
        const bodypart_id &bp,
        double enchant_multiplier,
        const Character *holder )
{


    if( has_var( "ENERGY_SHIELD_HP" ) && resist( du.type, false, bp ) > 0.0f ) {
        double shield_hp = get_var( "ENERGY_SHIELD_HP", 0.0 );
        shield_hp -= premitigated.amount;
        set_var( "ENERGY_SHIELD_HP", shield_hp );
        if( shield_hp > 0 ) {
            return armor_status::UNDAMAGED;
        } else {

            mod_damage( itype::damage_scale * 6, holder );
            return armor_status::DESTROYED;
        }
    }

    if( has_flag( flag_UNBREAKABLE ) || enchant_multiplier <= 0.0f ) {
        return armor_status::UNDAMAGED;
    }


    double armors_own_resist = resist( du.type, true, bp );
    if( armors_own_resist > 1000.0 ) {

        return armor_status::UNDAMAGED;
    }












    armors_own_resist = has_flag( flag_FRAGILE ) ? armors_own_resist * 0.6666 : std::max( 10.0,
                        armors_own_resist * 1.3333 );
    double k = has_flag( flag_FRAGILE ) ? 0.1 : 1.0;
    double damage_chance = 0.8 / ( 1.0 + std::exp( -k * ( premitigated.amount - armors_own_resist ) ) )
                           * enchant_multiplier;




    if( !has_flag( flag_FRAGILE ) ) {
        damage_chance = damage_chance / get_covered_body_parts().count();
    }
    add_msg_debug( debugmode::DF_CREATURE, "Chance to degrade armor %s: %.2f%%",
                   tname(), damage_chance * 100.0 );
    if( has_flag( flag_STURDY ) && premitigated.amount < armors_own_resist ) {
        return armor_status::UNDAMAGED;
    } else if( x_in_y( damage_chance, 1.0 ) ) {
        return mod_damage( itype::damage_scale * enchant_multiplier, holder ) ? armor_status::DESTROYED :
               armor_status::DAMAGED;
    }
    return armor_status::UNDAMAGED;
}


item::armor_status item::damage_armor_transforms( damage_unit &du, double enchant_multiplier ) const
{
    if( enchant_multiplier <= 0.0f ) {
        return armor_status::UNDAMAGED;
    }


    const float armors_own_resist = resist( du.type, true );



    if( du.amount / armors_own_resist < 0.2f ) {
        return armor_status::UNDAMAGED;
    }



    float break_chance = 33.3f * ( du.amount / armors_own_resist ) * enchant_multiplier;

    float roll_to_break = rng_float( 0.0, 100.0 );

    if( roll_to_break < break_chance ) {

        return armor_status::TRANSFORMED;
    }
    return armor_status::UNDAMAGED;
}

std::string item::damage_indicator() const
{
    return cata::options::damage_indicators[damage_level()];
}

std::string item::durability_indicator( bool include_intact ) const
{
    const std::string item_health_option = get_option<std::string>( "ITEM_HEALTH" );
    const bool show_bars_only = item_health_option == "bars";
    const bool show_description_only = item_health_option == "descriptions";
    const bool show_both = item_health_option == "both";
    const bool show_bars = show_both || show_bars_only;
    const bool show_description = show_both || show_description_only;
    std::string bars;
    std::string description;
    if( show_bars ) {
        bars = damage_indicator() + degradation_symbol() + "\u00A0";
    }
    if( damage() < 0 ) {
        if( show_description ) {
            if( is_gun() ) {
                description = pgettext( "damage adjective", "accurized " );
            } else {
                description = pgettext( "damage adjective", "reinforced " );
            }
        }
        if( show_bars_only ) {
            return bars;
        }
        if( show_description_only ) {
            return description;
        }
        return bars + description;
    }
    if( has_flag( flag_CORPSE ) ) {
        if( damage() > 0 ) {
            switch( damage_level() ) {
                case 1:
                    return pgettext( "damage adjective", "bruised " );
                case 2:
                    return pgettext( "damage adjective", "damaged " );
                case 3:
                    return pgettext( "damage adjective", "mangled " );
                default:
                    return pgettext( "damage adjective", "pulped " );
            }
        }
    }
    if( show_bars_only ) {
        return bars;
    }
    description = string_format( "%s ", get_base_material().dmg_adj( damage_level() ) );
    if( description == " " ) {
        if( include_intact ) {
            description = _( "fully intact " );
        } else {
            description = "";
        }
    }
    if( show_description_only ) {
        return description;
    }
    return bars + description;
}

const std::set<itype_id> &item::repaired_with() const
{
    static std::set<itype_id> no_repair;
    static std::set<itype_id> tools;
    tools.clear();

    std::copy_if( type->repair.begin(), type->repair.end(), std::inserter( tools, tools.begin() ),
    []( const itype_id & i ) {
        return !item_is_blacklisted( i );
    } );

    return has_flag( flag_NO_REPAIR ) ? no_repair : tools;
}

bool item::can_repair_with( const material_id &mat_ident ) const
{
    auto mat = type->repairs_with.find( mat_ident );
    return mat != type->repairs_with.end();
}

bool item::is_broken() const
{
    constexpr uint64_t bit = static_cast<uint64_t>( hot_flag_bit::ITEM_BROKEN );
    if( combined_hot_flags() & bit ) {
        return true;
    }
    return has_fault_flag( std::string( "ITEM_BROKEN" ) );
}

bool item::is_broken_on_active() const
{
    constexpr uint64_t bit = static_cast<uint64_t>( hot_flag_bit::ITEM_BROKEN );
    if( combined_hot_flags() & bit ) {
        return true;
    }
    if( has_fault_flag( std::string( "ITEM_BROKEN" ) ) ) {
        return true;
    }
    return wetness && has_flag( flag_WATER_BREAK_ACTIVE );
}

std::set<fault_id> item::faults_potential() const
{
    std::set<fault_id> res;
    for( const std::pair<fault_id, int> &fault_pair : type->faults ) {
        res.insert( fault_pair.first );
    }
    return res;
}

std::set<fault_id> item::faults_potential_of_type( const std::string &fault_type ) const
{
    std::set<fault_id> res;
    for( const std::pair<fault_id, int> &some_fault : type->faults ) {
        if( some_fault.first->type() == fault_type ) {
            res.emplace( some_fault.first );
        }
    }
    return res;
}

float item::simulate_burn( fire_data &frd ) const
{
    const std::map<material_id, int> &mats = made_of();
    float smoke_added = 0.0f;
    float time_added = 0.0f;
    float burn_added = 0.0f;
    const units::volume vol = base_volume();
    const int effective_intensity = frd.contained ? 3 : frd.fire_intensity;
    for( const auto &m : mats ) {
        const mat_burn_data &bd = m.first->burn_data( effective_intensity );
        if( bd.immune ) {

            return 0.0f;
        }


        if( frd.contained || bd.volume_per_turn == 0_ml ) {
            time_added += bd.fuel * m.second;
            smoke_added += bd.smoke * m.second;
            burn_added += bd.burn * m.second;
        } else {
            double volume_burn_rate = to_liter( bd.volume_per_turn ) / to_liter( vol );
            time_added += bd.fuel * volume_burn_rate * m.second;
            smoke_added += bd.smoke * volume_burn_rate * m.second;
            burn_added += bd.burn * volume_burn_rate * m.second;
        }
    }
    const int mat_total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;


    if( made_of( phase_id::LIQUID ) && time_added < 200 ) {
        time_added -= rng( 400.0 * to_liter( vol ), 1200.0 * to_liter( vol ) );
    } else if( mats.size() > 1 ) {

        time_added /= mat_total;
        smoke_added /= mat_total;
        burn_added /= mat_total;
    } else if( mats.empty() ) {

        burn_added = 1;
    }


    if( count_by_charges() ) {
        int stack_burnt = std::max( 1, count() / type->stack_size );
        time_added *= stack_burnt;
        smoke_added *= stack_burnt;
        burn_added *= stack_burnt;
    }

    frd.fuel_produced += time_added;
    frd.smoke_produced += smoke_added;
    return burn_added;
}

bool item::burn( fire_data &frd )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }
    float burn_added = simulate_burn( frd );

    if( burn_added <= 0 ) {
        return false;
    }

    if( count_by_charges() ) {
        if( type->volume == 0_ml ) {
            charges = 0;
        } else {
            charges -= roll_remainder( burn_added * 250_ml * type->stack_size /
                                       ( 3.0 * type->volume ) );
        }

        return charges <= 0;
    }

    if( is_corpse() ) {
        const mtype *mt = get_mtype();
        if( active && mt != nullptr && burnt + burn_added > mt->hp &&
            !mt->burn_into.is_null() && mt->burn_into.is_valid() ) {
            corpse = &get_mtype()->burn_into.obj();

            set_age( 0_turns );
            burnt = 0;
            return false;
        }
    } else if( has_temperature() ) {
        heat_up();
    }

    contents.heat_up();

    burnt += roll_remainder( burn_added );

    const int vol = base_volume() / 250_ml;
    return burnt >= vol * 3;
}

bool item::flammable( int threshold ) const
{
    const std::map<material_id, int> &mats = made_of();
    if( mats.empty() ) {

        return false;
    }

    int flammability = 0;
    units::volume volume_per_turn = 0_ml;
    for( const std::pair<const material_id, int> &m : mats ) {
        const mat_burn_data &bd = m.first->burn_data( 1 );
        if( bd.immune ) {

            return false;
        }

        flammability += bd.fuel * m.second;
        volume_per_turn += bd.volume_per_turn * m.second;
    }
    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    flammability /= total;
    volume_per_turn /= total;

    if( threshold == 0 || flammability <= 0 ) {
        return flammability > 0;
    }

    units::volume vol = base_volume();
    if( volume_per_turn > 0_ml && volume_per_turn < vol ) {
        flammability = flammability * volume_per_turn / vol;
    } else {

        flammability = flammability * vol / 250_ml;
    }

    return flammability > threshold;
}

bool item::will_explode_in_fire() const
{
    if( type->explode_in_fire ) {
        return true;
    }

    if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        return true;
    }

    return false;
}

bool item::has_rotten_away() const
{
    if( is_corpse() && !can_revive() ) {
        return get_rot() > 10_days;
    } else {
        return get_relative_rot() > 2.0;
    }
}

void item::apply_freezerburn()
{
    if( !has_flag( flag_FREEZERBURN ) ) {
        return;
    }
    set_flag( flag_MUSHY );
}

bool item::process_temperature_rot( float insulation, const tripoint_bub_ms &pos, map &here,
                                    Character *carrier, const temperature_flag flag, float spoil_modifier, bool watertight_container )
{
    const time_point now = calendar::turn;



    if( now - last_temp_check < 0_turns ) {
        reset_temp_check();
        return false;
    }



    time_duration smallest_interval = 10_minutes;
    if( now - last_temp_check < smallest_interval && units::to_joule_per_gram( specific_energy ) > 0 ) {
        return false;
    }

    units::temperature temp = get_weather().get_temperature( pos );

    switch( flag ) {
        case temperature_flag::NORMAL:

            break;
        case temperature_flag::FRIDGE:
            temp = std::min( temp, temperatures::fridge );
            break;
        case temperature_flag::FREEZER:
            temp = std::min( temp, temperatures::freezer );
            break;
        case temperature_flag::HEATER:
            temp = std::max( temp, temperatures::normal );
            break;
        case temperature_flag::ROOT_CELLAR:
            temp = units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
            break;
        default:
            debugmsg( "Temperature flag enum not valid.  Using current temperature." );
    }

    bool carried = carrier != nullptr;

    if( carried ) {
        insulation *= 1.5;
        temp += units::from_fahrenheit_delta( 5 );
    }

    time_point time = last_temp_check;
    item_internal::scoped_goes_bad_cache _cache( this );
    const bool process_rot = goes_bad() && spoil_modifier != 0;
    const bool decays_in_air = !watertight_container && has_flag( flag_DECAYS_IN_AIR ) &&
                               type->transform_into;
    int64_t max_air_exposure_hours = decays_in_air ? get_property_int64_t( "max_air_exposure_hours" ) :
                                     0;

    if( now - time > 1_hours ) {


        const weather_generator &wgen = get_weather().get_cur_weather_gen();
        const unsigned int seed = g->get_seed();

        units::temperature_delta temp_mod;

        if( !g->new_game && !g->swapping_dimensions ) {
            temp_mod = get_heat_radiation( pos );
            temp_mod += get_convection_temperature( pos );
            temp_mod += here.get_temperature_mod( pos );
        } else {
            temp_mod = units::from_kelvin_delta( 0 );
        }

        if( carried ) {
            temp_mod += units::from_fahrenheit_delta( 5 );
        }


        time_duration time_delta = 1_hours;

        while( now - time > 1_hours ) {
            time += time_delta;



            units::temperature env_temperature;
            if( pos.z() >= 0 && flag != temperature_flag::ROOT_CELLAR ) {
                env_temperature = wgen.get_weather_temperature( get_map().get_abs( pos ), time, seed );
            } else {
                env_temperature = units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
            }
            env_temperature += temp_mod;

            switch( flag ) {
                case temperature_flag::NORMAL:

                    break;
                case temperature_flag::FRIDGE:
                    env_temperature = std::min( env_temperature, temperatures::fridge );
                    break;
                case temperature_flag::FREEZER:
                    env_temperature = std::min( env_temperature, temperatures::freezer );
                    break;
                case temperature_flag::HEATER:
                    env_temperature = std::max( env_temperature, temperatures::normal );
                    break;
                case temperature_flag::ROOT_CELLAR:
                    env_temperature =  units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
                    break;
                default:
                    debugmsg( "Temperature flag enum not valid.  Using normal temperature." );
            }



            if( now - time < 2_days ) {
                calc_temp( env_temperature, insulation, time_delta );
            }
            last_temp_check = time;

            if( decays_in_air &&
                process_decay_in_air( here, carrier, pos, max_air_exposure_hours, time_delta ) ) {
                return false;
            }


            if( process_rot ) {
                calc_rot( env_temperature, spoil_modifier, time_delta );

                if( has_rotten_away() && carrier == nullptr ) {

                    return true;
                }
            }
        }
    }



    if( now - time > smallest_interval ) {
        calc_temp( temp, insulation, now - time );
        last_temp_check = now;

        if( decays_in_air &&
            process_decay_in_air( here, carrier, pos, max_air_exposure_hours, now - time ) ) {
            return false;
        }

        if( process_rot ) {
            calc_rot( temp, spoil_modifier, now - time );
            return has_rotten_away() && carrier == nullptr;
        }
    }


    if( units::to_joule_per_gram( specific_energy ) < 0 ) {
        set_item_temperature( temp );
    }
    return false;
}

bool item::is_filthy() const
{
    return has_flag( flag_FILTHY );
}

time_duration item::age() const
{
    return calendar::turn - birthday();
}

void item::set_age( const time_duration &age )
{
    set_birthday( time_point( calendar::turn ) - age );
}

time_point item::birthday() const
{
    return bday;
}

void item::set_birthday( const time_point &bday )
{
    this->bday = std::max( calendar::turn_zero, bday );
}

void rot_spawn_data::load( const JsonObject &jo, bool was_loaded )
{
    optional( jo, was_loaded, "monster", rot_spawn_monster, mtype_id::NULL_ID() );
    optional( jo, was_loaded, "group", rot_spawn_group, mongroup_id::NULL_ID() );
    optional( jo, was_loaded, "chance", rot_spawn_chance );
    optional( jo, was_loaded, "amount", rot_spawn_monster_amount, { 1, 1 } );
}

bool rot_spawn_data::handle_extend( const JsonValue &jv )
{
    JsonObject jo = jv.get_object();
    load( jo, true );
    return true;
}

void rot_spawn_data::deserialize( const JsonObject &jo )
{
    load( jo, false );
}
