#include "vehicle.h" // IWYU pragma: associated
#include "a11y_android.h"
#include "vehicle_autodrive_tiles.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <tuple>

#include "avatar.h"
#include "bodypart.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "effect_source.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "options.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "vpart_range.h"

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_stunned( "stunned" );

static const fault_id fault_axle_broken( "fault_axle_broken" );
static const fault_id fault_cracked_rim( "fault_cracked_rim" );
static const fault_id fault_flat_tire_riding_on_rims( "fault_flat_tire_riding_on_rims" );
static const fault_id fault_punctured_tires( "fault_punctured_tires" );

static const flag_id json_flag_CANNOT_TAKE_DAMAGE( "CANNOT_TAKE_DAMAGE" );
static const flag_id json_flag_PUNCTURE_VEHICLE_WHEELS( "PUNCTURE_VEHICLE_WHEELS" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );

static const proficiency_id proficiency_prof_boat_pilot( "prof_boat_pilot" );
static const proficiency_id proficiency_prof_driver( "prof_driver" );

static const skill_id skill_driving( "driving" );

static const ter_str_id ter_t_railroad_track( "t_railroad_track" );
static const ter_str_id ter_t_railroad_track_d( "t_railroad_track_d" );
static const ter_str_id ter_t_railroad_track_d1( "t_railroad_track_d1" );
static const ter_str_id ter_t_railroad_track_d2( "t_railroad_track_d2" );
static const ter_str_id ter_t_railroad_track_d_on_tie( "t_railroad_track_d_on_tie" );
static const ter_str_id ter_t_railroad_track_h( "t_railroad_track_h" );
static const ter_str_id ter_t_railroad_track_h_on_tie( "t_railroad_track_h_on_tie" );
static const ter_str_id ter_t_railroad_track_on_tie( "t_railroad_track_on_tie" );
static const ter_str_id ter_t_railroad_track_v( "t_railroad_track_v" );
static const ter_str_id ter_t_railroad_track_v_on_tie( "t_railroad_track_v_on_tie" );

static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );

static const trap_str_id tr_thick_ice( "tr_thick_ice" );

static const vpart_location_id vpart_location_structure( "structure" );


static const float tile_height = 4.0f;

static const int mi_to_vmi = 100;

static const float mps_to_miph = 2.23694f;


int mps_to_vmiph( double mps )
{
    return mps * mps_to_miph * mi_to_vmi;
}


double vmiph_to_mps( int vmiph )
{
    return vmiph / mps_to_miph / mi_to_vmi;
}

int cmps_to_vmiph( int cmps )
{
    return cmps * mps_to_miph;
}

int vmiph_to_cmps( int vmiph )
{
    return vmiph / mps_to_miph;
}

int vehicle::slowdown( map &here, int at_velocity ) const
{
    double mps = vmiph_to_mps( std::abs( at_velocity ) );


    double f_total_drag = coeff_air_drag() * mps * mps;
    if( is_watercraft() ) {

        f_total_drag += coeff_water_drag( here ) * mps * mps;
    } else if( !is_falling && !is_flying ) {
        double f_rolling_drag = coeff_rolling_drag( here ) * ( vehicles::rolling_constant_to_variable +
                                mps );

        const double skid_factor = 1 + 24 * std::abs( units::sin( face.dir() - move.dir() ) );
        f_total_drag += f_rolling_drag * skid_factor;
    }

    double accel_slowdown = total_mass( here ).value() > 0 ? f_total_drag / to_kilogram( total_mass(
                                here ) ) : 0;

    int slowdown = mps_to_vmiph( accel_slowdown );
    if( is_towing() ) {
        vehicle *other_veh = tow_data.get_towed();
        if( other_veh ) {
            slowdown += other_veh->slowdown( here,  at_velocity );
        }
    }
    if( slowdown < 0 ) {
        debugmsg( "vehicle %s has negative drag slowdown %d\n", name, slowdown );
    }
    add_msg_debug( debugmode::DF_VEHICLE_MOVE,
                   "%s at %d vimph, f_drag %3.2f, drag accel %d vmiph - extra drag %d",
                   name, at_velocity, f_total_drag, slowdown, units::to_watt( static_drag() ) );

    if( !( is_falling || ( is_watercraft() && can_float( here ) ) || is_flying ) ) {
        slowdown -= units::to_watt( static_drag() );
    }

    return std::max( 1, slowdown );
}

void vehicle::smart_controller_handle_turn( map &here,
        const std::optional<float> &k_traction_cache )
{

    smart_controller_config cfg = smart_controller_cfg.value_or( smart_controller_config() );

    if( !has_enabled_smart_controller ) {
        smart_controller_state = std::nullopt;
        return;
    }

    if( smart_controller_state && smart_controller_state->created == calendar::turn ) {
        return;
    }



    std::vector<int> c_engines;
    bool has_electric_engine = false;
    for( int i = 0; i < static_cast<int>( engines.size() ); ++i ) {
        const vehicle_part &vp = parts[engines[i]];
        const bool is_electric = is_engine_type( vp, fuel_type_battery );
        if( ( is_electric || is_engine_type_combustion( vp ) ) &&
            ( ( vp.is_available() && engine_fuel_left( here, vp ) ) || vp.enabled ) ) {
            c_engines.push_back( i );
            if( is_electric ) {
                has_electric_engine = true;
            }
        }
    }

    bool rotorcraft = is_flying && is_rotorcraft( here );


    if( rotorcraft || c_engines.empty() || ( has_electric_engine && c_engines.size() == 1 ) ||
        c_engines.size() > 5 ) {
        for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) ) {
            vp.part().enabled = false;
        }

        if( player_is_driving_this_veh( &here ) ) {
            if( rotorcraft ) {
                add_msg( _( "Smart controller does not support flying vehicles." ) );
            } else if( c_engines.empty() ) {

                add_msg( _( "Smart controller can not detect any controllable engine." ) );
            } else if( c_engines.size() == 1 ) {

                add_msg( _( "Smart controller detects only a single electric engine." ) );
                add_msg( _( "An electric engine does not need optimization." ) );
            } else {
                add_msg( _( "Smart controller does not support more than five engines." ) );
            }
            add_msg( m_bad, _( "Smart controller is shutting down." ) );
        }
        has_enabled_smart_controller = false;
        smart_controller_state = std::nullopt;
        return;
    }

    int cur_battery_level;
    int max_battery_level;
    std::tie( cur_battery_level, max_battery_level ) = battery_power_level( );
    int battery_level_percent = max_battery_level == 0 ? 0 : cur_battery_level * 100 /
                                max_battery_level;


    cfg.battery_hi = clamp( cfg.battery_hi, 0, 100 );
    cfg.battery_lo = clamp( cfg.battery_lo, 0, cfg.battery_hi );



    bool discharge_forbidden_soft = battery_level_percent <= cfg.battery_hi;
    bool discharge_forbidden_hard = battery_level_percent <= cfg.battery_lo;
    units::power target_charging_rate;
    if( max_battery_level == 0 || !discharge_forbidden_soft ) {
        target_charging_rate = 0_W;
    } else {
        target_charging_rate = units::from_watt( static_cast<std::int64_t>( ( max_battery_level *
                               cfg.battery_hi / 100 -
                               cur_battery_level ) * 10 / ( 6 * 3 ) ) );
    }




    int accel_demand = std::max( std::abs( cruise_velocity - velocity ),
                                 std::abs( cruise_velocity - avg_velocity ) );
    if( velocity != 0 && accel_demand == 0 ) {
        accel_demand = 1;
    }

    int velocity_demand = std::max( std::abs( this->velocity ), std::abs( cruise_velocity ) );


    bool is_stationary = avg_velocity == 0 && velocity_demand == 0 && accel_demand == 0;

    bool gas_engine_shutdown_forbidden = smart_controller_state &&
                                         ( calendar::turn - smart_controller_state->gas_engine_last_turned_on ) <
                                         15_seconds;

    smart_controller_cache cur_state;

    float traction = is_stationary ? 1.0f :
                     ( k_traction_cache ? *k_traction_cache : k_traction( here, here.vehicle_wheel_traction( *this ) ) );

    int prev_mask = 0;

    units::power opt_net_echarge_rate = net_battery_charge_rate( here, true );

    units::power opt_fuel_usage = 0_W;

    int opt_accel = is_stationary ? 1 : current_acceleration( here ) * traction;
    int opt_safe_vel = is_stationary ? 1 : safe_ground_velocity( here, true );
    float cur_load_approx = static_cast<float>( std::min( accel_demand,
                            opt_accel ) )  / std::max( opt_accel, 1 );
    float cur_load_alternator = std::min( 0.01f, static_cast<float>( alternator_load ) / 1000 );

    for( size_t i = 0; i < c_engines.size(); ++i ) {
        const vehicle_part &vp = parts[engines[c_engines[i]]];
        if( is_engine_on( vp ) ) {
            const bool is_electric = is_engine_type( vp, fuel_type_battery );
            prev_mask |= 1 << i;
            units::power fu = engine_fuel_usage( vp ) * ( cur_load_approx + ( is_electric ? 0 :
                              cur_load_alternator ) );
            opt_fuel_usage += fu;
            if( is_electric ) {
                opt_net_echarge_rate -= fu;
            }
        }
    }
    cur_state.created = calendar::turn;
    cur_state.battery_percent = battery_level_percent;
    cur_state.battery_net_charge_rate = opt_net_echarge_rate;
    cur_state.velocity = avg_velocity;
    cur_state.load = cur_load_approx + cur_load_alternator;
    if( smart_controller_state ) {
        cur_state.gas_engine_last_turned_on = smart_controller_state->gas_engine_last_turned_on;
    }
    cur_state.gas_engine_shutdown_forbidden = gas_engine_shutdown_forbidden;

    int opt_mask = prev_mask;


    if( smart_controller_state &&
        std::abs( smart_controller_state->velocity - cur_state.velocity ) < 100 &&
        std::abs( smart_controller_state->battery_percent - cur_state.battery_percent ) <= 2 &&
        std::abs( smart_controller_state->load - cur_state.load ) < 0.1 &&
        smart_controller_state->battery_net_charge_rate == cur_state.battery_net_charge_rate &&

        !( smart_controller_state->gas_engine_shutdown_forbidden && !gas_engine_shutdown_forbidden )
      ) {
        smart_controller_state->created = calendar::turn;
        return;
    }


    if( !has_electric_engine ) {
        if( !discharge_forbidden_soft && is_stationary && engine_on && !autopilot_on &&
            !player_is_driving_this_veh( &here ) ) {
            stop_engines( here );
            sfx::do_vehicle_engine_sfx();

        } else if( discharge_forbidden_hard && !engine_on && cur_battery_level > 0 ) {
            engine_on = true;
            sfx::do_vehicle_engine_sfx();
        }
    }


    for( int mask = 1; mask < static_cast<int>( 1 << c_engines.size() ); ++mask ) {
        if( mask == prev_mask ) {
            continue;
        }

        bool gas_engine_to_shut_down = false;
        for( size_t i = 0; i < c_engines.size(); ++i ) {
            vehicle_part &vp = parts[engines[c_engines[i]]];
            bool old_state = ( prev_mask & ( 1 << i ) ) != 0;
            bool new_state = ( mask & ( 1 << i ) ) != 0;

            vp.enabled = new_state;

            if( old_state && !new_state && !is_engine_type( vp, fuel_type_battery ) ) {
                gas_engine_to_shut_down = true;
            }
        }

        if( gas_engine_to_shut_down && gas_engine_shutdown_forbidden ) {
            continue;
        }

        int safe_vel =  is_stationary ? 1 : safe_ground_velocity( here, true );
        int accel = is_stationary ? 1 : current_acceleration( here ) * traction;
        units::power fuel_usage = 0_W;
        units::power net_echarge_rate = net_battery_charge_rate( here, true );
        float load_approx = static_cast<float>( std::min( accel_demand, accel ) ) / std::max( accel, 1 );
        update_alternator_load( here );
        float load_approx_alternator  = std::min( 0.01f, static_cast<float>( alternator_load ) / 1000 );

        for( int e : c_engines ) {
            const vehicle_part &vp = parts[engines[e]];
            const bool is_electric = is_engine_type( vp, fuel_type_battery );
            units::power fu = engine_fuel_usage( vp ) * ( load_approx + ( is_electric ? 0 :
                              load_approx_alternator ) );
            fuel_usage += fu;
            if( is_electric ) {
                net_echarge_rate -= fu;
            }
        }

        if( std::forward_as_tuple(
                !discharge_forbidden_hard || ( net_echarge_rate > 0_W ),
                accel >= accel_demand,
                opt_accel < accel_demand ? accel : 0,
                safe_vel >= velocity_demand,
                opt_safe_vel < velocity_demand ? -safe_vel : 0,
                !discharge_forbidden_soft || ( net_echarge_rate > target_charging_rate ),
                -fuel_usage,
                net_echarge_rate
            ) >= std::forward_as_tuple(
                !discharge_forbidden_hard || ( opt_net_echarge_rate > 0_W ),
                opt_accel >= accel_demand,
                opt_accel < accel_demand ? opt_accel : 0,
                opt_safe_vel >= velocity_demand,
                opt_safe_vel < velocity_demand ? -opt_safe_vel : 0,
                !discharge_forbidden_soft || ( opt_net_echarge_rate > target_charging_rate ),
                -opt_fuel_usage,
                opt_net_echarge_rate
            ) ) {
            opt_mask = mask;
            opt_fuel_usage = fuel_usage;
            opt_net_echarge_rate = net_echarge_rate;
            opt_accel = accel;
            opt_safe_vel = safe_vel;

            cur_state.battery_net_charge_rate = net_echarge_rate;
            cur_state.load = load_approx + load_approx_alternator;

        }
    }

    for( size_t i = 0; i < c_engines.size(); ++i ) {
        vehicle_part &vp = parts[engines[c_engines[i]]];
        vp.enabled = static_cast<bool>( prev_mask & ( 1 << i ) );
    }

    if( opt_mask != prev_mask ) {
        bool failed_to_start = false;
        bool turned_on_gas_engine = false;
        for( size_t i = 0; i < c_engines.size(); ++i ) {
            vehicle_part &vp = parts[engines[c_engines[i]]];

            if( ( prev_mask & ( 1 << i ) ) < ( opt_mask & ( 1 << i ) ) ) {
                if( !start_engine( here, vp, true ) ) {
                    failed_to_start = true;
                }
                turned_on_gas_engine |= !is_engine_type( vp, fuel_type_battery );
            }
        }
        if( failed_to_start ) {
            this->smart_controller_state = std::nullopt;

            for( size_t i = 0; i < c_engines.size(); ++i ) {
                vehicle_part &vp = parts[engines[c_engines[i]]];
                vp.enabled = static_cast<bool>( prev_mask & ( 1 << i ) );
            }
            for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) ) {
                vp.part().enabled = false;
            }
            if( player_is_driving_this_veh( &here ) ) {
                add_msg( m_bad, _( "Smart controller failed to start an engine." ) );
                add_msg( m_bad, _( "Smart controller is shutting down." ) );
            }
            has_enabled_smart_controller = false;

        } else {
            for( size_t i = 0; i < c_engines.size(); ++i ) {
                vehicle_part &vp = parts[engines[c_engines[i]]];

                if( ( prev_mask & ( 1 << i ) ) > ( opt_mask & ( 1 << i ) ) ) {
                    start_engine( here, vp, false );
                }
            }
            if( turned_on_gas_engine ) {
                cur_state.gas_engine_last_turned_on = calendar::turn;
            }
            smart_controller_state = cur_state;

            if( player_is_driving_this_veh( &here ) ) {
                add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Smart controller optimizes engine state." );
            }
        }
    } else {

        smart_controller_state = cur_state;
    }
    update_alternator_load( here );
}

void vehicle::thrust( map &here, int thd, int z )
{


    if( !is_moving() && z == 0 ) {
        turn_dir = face.dir();
        stop( here );
    }
    bool pl_ctrl = player_is_driving_this_veh( &here );


    if( ( is_watercraft() && can_float( here ) ) || ( is_rotorcraft( here ) && ( z != 0 ||
            is_flying ) ) ) {

    } else if( in_deep_water && !can_float( here ) ) {
        stop( here );
        if( pl_ctrl ) {
            add_msg( _( "The %s is too leaky!" ), name );
        }
        return;
    } else if( !valid_wheel_config( here )  && z == 0 ) {
        stop( here );
        if( pl_ctrl ) {
            add_msg( _( "The %s doesn't have enough wheels to move!" ), name );
        }
        return;
    }

    bool thrusting = true;
    if( velocity ) {
        int sgn = ( velocity < 0 ) ? -1 : 1;
        thrusting = ( sgn == thd );
    }


    float traction = k_traction( here, here.vehicle_wheel_traction( *this ) );


    if( !skidding && !wheelcache.empty() ) {
        int thick_ice_wheels = 0;
        for( const int wheel_idx : wheelcache ) {
            const vehicle_part &wp = part( wheel_idx );
            const tripoint_bub_ms pwp = bub_part_pos( here, wp );
            if( here.ter( pwp ).obj().has_flag( ter_furn_flag::TFLAG_THICK_ICE ) ) {
                thick_ice_wheels++;
            }
        }
        if( thick_ice_wheels > 0 ) {

            if( one_in( 4 ) || std::abs( velocity ) > 1000 ) {
                skidding = true;
            }
        }
    }

    if( thrusting ) {
        smart_controller_handle_turn( here, traction );
    }

    int accel = current_acceleration( here ) * traction;
    if( accel < 200 && velocity > 0 && is_towing() ) {
        if( pl_ctrl ) {
            add_msg( _( "The %s struggles to pull the %s on this surface!" ), name,
                     tow_data.get_towed()->name );
        }
        return;
    }
    if( thrusting && accel == 0 ) {
        if( pl_ctrl ) {
            if( has_engine_type( fuel_type_muscle, true ) ) {
                add_msg( _( "The %s is too heavy to move!" ), name );
            } else {
                add_msg( n_gettext( "The %s is too heavy for its engine!",
                                    "The %s is too heavy for its engines!",
                                    engines.size() ), name );
            }
        }
        return;
    }
    const int max_vel = traction * max_velocity( here );

    bool on_thick_ice = false;
    for( const int wheel_idx : wheelcache ) {
        const vehicle_part &wp = part( wheel_idx );
        const tripoint_bub_ms pwp = bub_part_pos( here, wp );
        if( here.ter( pwp ).obj().has_flag( ter_furn_flag::TFLAG_THICK_ICE ) ) {
            on_thick_ice = true;
            break;
        }
    }
    const float ICE_BRAKE_FACTOR = 0.25f;
    const int max_brake = static_cast<int>( 20 * 100 * ( on_thick_ice ? ICE_BRAKE_FACTOR : 1.0f ) );

    int vel_inc = ( accel + ( thrusting ? 0 : max_brake ) ) * thd;

    if( thd == -1 && thrusting && !has_engine_type( fuel_type_battery, true ) ) {
        vel_inc = .6 * vel_inc;
    }



    int load;
    if( accel != 0 ) {
        int effective_cruise;
        if( cruise_velocity > 0 ) {
            effective_cruise = std::min( cruise_velocity, max_vel );
        } else {
            effective_cruise = std::max( cruise_velocity, -max_vel );
        }
        if( thd > 0 ) {
            vel_inc = std::min( vel_inc, effective_cruise - velocity );
        } else {
            vel_inc = std::max( vel_inc, effective_cruise - velocity );
        }
        if( thrusting ) {
            load = 1000 * std::abs( vel_inc ) / accel;
        } else {


            load = 1000 * std::max( 0, std::abs( vel_inc ) - max_brake ) / accel;
        }
    } else {
        load = ( thrusting ? 1000 : 0 );
    }

    if( is_rotorcraft( here ) && ( z > 0 || is_flying_in_air() ) ) {
        load = std::max( load, z > 0 ? 300 : 150 );
        thrusting = true;
    }


    if( load >= 1 && thrusting ) {

        if( total_power( here ) <= 0_W || !engine_on || ( z == 0 && accel == 0 ) ) {
            if( pl_ctrl ) {
                if( total_power( here, false ) <= 0_W ) {
                    add_msg( m_info, _( "The %s doesn't have an engine!" ), name );
                } else if( has_engine_type( fuel_type_muscle, true ) ) {
                    add_msg( m_info, _( "The %s's mechanism is out of reach!" ), name );
                } else if( !engine_on ) {
                    add_msg( _( "The %s's engine isn't on!" ), name );
                } else if( traction < 0.01f ) {
                    add_msg( _( "The %s is stuck." ), name );
                } else {
                    add_msg( _( "The %s's engine emits a sneezing sound." ), name );
                }
            }
            cruise_velocity = 0;
            return;
        }



        if( is_rotorcraft( here ) && is_flying_in_air() ) {
            const int velocity_kt = velocity * 0.01;
            int value;
            if( velocity_kt < 70 ) {
                value = 49 * std::pow( velocity_kt, 3 ) -
                        4118 * std::pow( velocity_kt, 2 ) - 76512 * velocity_kt + 18458000;
            } else {
                value = 1864 * std::pow( velocity_kt, 2 ) - 272190 * velocity_kt + 19473000;
            }
            value *= 0.0001;
            load = std::max( 200, std::min( 1000, ( ( value / 2 ) + 100 ) ) );
        }

        noise_and_smoke( here, load + alternator_load );
        check_flats_do_rim_damage_or_sounds( here );
        consume_fuel( here, load + alternator_load, false );
        if( z != 0 && is_rotorcraft( here ) ) {
            requested_z_change = z;
        }

        const int strn = static_cast<int>( strain( here ) * strain( here ) * 100 );
        for( const int p : engines ) {
            do_engine_damage( here, parts[p], strn );
        }
    }



    if( skidding ) {
        return;
    }


    if( ( velocity > 0 && velocity + vel_inc < 0 ) || ( velocity < 0 && velocity + vel_inc > 0 ) ) {

        stop( here );
    } else {

        const int min_vel = max_reverse_velocity( here );
        if( vel_inc > 0 ) {

            velocity = std::max( velocity, std::min( velocity + vel_inc, max_vel ) );
        } else {
            velocity = std::min( velocity, std::max( velocity + vel_inc, min_vel ) );
        }
    }


    for( size_t e = 0; e < parts.size(); e++ ) {
        vehicle_part &vp = parts[e];
        if( vp.info().fuel_type == fuel_type_animal && engines.size() != 1 ) {
            monster *mon = get_monster( here, e );
            if( mon != nullptr && mon->has_effect( effect_harnessed ) ) {
                if( velocity > mon->get_speed() * 12 ) {
                    add_msg( m_bad, _( "Your %s is not fast enough to keep up with the %s" ), mon->get_name(), name );
                    int dmg = rng( 0, 10 );
                    damage_direct( here, vp, dmg );
                }
            }
        }
    }
}

void vehicle::cruise_thrust( map &here, int amount )
{
    if( amount == 0 ) {
        return;
    }
    int safe_vel = safe_velocity( here );
    int max_vel = autopilot_on ? safe_velocity( here ) : max_velocity( here );
    int max_rev_vel = max_reverse_velocity( here );


    if( ( cruise_velocity < safe_vel && safe_vel < ( cruise_velocity + amount ) ) ||
        ( cruise_velocity > safe_vel && safe_vel > ( cruise_velocity + amount ) ) ) {
        cruise_velocity = safe_vel;
    } else {
        if( amount < 0 && ( cruise_velocity == safe_vel || cruise_velocity == max_vel ) ) {


            cruise_velocity += -1;
        } else if( amount > 0 && cruise_velocity == max_rev_vel ) {

            cruise_velocity += 1;
        } else {

            cruise_velocity += amount;
        }



        cruise_velocity = ( cruise_velocity / std::abs( amount ) ) * std::abs( amount );
    }


    if( cruise_velocity > max_vel ) {
        cruise_velocity = max_vel;
    } else if( cruise_velocity < max_rev_vel ) {
        cruise_velocity = max_rev_vel;
    }
}

void vehicle::turn( units::angle deg )
{
    if( deg == 0_degrees ) {
        return;
    }
    if( velocity < 0 && !::get_option<bool>( "REVERSE_STEERING" ) ) {
        deg = -deg;
    }
    last_turn = deg;
    turn_dir = normalize( turn_dir + deg );

    turn_dir = round_to_multiple_of( turn_dir, vehicles::steer_increment );
}

bool vehicle::a11y_rotate_in_place( map &here, const int steering_steps,
                                    const bool preview_only )
{



    if( steering_steps == 0 ) {
        return true;
    }


    const auto record_block = [&]( const char *kind, const tripoint_bub_ms & tile,
    const int intermediate, const units::angle heading ) {
        if( !a11y::android::enabled() ) {
            return;
        }
        const std::string terrain_name = here.inbounds( tile ) ?
                                         here.name( tile ) : "outside_loaded_map";


        std::string obstacle_name = terrain_name;
        if( here.inbounds( tile ) ) {
            if( std::string( kind ) == "visible_trap" ) {
                obstacle_name = here.tr_at( tile ).name();
            } else if( std::string( kind ) == "furniture" ) {
                obstacle_name = here.furnname( tile );
            } else if( std::string( kind ) == "creature" ) {
                const Creature *creature = get_creature_tracker().creature_at( tile, true );
                if( creature ) {
                    obstacle_name = creature->disp_name();
                }
            } else if( std::string( kind ) == "other_vehicle" ) {
                const optional_vpart_position other = here.veh_at( tile );
                if( other ) {
                    obstacle_name = other->vehicle().name;
                }
            } else if( std::string( kind ) == "terrain" ) {
                obstacle_name = here.tername( tile );
            }
        }
        a11y::android::diagnostic_event( "AUTODRIVE_PIVOT_BLOCK",
                                         "preview=" + std::string( preview_only ? "true" : "false" ) +
                                         " turn_steps=" + std::to_string( steering_steps ) +
                                         " intermediate=" + std::to_string( intermediate ) +
                                         " heading_deg=" + std::to_string( units::to_degrees( heading ) ) +
                                         " tile=" + tile.to_string() +
                                         " kind=" + kind +
                                         " terrain_name=" + terrain_name +
                                         " blocker_name=" + obstacle_name );
    };
    const auto blocker_kind = []( const vehicle_autodrive::tile_blocker kind ) {
        switch( kind ) {
            case vehicle_autodrive::tile_blocker::map_boundary:
                return "map_boundary";
            case vehicle_autodrive::tile_blocker::other_vehicle:
                return "other_vehicle";
            case vehicle_autodrive::tile_blocker::creature:
                return "creature";
            case vehicle_autodrive::tile_blocker::visible_trap:
                return "visible_trap";
            case vehicle_autodrive::tile_blocker::furniture:
                return "furniture";
            case vehicle_autodrive::tile_blocker::terrain:
                return "terrain";
            case vehicle_autodrive::tile_blocker::none:
                return "none";
        }
        return "unknown";
    };
    if( velocity != 0 ) {
        record_block( "vehicle_moving", pos_bub( here ), 0, face.dir() );
        if( !preview_only ) {
            add_msg( m_warning, _( "Cannot turn in place while moving. Brake to a stop first." ) );
        }
        return false;
    }
    if( skidding || is_on_ramp || is_rotorcraft( here ) || is_towed() || is_towing() ) {
        record_block( "vehicle_state", pos_bub( here ), 0, face.dir() );
        if( !preview_only ) {
            add_msg( m_warning, _( "Cannot turn in place in the current vehicle state." ) );
        }
        return false;
    }
    const float steering = steering_effectiveness( here );
    if( steering == -2.0f ) {
        record_block( "no_animal_harness", pos_bub( here ), 0, face.dir() );
        if( !preview_only ) {
            add_msg( m_warning,
                     _( "You cannot steer an animal-drawn vehicle with no animal harnessed." ) );
        }
        return false;
    }
    if( steering < 0.0f ) {
        record_block( "no_steering", pos_bub( here ), 0, face.dir() );
        if( !preview_only ) {
            add_msg( m_warning,
                     _( "This vehicle has no steering system installed, you can't turn it." ) );
        }
        return false;
    }
    if( steering == 0.0f || !valid_wheel_config( here ) ) {
        record_block( "broken_steering_or_wheels", pos_bub( here ), 0, face.dir() );
        if( !preview_only ) {
            add_msg( m_warning, _( "Cannot turn in place: steering or wheels are broken." ) );
        }
        return false;
    }

    const point_rel_ms pivot = pivot_point( here );
    const tripoint_bub_ms origin = pos_bub( here );


    const vehicle_autodrive::mobility_profile mobility = {
        valid_wheel_config( here ), can_float( here ), has_sufficient_rotorlift( here ), true
    };
    const int sign = steering_steps > 0 ? 1 : -1;
    const int count = std::min( std::abs( steering_steps ), 12 );



    for( int step = 1; step <= count; ++step ) {
        const units::angle heading =
            normalize( face.dir() + vehicles::steer_increment * sign * step );
        for( int index = 0; index < part_count(); ++index ) {
            const vehicle_part &part = parts[index];
            if( part.removed || part.is_fake ||
                part.info().location != vpart_location_structure ) {
                continue;
            }
            tripoint_rel_ms offset;
            coord_translate( heading, pivot, part.mount, offset );
            offset.z() = part.precalc[0].z();
            const tripoint_bub_ms tile = origin + offset;
            if( !here.inbounds( tile ) ) {
                record_block( "map_boundary", tile, step, heading );
                if( !preview_only ) {
                    add_msg( m_warning, _( "Cannot turn in place: unloaded map boundary." ) );
                }
                return false;
            }
            const optional_vpart_position occupying = here.veh_at( tile );
            if( occupying && &occupying->vehicle() == this ) {
                continue;
            }
            if( occupying ) {
                record_block( "other_vehicle", tile, step, heading );
                if( !preview_only ) {
                    add_msg( m_warning, _( "Cannot turn in place: another vehicle is in the way." ) );
                }
                return false;
            }


            const vehicle_autodrive::tile_blocker obstacle =
                vehicle_autodrive::classify_tile( *this, here, tile, get_avatar(), mobility );
            if( obstacle != vehicle_autodrive::tile_blocker::none ) {
                record_block( blocker_kind( obstacle ), tile, step, heading );
                if( !preview_only ) {
                    const std::string blocker_name =
                        obstacle == vehicle_autodrive::tile_blocker::visible_trap ?
                        here.tr_at( tile ).name() :
                        obstacle == vehicle_autodrive::tile_blocker::furniture ?
                        here.furnname( tile ) :
                        obstacle == vehicle_autodrive::tile_blocker::terrain ?
                        here.tername( tile ) : here.name( tile );
                    add_msg( m_warning, _( "Cannot turn in place: %s is in the way." ),
                             blocker_name );
                }
                return false;
            }
            const Creature *creature = get_creature_tracker().creature_at( tile, true );
            if( creature && creature != &get_avatar() ) {
                record_block( "creature", tile, step, heading );
                if( !preview_only ) {
                    add_msg( m_warning, _( "Cannot turn in place: %s is in the way." ),
                             creature->disp_name() );
                }
                return false;
            }
        }
    }



    if( preview_only ) {
        return true;
    }
    const units::angle target = normalize(
                                    face.dir() + vehicles::steer_increment * sign * count );
    precalc_mounts( 1, target, pivot );



    if( !here.displace_vehicle( *this, tripoint_rel_ms::zero ) ) {
        precalc_mounts( 1, face.dir(), pivot );
        add_msg( m_warning, _( "Cannot turn in place: vehicle position could not be updated." ) );
        return false;
    }
    face.init( target );
    move.init( target );
    turn_dir = target;
    last_turn = 0_degrees;
    a11y_obstacle_override = false;
    a11y_override_obstacle_name.clear();
    a11y_obstacle_stop = false;
    a11y_brake_alerted = false;
    return true;
}

void vehicle::stop( map &here )
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0_degrees;
    of_turn_carry = 0;
    for( const tripoint_abs_ms &p : get_points() ) {
        if( here.inbounds( p ) ) {
            here.memory_cache_dec_set_dirty( here.get_bub( p ), true );
        }
    }
}

bool vehicle::collision( map &here, std::vector<veh_collision> &colls,
                         const tripoint_rel_ms &dp,
                         bool just_detect, bool bash_floor )
{














    if( dp.z() != 0 && ( dp.x() != 0 || dp.y() != 0 ) ) {

        return collision( here, colls, tripoint_rel_ms( dp.xy(), 0 ), just_detect, bash_floor ) ||
               collision( here, colls, tripoint_rel_ms( 0,    0,    dp.z() ), just_detect, bash_floor );
    }

    if( dp.z() == -1 && !bash_floor ) {


        if( collision( here, colls, tripoint_rel_ms::zero, just_detect, true ) ) {
            return true;
        }
    }

    const bool vertical = bash_floor || dp.z() != 0;
    const int &coll_velocity = vertical ? vertical_velocity : velocity;

    if( coll_velocity == 0 && !is_rotorcraft( here ) ) {
        just_detect = true;
    }

    const int velocity_before = coll_velocity;
    int lowest_velocity = coll_velocity;
    const int sign_before = sgn( velocity_before );
    bool empty = true;
    part_project_points( dp );
    for( int p = 0; p < part_count(); p++ ) {
        const vehicle_part &vp = parts.at( p );
        if( vp.removed || !vp.is_real_or_active_fake() ) {
            continue;
        }

        const vpart_info &info = vp.info();
        if( !vp.is_fake && info.location != vpart_location_structure && !info.has_flag( VPFLAG_ROTOR ) ) {
            continue;
        }
        empty = false;


        const tripoint_abs_ms dsp = vp.next_pos;
        veh_collision coll = part_collision( here, p, dsp, just_detect, bash_floor );
        if( coll.type == veh_coll_nothing && info.has_flag( VPFLAG_ROTOR ) ) {
            size_t radius = static_cast<size_t>( std::round( info.rotor_info->rotor_diameter / 2.0f ) );
            for( const tripoint_bub_ms &rotor_point : here.points_in_radius( here.get_bub( dsp ), radius ) ) {
                veh_collision rotor_coll = part_collision( here, p, here.get_abs( rotor_point ), just_detect,
                                           false );
                if( rotor_coll.type != veh_coll_nothing ) {
                    coll = rotor_coll;
                    if( just_detect ) {
                        break;
                    } else {
                        colls.push_back( rotor_coll );
                    }
                }
            }
        }
        if( coll.type == veh_coll_nothing ) {
            continue;
        }

        colls.push_back( coll );

        if( just_detect ) {

            return true;
        }

        const int velocity_after = coll_velocity;


        if( vertical ) {
            if( velocity_before < 0 ) {
                lowest_velocity = std::max( lowest_velocity, coll_velocity );
            } else {
                lowest_velocity = std::min( lowest_velocity, coll_velocity );
            }
            vertical_velocity = velocity_before;
        } else if( sgn( velocity_after ) != sign_before ) {

            break;
        }
    }

    if( vertical ) {
        vertical_velocity = lowest_velocity;
        if( vertical_velocity == 0 ) {
            is_falling = false;
        }
    }

    if( empty ) {

        veh_collision fake_coll;
        fake_coll.type = veh_coll_other;
        colls.push_back( fake_coll );
        velocity = 0;
        vertical_velocity = 0;
        add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Collision check on a dirty vehicle %s", name );
        return true;
    }

    return !colls.empty();
}


static void terrain_collision_data( map &here, const tripoint_bub_ms &p, bool bash_floor,
                                    float &mass, float &density, float &elastic )
{
    elastic = 0.30;

    const int bash_min = here.bash_resistance( p, bash_floor );
    const int bash_max = here.bash_strength( p, bash_floor );
    mass = ( bash_min + bash_max ) / 2.0;
    density = bash_min;
}

veh_collision vehicle::part_collision( map &here, int part, const tripoint_abs_ms &p,
                                       bool just_detect, bool bash_floor )
{
    tripoint_bub_ms pos = here.get_bub( p );


    const bool vert_coll = bash_floor || p.z() != sm_pos.z();
    Creature *critter = get_creature_tracker().creature_at( p, true );
    Character *ph = dynamic_cast<Character *>( critter );

    Character *driver = get_driver( here );


    if( ph != nullptr && ph->in_vehicle ) {
        critter = nullptr;
        ph = nullptr;
    }

    const optional_vpart_position ovp = here.veh_at( p );


    const bool is_veh_collision = !bash_floor && ovp && &ovp->vehicle() != this;
    const bool is_body_collision = !bash_floor && critter != nullptr;

    veh_collision ret;
    ret.type = veh_coll_nothing;
    ret.part = part;



    if( is_veh_collision ) {
        ret.type = veh_coll_veh;

        ret.target = &ovp->vehicle();
        ret.target_part = ovp->part_index();
        ret.target_name = ovp->vehicle().disp_name();
        return ret;
    }


    int rotor_velocity = 45600;

    int &coll_velocity = ( parts[part].info().has_flag( VPFLAG_ROTOR ) == 0 ) ?
                         ( vert_coll ? vertical_velocity : velocity ) :
                         rotor_velocity;
    if( !just_detect && coll_velocity == 0 ) {
        return ret;
    }

    if( is_body_collision ) {

        if( ovp && ( &ovp->vehicle() == this ) && get_monster( here, ovp->part_index() ) ) {
            return ret;
        }

        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, critter->pos_bub() ) ) {
            tripoint_abs_ms end_pos = critter->pos_abs();
            tripoint_abs_ms start_pos;
            const std::set<tripoint_abs_ms> projected_points = get_projected_part_points();
            const units::angle angle =
                move.dir() + ( coll_velocity > 0 ? 0_degrees : 180_degrees ) + 45_degrees *
                ( parts[part].mount.x() > pivot_point( here ).x() ? -1 : 1 );
            const std::set<tripoint_abs_ms> &cur_points = get_points( true );


            while( get_creature_tracker().creature_at( end_pos, true ) ||
                   cur_points.find( end_pos ) != cur_points.end() ||
                   projected_points.find( end_pos ) != projected_points.end() ) {
                start_pos = end_pos;
                calc_ray_end( angle, 2, start_pos, end_pos );
            }
            critter->setpos( end_pos );
            return ret;
        }
    }



    const int armor_part = part_with_feature( ret.part, VPFLAG_ARMOR, true );
    if( armor_part >= 0 ) {
        ret.part = armor_part;
    }
    vehicle_part &vp = this->part( ret.part );
    const vpart_info &vpi = vp.info();

    float mass2 = 0.0f;

    float e = 0.3f;


    float part_dens = 0.0f;

    std::optional<field_entry> potential_impassable_field = here.get_impassable_field_at( pos );
    if( potential_impassable_field.has_value() ) {

        ret.type = veh_coll_other;
        mass2 = 1000;
        e = 0.10f;
        part_dens = 80;
        ret.target_name = potential_impassable_field.value().name();
    } else if( is_body_collision ) {


        ret.type = veh_coll_body;
        ret.target = critter;
        e = 0.30f;
        part_dens = 15;
        mass2 = units::to_kilogram( critter->get_weight() );
        ret.target_name = critter->disp_name();
    } else if( ( bash_floor && here.is_bashable_ter_furn( pos, true ) ) ||
               ( here.is_bashable_ter_furn( pos, false ) && here.move_cost_ter_furn( pos ) != 2 &&

                 ( part_with_feature( ret.part, VPFLAG_WHEEL, true ) >= 0 ||
                   !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_TINY, pos ) ) &&


                 !( part_with_feature( vp.mount, "PROTRUSION", true ) >= 0 &&
                    here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SHORT, pos ) ) &&

                 !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NOCOLLIDE, pos ) &&

                 !( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_RAIL, pos ) &&
                    this->can_use_rails( here ) ) ) ) {

        ret.type = veh_coll_bashable;
        terrain_collision_data( here, pos, bash_floor, mass2, part_dens, e );
        ret.target_name = here.disp_name( pos );
    } else if( here.impassable_ter_furn( pos ) ||
               ( bash_floor && !here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, pos ) ) ) {

        ret.type = veh_coll_other;
        mass2 = 1000;
        e = 0.10f;
        part_dens = 80;
        ret.target_name = here.disp_name( pos );
    }

    if( ret.type == veh_coll_nothing || just_detect ) {

        return ret;
    }
    if( is_autodriving && a11y::android::enabled() &&
        get_driver( here ) == &get_player_character() ) {
        a11y::android::diagnostic_event( "AUTODRIVE_STOP",
                                         "reason=physical_collision velocity=" + std::to_string( velocity ) );
    }
    stop_autodriving();



    const float mass = vpi.has_flag( VPFLAG_ROTOR )
                       ? to_kilogram( vp.base.weight() )
                       : to_kilogram( total_mass( here ) );


    const itype *type = item::find_type( vpi.base_item );
    const auto &mats = type->materials;
    float mat_total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    float vpart_dens = 0.0f;
    if( !mats.empty() ) {
        for( const std::pair<const material_id, int> &mat_id : mats ) {
            vpart_dens += mat_id.first->density() * ( static_cast<float>( mat_id.second ) / mat_total );
        }

        vpart_dens /= mats.size();
    }



    float material_factor = ( part_dens - vpart_dens ) * 0.5f;
    material_factor = std::max( -25.0f, std::min( 25.0f, material_factor ) );


    const float weight_factor = mass >= mass2 ?
                                -25 * ( std::log( mass ) - std::log( mass2 ) ) / std::log( mass ) :
                                25 * ( std::log( mass2 ) - std::log( mass ) ) / std::log( mass2 );

    float k = 50 + material_factor + weight_factor;
    k = std::max( 10.0f, std::min( 90.0f, k ) );

    bool smashed = true;
    const std::string snd = _( "smash!" );
    float dmg = 0.0f;
    float part_dmg = 0.0f;

    time_duration time_stunned = 0_turns;

    const int prev_velocity = coll_velocity;
    const int vel_sign = sgn( coll_velocity );



    float vel2 = 0.0f;
    do {
        smashed = false;

        const float vel1 = coll_velocity / 100.0f;

        const float vel1_a = ( mass * vel1 + mass2 * vel2 + e * mass2 * ( vel2 - vel1 ) ) /
                             ( mass + mass2 );

        const float vel2_a = ( mass * vel1 + mass2 * vel2 + e * mass * ( vel1 - vel2 ) ) / ( mass + mass2 );

        const float E_before = 0.5f * ( mass * vel1 * vel1 )     + 0.5f * ( mass2 * vel2 * vel2 );
        const float E_after  = 0.5f * ( mass * vel1_a * vel1_a ) + 0.5f * ( mass2 * vel2_a * vel2_a );
        const float d_E = E_before - E_after;
        if( d_E <= 0 ) {



            if( std::fabs( vel1_a ) < std::fabs( vel1 ) ) {

                coll_velocity = vel1_a * 90;
            }
            if( std::fabs( vel2_a ) > std::fabs( vel2 ) ) {
                vel2 = vel2_a;
            }

            if( mass2 == 0 ) {
                mass2 = 1;
            }
            continue;
        }

        add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Deformation energy: %.2f", d_E );


        dmg += d_E / 400;


        if( critter == nullptr || !critter->is_hallucination() ) {
            part_dmg = dmg * k / 100.0f;
            add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Part collision damage: %.2f", part_dmg );
        }

        const float obj_dmg = dmg * ( 100.0f - k ) / 100.0f;

        if( ret.type == veh_coll_bashable ) {



            smashed = here.is_bashable_ter_furn( pos, false ) &&
                      here.bash_resistance( pos, bash_floor ) <= obj_dmg &&
                      here.bash( pos, obj_dmg, false, false, false, this ).success;
            if( smashed ) {
                if( here.is_bashable_ter_furn( pos, bash_floor ) ) {

                    smashed = false;
                    terrain_collision_data( here, pos, bash_floor, mass2, part_dens, e );
                    ret.target_name = here.disp_name( pos );
                } else if( here.impassable_ter_furn( pos ) ) {

                    smashed = false;
                    ret.type = veh_coll_other;
                    mass2 = 1000;
                    e = 0.10f;
                    part_dens = 80;
                    ret.target_name = here.disp_name( pos );
                }
            }
        } else if( ret.type == veh_coll_body ) {
            int dam = obj_dmg * vpi.dmg_mod / 100;



            cata_assert( critter );


            if( !critter->is_hallucination() ) {
                if( vpi.has_flag( "SHARP" ) ) {
                    vp.blood += 100 + 5 * dam;
                } else if( dam > rng( 10, 30 ) ) {
                    vp.blood += 50 + dam * 5 / 2;
                }

                check_environmental_effects = true;
            }

            time_stunned = time_duration::from_turns( ( rng( 0, dam ) > 10 ) + ( rng( 0, dam ) > 40 ) );
            if( time_stunned > 0_turns ) {
                critter->add_effect( effect_stunned, time_stunned );
            }

            if( ph != nullptr ) {
                ph->hitall( dam, 40, driver );
            } else {
                const int armor = vpi.has_flag( "SHARP" ) ?
                                  critter->get_armor_type( damage_cut, bodypart_id( "torso" ) ) :
                                  critter->get_armor_type( damage_bash, bodypart_id( "torso" ) );
                dam = std::max( 0, dam - armor );
                critter->apply_damage( driver, bodypart_id( "torso" ), dam );
                if( !critter->has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
                    if( vpi.has_flag( "SHARP" ) ) {
                        critter->add_effect( effect_source( driver ), effect_bleed, 1_minutes * rng( 1, dam ),
                                             critter->get_random_body_part_of_type( bp_type::torso ) );
                    } else if( dam > 18 && rng( 1, 20 ) > 15 ) {

                        critter->add_effect( effect_source( driver ), effect_bleed, 1_minutes,
                                             critter->get_random_body_part_of_type( bp_type::torso ) );
                    }
                }
                add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Critter collision damage: %d", dam );
            }


            if( !vert_coll ) {
                if( std::fabs( vel2_a ) > 10.0f ||
                    std::fabs( e * mass * vel1_a ) > std::fabs( mass2 * ( 10.0f - vel2_a ) ) ) {
                    const units::angle angle = rng_float( -60_degrees, 60_degrees );


                    const float push_force = std::max<float>( std::fabs( vel2_a ), 10.1f );


                    const units::angle angle_sum =
                        angle + move.dir() + ( vel2_a > 0 ? 0_degrees : 180_degrees );
                    g->fling_creature( critter, angle_sum, push_force );
                } else if( std::fabs( vel2_a ) > std::fabs( vel2 ) ) {
                    vel2 = vel2_a;
                } else {

                    velocity = 0;
                    break;
                }

                if( critter->is_dead_state() ) {
                    smashed = true;
                } else if( critter != nullptr ) {

                    smashed = critter->pos_abs() != p;
                }
            }
        }

        if( critter == nullptr || !critter->is_hallucination() ) {
            coll_velocity = vel1_a * 100;
        }

    } while( !smashed && sgn( coll_velocity ) == vel_sign );


    if( critter != nullptr ) {
        if( !critter->is_hallucination() ) {
            if( player_is_driving_this_veh( &here ) ) {
                if( time_stunned > 0_turns ) {
                    //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                    add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s and stuns it!" ),
                             name, vp.name(), ret.target_name );
                } else {
                    //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                    add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s!" ),
                             name, vp.name(), ret.target_name );
                }
            }

            if( vpi.has_flag( "SHARP" ) ) {
                critter->bleed( here );
            } else {
                sounds::sound( pos, 20, sounds::sound_t::combat, snd, false, "smash_success", "hit_vehicle" );
            }
        }
    } else {
        if( player_is_driving_this_veh( &here ) ) {
            if( !snd.empty() ) {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name, 4$s - sound message
                add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s with a %4$s!" ),
                         name, vp.name(), ret.target_name, snd );
            } else {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name
                add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s!" ),
                         name, vp.name(), ret.target_name );
            }
        }

        sounds::sound( pos, smashed ? 80 : 50, sounds::sound_t::combat, snd, false, "smash_success",
                       "hit_vehicle" );
    }

    if( smashed && !vert_coll ) {
        int turn_amount = rng( 1, 3 ) * std::sqrt( static_cast<double>( part_dmg ) );
        turn_amount /= 15;
        if( turn_amount < 1 ) {
            turn_amount = 1;
        }
        turn_amount *= 15;
        if( turn_amount > 120 ) {
            turn_amount = 120;
        }
        int turn_roll = rng( 0, 100 );

        if( turn_roll < std::abs( ( prev_velocity - coll_velocity ) / 100.0f * 2.0f ) ) {



            skidding = true;
            turn( units::from_degrees( one_in( 2 ) ? turn_amount : -turn_amount ) );
        }
    }

    ret.imp = part_dmg;
    return ret;
}


static std::pair<double, double> item_hardness_calc( const item &it )
{

    const double min_value_clamp = 0.001;
    const double max_value_clamp = 1000.0;

    double min_hardness = max_value_clamp;
    double max_hardness = min_value_clamp;
    const std::map<material_id, int> &mats = it.made_of();
    if( mats.empty() ) {

        return std::make_pair( min_value_clamp, max_value_clamp );
    }
    for( const std::pair<const material_id, int> &mat_pair : mats ) {
        const double chip_resist = mat_pair.first->chip_resist();
        min_hardness = std::min( min_hardness, chip_resist );
        max_hardness = std::max( max_hardness, chip_resist );
    }
    min_hardness = std::clamp( min_hardness, min_value_clamp, max_value_clamp );
    max_hardness = std::clamp( max_hardness, min_value_clamp, max_value_clamp );
    return std::make_pair( min_hardness, max_hardness );
}

double vehicle::hit_probability( const item &it, const vehicle_part *vp_wheel )
{


    const double item_coverage = to_millimeter( it.length() ) / 1000.0;

    const double wheel_coverage = static_cast<double>( vp_wheel->contact_area() ) / 100000.0;
    return std::min( wheel_coverage + item_coverage, 1.0 );
}

double vehicle::wheel_damage_chance_vs_item( const item &it, const vehicle_part &vp_wheel ) const
{

    double wheel_hardness = item_hardness_calc( vp_wheel.get_base() ).first;
    if( vp_wheel.info().has_flag( "RESIST_RUNOVER_DAMAGE" ) ) {

        wheel_hardness = wheel_hardness * 2.0;
    }

    const double item_hardness = item_hardness_calc( it ).second;

    const double chance_to_damage = std::min( std::pow( item_hardness / wheel_hardness, 2.0 ), 1.0 );
    add_msg_debug( debugmode::DF_VEHICLE_MOVE,
                   "Vehicle %s running over item %s."
                   "\n Chance to damage: %f%%."
                   "\n Item hardness: %f"
                   "\n Wheel hardness: %f",
                   disp_name(), it.tname(), chance_to_damage * 100.0, item_hardness,
                   wheel_hardness );
    return chance_to_damage;
}

static void apply_tire_punctures( vehicle_part *vp_wheel, std::vector<std::string> *messages )
{
    if( vp_wheel->has_fault( fault_flat_tire_riding_on_rims ) ) {
        return;
    }

    if( !vp_wheel->has_fault( fault_punctured_tires ) ) {
        if( vp_wheel->fault_set( fault_punctured_tires ) ) {
            messages->emplace_back( string_format(
                                        _( "You hear a loud pop from below, and your vehicle suddenly start to wobble like crazy!" ) ) );
            return;
        }
    } else {

        if( vp_wheel->fault_set( fault_flat_tire_riding_on_rims ) ) {
            messages->emplace_back( string_format(
                                        _( "With a jolt, hitting something has blown out your tire!" ) ) );
            return;
        }
    }
}

static void apply_generic_wheel_fault( vehicle_part *vp_wheel, std::vector<std::string> *messages )
{
    if( vp_wheel->has_fault( fault_axle_broken ) ) {
        return;
    }




    if( !vp_wheel->has_fault( fault_cracked_rim ) && x_in_y( 90, 100 ) ) {
        if( vp_wheel->fault_set( fault_cracked_rim ) ) {
            messages->emplace_back( string_format(
                                        _( "You hear a crunch as you run something over!" ) ) );
            return;
        }
    } else {
        if( vp_wheel->fault_set( fault_axle_broken ) ) {
            messages->emplace_back( string_format(
                                        _( "Your vehicle lurches as one of the wheels gives out!" ) ) );
            return;
        }
    }
}

static void apply_wheel_faults( vehicle_part *vp_wheel, std::vector<std::string> *messages )
{
    if( vp_wheel->faults_potential().count( fault_punctured_tires ) ) {
        apply_tire_punctures( vp_wheel, messages );
    } else if( vp_wheel->faults_potential().count( fault_cracked_rim ) ) {
        apply_generic_wheel_fault( vp_wheel, messages );
    }
}

void vehicle::damage_wheel_on_item( vehicle_part *vp_wheel, const item &it,
                                    std::vector<std::string> *messages ) const
{
    if( !it.has_flag( json_flag_PUNCTURE_VEHICLE_WHEELS ) ) {
        return;
    }

    const double chance_to_damage = wheel_damage_chance_vs_item( it, *vp_wheel );

    if( chance_to_damage > 0.0 && chance_to_damage >= rng_float( 0.0, 1.0 ) ) {
        apply_wheel_faults( vp_wheel, messages );
        refresh_pivot( get_map() );
        return;
    }
}

void vehicle::handle_trap( map *here, const tripoint_bub_ms &p, vehicle_part &vp_wheel )
{
    if( !vp_wheel.info().has_flag( VPFLAG_WHEEL ) ) {
        debugmsg( "vehicle::handle_trap called on non-WHEEL part" );
        return;
    }
    const trap &tr = here->tr_at( p );

    if( tr.is_null() ) {

        return;
    }
    const vehicle_handle_trap_data &veh_data = tr.vehicle_data;

    if( veh_data.is_falling ) {
        return;
    }

    Character &player_character = get_player_character();
    const Character *driver = get_driver( *here );
    const bool seen = player_character.sees( *here, p );
    const bool known = tr.can_see( p, player_character );
    const bool damage_done = vp_wheel.info().durability <= veh_data.damage;
    if( seen && damage_done ) {
        if( known ) {
            //~ %1$s: name of the vehicle; %2$s: name of the related vehicle part; %3$s: trap name
            add_msg( m_bad, _( "The %1$s's %2$s runs over %3$s." ), name, vp_wheel.name(), tr.name() );
        } else {
            add_msg( m_bad, _( "The %1$s's %2$s runs over something." ), name, vp_wheel.name() );
        }
    }

    if( veh_data.chance >= rng( 1, 100 ) ) {
        if( veh_data.sound_volume > 0 ) {
            sounds::sound( p, veh_data.sound_volume, sounds::sound_t::combat, veh_data.sound, false,
                           veh_data.sound_type, veh_data.sound_variant );
        }
        if( veh_data.do_explosion ) {
            explosion_handler::explosion( driver, p, veh_data.damage, 0.5f, false, veh_data.shrapnel );

        } else if( damage_done ) {

            damage_direct( *here, vp_wheel, veh_data.damage );
        }
        bool still_has_trap = true;
        if( veh_data.remove_trap || veh_data.do_explosion ) {
            here->remove_trap( p );
            still_has_trap = false;
        }
        for( const auto &it : veh_data.spawn_items ) {
            int cnt = roll_remainder( it.second );
            if( cnt > 0 ) {
                here->spawn_item( p, it.first, cnt );
            }
        }
        if( veh_data.set_trap ) {
            here->trap_set( p, veh_data.set_trap.id() );
            still_has_trap = true;
        }
        if( still_has_trap && player_is_driving_this_veh( here ) ) {
            const trap &tr = here->tr_at( p );
            if( seen || known ) {

                player_character.add_known_trap( p, tr );
            }
            if( seen && !known ) {

                const std::string direction = direction_name( direction_from( player_character.pos_bub(), p ) );
                if( tr.id != tr_thick_ice ) {
                    add_msg( _( "You've spotted a %1$s to the %2$s!" ), tr.name(), direction );
                }
            }
        }
    }
}

monster *vehicle::get_harnessed_animal( const map &here ) const
{
    for( size_t e = 0; e < parts.size(); e++ ) {
        const vehicle_part &vp = parts[ e ];
        if( vp.info().fuel_type == fuel_type_animal ) {
            monster *mon = get_monster( here, e );
            if( mon && mon->has_effect( effect_harnessed ) && mon->has_effect( effect_pet ) ) {
                return mon;
            }
        }
    }
    return nullptr;
}

void vehicle::selfdrive( map &here, const int trn, const int acceleration )
{
    if( !is_towed() && !magic && !get_harnessed_animal( here ) && !has_part( "AUTOPILOT" ) ) {
        is_following = false;
        return;
    }
    if( trn != 0 ) {
        if( steering_effectiveness( here ) <= 0 ) {
            return;
        }
        turn( trn * vehicles::steer_increment );
    }
    if( acceleration != 0 ) {
        if( !is_towed() ) {
            const int thr_amount = std::abs( velocity ) < 2000 ? 400 : 500;
            cruise_thrust( here, -acceleration * thr_amount );
        } else {
            thrust( here,  -acceleration );
        }
    }

    if( skidding && valid_wheel_config( here ) ) {
        const float handling_diff = handling_difficulty( here );
        if( handling_diff * rng( 1, 10 ) < 15 ) {
            velocity = static_cast<int>( forward_velocity() );
            skidding = false;
            move.init( turn_dir );
        }
    }
}

bool vehicle::check_is_heli_landed( map &here )
{
    if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, pos_bub( here ) ) ) {
        is_flying = false;
        return true;
    }
    return false;
}

bool vehicle::check_heli_descend( map &here, Character &p ) const
{
    if( !is_rotorcraft( here ) ) {
        debugmsg( "A vehicle is somehow flying without being an aircraft" );
        return true;
    }
    int count = 0;
    int air_count = 0;
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_abs_ms &pt : get_points( true ) ) {
        const tripoint_bub_ms pos = here.get_bub( pt );
        tripoint_bub_ms below( pos + tripoint::below );
        if( pt.z() < -OVERMAP_DEPTH || !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, pos ) ) {
            p.add_msg_if_player( _( "You are already landed!" ) );
            return false;
        }
        const optional_vpart_position ovp = here.veh_at( below );
        if( here.impassable_ter_furn( below ) || ovp || creatures.creature_at( below ) ) {
            p.add_msg_if_player( m_bad,
                                 _( "It would be unsafe to try and land when there are obstacles below you." ) );
            return false;
        }
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, below ) ) {
            air_count++;
        }
        count++;
    }
    if( velocity > 0 && air_count != count ) {
        p.add_msg_if_player( m_bad, _( "It would be unsafe to try and land while you are moving." ) );
        return false;
    }
    return true;

}

bool vehicle::check_heli_ascend( map &here, Character &p ) const
{
    if( !is_rotorcraft( here ) ) {
        debugmsg( "A vehicle is somehow flying without being an aircraft" );
        return true;
    }
    if( velocity > 0 && !is_flying_in_air() ) {
        p.add_msg_if_player( m_bad, _( "It would be unsafe to try and take off while you are moving." ) );
        return false;
    }
    if( sm_pos.z() + 1 >= OVERMAP_HEIGHT ) {
        return false;
    }
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_abs_ms &pt : get_points( true ) ) {
        const tripoint_bub_ms pos = here.get_bub( pt );
        tripoint_bub_ms above( pos + tripoint::above );
        const optional_vpart_position ovp = here.veh_at( above );
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, pos ) ||
            here.impassable_ter_furn( above ) ||
            ovp ||
            creatures.creature_at( above ) ) {
            p.add_msg_if_player( m_bad,
                                 _( "It would be unsafe to try and ascend when there are obstacles above you." ) );
            return false;
        }
    }
    return true;
}

void vehicle::pldrive( map &here, Character &driver, const int trn, const int acceleration,
                       const int z )
{
    bool is_non_proficient = false;
    float effective_driver_skill = driver.get_skill_level( skill_driving );
    float vehicle_proficiency;

    if( !driver.has_proficiency( proficiency_prof_driver ) && !in_deep_water ) {
        is_non_proficient = true;
        vehicle_proficiency = driver.get_proficiency_practice( proficiency_prof_driver );
    } else if( !driver.has_proficiency( proficiency_prof_boat_pilot ) && in_deep_water ) {
        is_non_proficient = true;
        vehicle_proficiency = driver.get_proficiency_practice( proficiency_prof_boat_pilot );
    }

    bool non_prof_fumble = false;
    float non_prof_penalty = 0;

    if( is_non_proficient ) {
        effective_driver_skill *= vehicle_proficiency;
        non_prof_penalty = std::max( 0.0f,
                                     ( 1.0f - vehicle_proficiency ) * 10.0f -
                                     ( driver.get_dex() + driver.get_per() ) * 0.25f );
        non_prof_fumble = one_in( vehicle_proficiency * 12.0f +
                                  ( driver.get_dex() + driver.get_per() ) * 0.5f + 6.0f );







    }
    if( z != 0 && is_rotorcraft( here ) ) {
        driver.set_moves( std::min( driver.get_moves(), 0 ) );
        thrust( here, 0, z );
    }
    units::angle turn_delta = vehicles::steer_increment * trn;
    const float handling_diff = handling_difficulty( here ) + non_prof_penalty;
    if( turn_delta != 0_degrees ) {
        float eff = steering_effectiveness( here );
        if( eff == -2 ) {
            driver.add_msg_if_player( m_info,
                                      _( "You cannot steer an animal-drawn vehicle with no animal harnessed." ) );
            return;
        }

        if( eff < 0 ) {
            driver.add_msg_if_player( m_info,
                                      _( "This vehicle has no steering system installed, you can't turn it." ) );
            return;
        }

        if( eff == 0 ) {
            driver.add_msg_if_player( m_bad, _( "The steering is completely broken!" ) );
            return;
        }



        driver.set_moves( std::min( driver.get_moves(), driver.get_speed() ) );






        float skill = std::min( 10.0f, effective_driver_skill +
                                ( driver.get_dex() + driver.get_per() ) / 10.0f );
        float penalty = rng_float( 0.0f, handling_diff ) - skill;

        int cost;
        if( penalty > 0.0f ) {

            cost = 100 * ( 1.0f + penalty / 2.5f );
        } else {

            cost = std::max( driver.get_speed(), 100 ) * ( 1.0f - ( -penalty / 10.0f ) * 2 / 3 );
        }


        if( non_prof_fumble || penalty > skill || ( penalty > 0 && cost > 400 ) ) {
            int fumble_roll = rng( 0, 6 );
            int fumble_factor = 0;
            int fumble_time = 1;
            if( fumble_roll <= 1 ) {

                driver.add_msg_if_player( m_warning, _( "You fumble with the %s's controls." ), name );
            } else if( fumble_roll <= 4 ) {

                driver.add_msg_if_player( m_warning, _( "You turn slower than you meant to." ) );
                fumble_factor = 1;
                fumble_time = 2;
            } else {

                driver.add_msg_if_player( m_warning, _( "You oversteer the %s!" ), name );
                fumble_factor = 2;
                fumble_time = 2;
            }
            turn_delta *= fumble_factor;
            cost = std::max( cost, driver.get_moves() + fumble_time * 100 );
        } else if( one_in( 10 ) ) {

            if( cost >= driver.get_speed() * 2 ) {
                driver.add_msg_if_player( m_warning, _( "It takes you a very long time to steer the vehicle!" ) );
            } else if( cost >= driver.get_speed() * 1.5f ) {
                driver.add_msg_if_player( m_warning, _( "It takes you a long time to steer the vehicle!" ) );
            }
        }

        turn( turn_delta );


        driver.mod_moves( -std::max( cost, driver.get_speed() / 3 + 1 ) );
    }

    if( acceleration != 0 ) {
        cruise_thrust( here, -acceleration * 400 );
    }


    if( skidding && trn != 0 && valid_wheel_config( here ) ) {



        if( handling_diff * rng( 1, 10 ) <
            driver.get_dex() + effective_driver_skill * 2 ) {
            driver.add_msg_if_player( _( "You regain control of the %s." ), name );
            driver.practice( skill_driving, velocity / 5 );
            velocity = static_cast<int>( forward_velocity() );
            skidding = false;
            move.init( turn_dir );
        }
    }
}


void vehicle::possibly_recover_from_skid()
{
    if( last_turn > 13_degrees ) {

        return;
    }

    rl_vec2d mv = move_vec();
    rl_vec2d fv = face_vec();
    float dot = mv.dot_product( fv );


    if( std::fabs( dot ) * 100 > dice( 9, 20 ) ) {
        add_msg( _( "The %s recovers from its skid." ), name );

        skidding = false;

        velocity *= dot;
        if( dot < -.8 ) {


            velocity *= -1;
        }

        move = face;
    }
}


float vehicle::forward_velocity() const
{
    rl_vec2d mv = move_vec();
    rl_vec2d fv = face_vec();
    float dot = mv.dot_product( fv );
    return velocity * dot;
}

rl_vec2d vehicle::velo_vec() const
{
    rl_vec2d ret;
    if( skidding ) {
        ret = move_vec();
    } else {
        ret = face_vec();
    }
    ret = ret.normalized();
    ret = ret * velocity;
    return ret;
}

static rl_vec2d angle_to_vec( const units::angle &angle )
{
    return rl_vec2d( units::cos( angle ), units::sin( angle ) );
}


rl_vec2d vehicle::move_vec() const
{
    return angle_to_vec( move.dir() );
}


rl_vec2d vehicle::face_vec() const
{
    return angle_to_vec( face.dir() );
}

rl_vec2d vehicle::dir_vec() const
{
    return angle_to_vec( turn_dir );
}

float get_collision_factor( const float delta_v )
{
    if( std::abs( delta_v ) <= 31 ) {
        return 1 - ( 0.9 * std::abs( delta_v ) ) / 31;
    } else {
        return 0.1;
    }
}

void vehicle::precalculate_vehicle_turning( map &here, units::angle new_turn_dir,
        bool check_rail_direction,
        const ter_furn_flag ter_flag_to_check, int &wheels_on_rail,
        int &turning_wheels_that_are_one_axis ) const
{

    tileray mdir;

    mdir.init( new_turn_dir );
    tripoint_rel_ms dp;
    bool is_diagonal_movement = std::lround( to_degrees( new_turn_dir ) ) % 90 == 45;

    if( std::abs( velocity ) >= 20 ) {
        mdir.advance( velocity < 0 ? -1 : 1 );
        dp.x() = mdir.dx();
        dp.y() = mdir.dy();
    }


    wheels_on_rail = 0;


    int yVal = INT_MAX;





    turning_wheels_that_are_one_axis = 0;

    for( int part_index : wheelcache ) {
        const auto &wheel = parts[ part_index ];
        bool rails_ahead = true;
        tripoint_rel_ms wheel_point;
        coord_translate( mdir.dir(), this->pivot_point( here ), wheel.mount,
                         wheel_point );

        tripoint_bub_ms wheel_tripoint = pos_bub( here ) + wheel_point;


        const int allowed_incorrect_tiles_diagonal = 1;
        const int allowed_incorrect_tiles_not_diagonal = 2;
        int incorrect_tiles_diagonal = 0;
        int incorrect_tiles_not_diagonal = 0;


        for( int try_num = 0; try_num < 3; try_num++ ) {

            wheel_tripoint += dp;

            if( !here.has_flag_ter_or_furn( ter_flag_to_check, wheel_tripoint ) ) {

                rails_ahead = false;
                break;
            }


            if( check_rail_direction ) {
                const ter_str_id &terrain_at_wheel = here.ter( wheel_tripoint ).id();

                if( !is_diagonal_movement ) {
                    const std::unordered_set<ter_str_id> diagonal_track_ters = { ter_t_railroad_track_d, ter_t_railroad_track_d1, ter_t_railroad_track_d2, ter_t_railroad_track_d_on_tie };
                    if( diagonal_track_ters.find( terrain_at_wheel ) != diagonal_track_ters.end() ) {
                        incorrect_tiles_not_diagonal++;
                    }
                } else if( const std::unordered_set<ter_str_id> straight_track_ters = { ter_t_railroad_track, ter_t_railroad_track_on_tie, ter_t_railroad_track_h, ter_t_railroad_track_v, ter_t_railroad_track_h_on_tie, ter_t_railroad_track_v_on_tie };
                           straight_track_ters.find( terrain_at_wheel ) != straight_track_ters.end() ) {
                    incorrect_tiles_not_diagonal++;
                }
                if( incorrect_tiles_diagonal > allowed_incorrect_tiles_diagonal ||
                    incorrect_tiles_not_diagonal > allowed_incorrect_tiles_not_diagonal ) {
                    rails_ahead = false;
                    break;
                }
            }
        }

        if( rails_ahead ) {

            if( yVal == INT_MAX ) {

                yVal = wheel.mount.y();
            }
            if( yVal == wheel.mount.y() ) {
                turning_wheels_that_are_one_axis++;
            }
            wheels_on_rail++;
        }
    }
}


static units::angle get_corrected_turn_dir( const units::angle &turn_dir,
        const units::angle &face_dir )
{
    units::angle corrected_turn_dir = 0_degrees;


    if( turn_dir > face_dir && turn_dir < face_dir + 180_degrees ) {
        corrected_turn_dir = face_dir + 45_degrees;
    } else if( turn_dir < face_dir || turn_dir > 270_degrees ) {
        corrected_turn_dir = face_dir - 45_degrees;
    }
    return normalize( corrected_turn_dir );
}

bool vehicle::allow_manual_turn_on_rails( map &here, units::angle &corrected_turn_dir ) const
{
    bool allow_turn_on_rail = false;

    if( turn_dir != face.dir() ) {
        corrected_turn_dir = get_corrected_turn_dir( turn_dir, face.dir() );

        int wheels_on_rail;
        int turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( here,  corrected_turn_dir, true, ter_furn_flag::TFLAG_RAIL,
                                      wheels_on_rail,
                                      turning_wheels_that_are_one_axis );
        if( is_wheel_state_correct_to_turn_on_rails( wheels_on_rail, rail_wheelcache.size(),
                turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
        }
    }
    return allow_turn_on_rail;
}

bool vehicle::allow_auto_turn_on_rails( map &here, units::angle &corrected_turn_dir ) const
{
    bool allow_turn_on_rail = false;

    if( turn_dir == face.dir() ) {

        int straight_wheels_on_rail;
        int straight_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( here, face.dir(), true, ter_furn_flag::TFLAG_RAIL,
                                      straight_wheels_on_rail,
                                      straight_turning_wheels_that_are_one_axis );

        units::angle left_turn_dir =
            get_corrected_turn_dir( face.dir() - 45_degrees, face.dir() );
        int leftturn_wheels_on_rail;
        int leftturn_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( here, left_turn_dir, true, ter_furn_flag::TFLAG_RAIL,
                                      leftturn_wheels_on_rail,
                                      leftturn_turning_wheels_that_are_one_axis );

        units::angle right_turn_dir =
            get_corrected_turn_dir( face.dir() + 45_degrees, face.dir() );
        int rightturn_wheels_on_rail;
        int rightturn_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( here, right_turn_dir, true, ter_furn_flag::TFLAG_RAIL,
                                      rightturn_wheels_on_rail,
                                      rightturn_turning_wheels_that_are_one_axis );


        if( straight_wheels_on_rail <= leftturn_wheels_on_rail &&
            is_wheel_state_correct_to_turn_on_rails( leftturn_wheels_on_rail, rail_wheelcache.size(),
                    leftturn_turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
            corrected_turn_dir = left_turn_dir;
        } else if( straight_wheels_on_rail <= rightturn_wheels_on_rail &&
                   is_wheel_state_correct_to_turn_on_rails( rightturn_wheels_on_rail, rail_wheelcache.size(),
                           rightturn_turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
            corrected_turn_dir = right_turn_dir;
        }
    }
    return allow_turn_on_rail;
}

bool vehicle::is_wheel_state_correct_to_turn_on_rails( int wheels_on_rail, int wheel_count,
        int turning_wheels_that_are_one_axis ) const
{
    return ( wheels_on_rail >= 2 ||
             ( wheels_on_rail == 1 && ( wheel_count == 1 ||
                                        all_wheels_on_one_axis ) ) )
           && ( wheels_on_rail !=
                turning_wheels_that_are_one_axis
                || all_wheels_on_one_axis ||
                ( std::abs( rail_wheel_bounding_box.p2.x() - rail_wheel_bounding_box.p1.x() ) < 4 &&
                  velocity < 0 ) );

}

vehicle *vehicle::act_on_map( map &here )
{
    const tripoint_bub_ms pt = pos_bub( here );
    if( !here.inbounds( pt ) ) {
        dbg( D_INFO ) << "stopping out-of-map vehicle.  (x,y,z)=(" << pt.x() << "," << pt.y() << "," <<
                      pt.z() <<
                      ")";
        stop( here );
        of_turn = 0;
        is_falling = false;
        return this;
    }
    if( decrement_summon_timer( here ) ) {
        return nullptr;
    }
    const bool pl_ctrl = player_is_driving_this_veh( &here );

    if( in_deep_water && !can_float( here ) ) {
        add_msg( m_bad, _( "Your %s sank." ), name );
        if( pl_ctrl ) {
            unboard_all( here );
        }
        if( g->remoteveh() == this ) {
            g->setremoteveh( nullptr );
        }

        here.on_vehicle_moved( sm_pos.z() );

        here.destroy_vehicle( this );
        return nullptr;
    }



    const bool should_fall = is_falling || vertical_velocity != 0;




    if( should_fall ) {

        const float g = 9.8f;

        const float old_vel = vmiph_to_mps( vertical_velocity );


        const float new_vel = -std::sqrt( 2 * tile_height * g + old_vel * old_vel );
        vertical_velocity = mps_to_vmiph( new_vel );
        is_falling = true;
    } else {

        is_falling = false;
    }



    if( !should_fall && std::abs( velocity ) < 20 && requested_z_change == 0 ) {
        stop( here );
        of_turn -= .321f;
        return this;
    }

    const float wheel_traction_area = here.vehicle_wheel_traction( *this );
    const float traction = k_traction( here, wheel_traction_area );
    if( traction < 0.001f ) {
        of_turn = 0;
        if( !should_fall ) {
            stop( here );
            if( floating.empty() ) {
                add_msg( m_info, _( "Your %s can't move on this terrain." ), name );
            } else {
                add_msg( m_info, _( "Your %s is beached." ), name );
            }
            return this;
        }
    }
    const float turn_cost = vehicles::vmiph_per_tile / std::max<float>( 0.0001f, std::abs( velocity ) );



    bool falling_only = false;
    if( turn_cost >= of_turn && ( ( !is_flying && requested_z_change == 0 ) ||
                                  !is_rotorcraft( here ) ) ) {
        if( !should_fall ) {
            of_turn_carry = of_turn;
            of_turn = 0;
            return this;
        }
        falling_only = true;
    }


    if( !falling_only ) {
        of_turn -= turn_cost;
    }

    bool can_use_rails = this->can_use_rails( here );
    if( one_in( 10 ) ) {
        bool controlled = false;

        for( int boarded : boarded_parts() ) {
            if( part_with_feature( boarded, VPFLAG_CONTROLS, true ) >= 0 ) {
                controlled = true;
                Character *passenger = get_passenger( boarded );
                if( passenger != nullptr ) {
                    passenger->practice( skill_driving, 1 );
                }
            }
        }



        if( !controlled && !pl_ctrl && !( is_watercraft() && can_float( here ) ) && !can_use_rails &&
            !is_flying && requested_z_change == 0 ) {
            skidding = true;
        }
    }

    if( skidding && one_in( 4 ) ) {

        turn( vehicles::steer_increment * ( one_in( 2 ) ? -1 : 1 ) );
    }

    if( should_fall ) {

        skidding = true;
    }

    bool allow_turn_on_rail = false;
    if( can_use_rails && !falling_only ) {
        units::angle corrected_turn_dir;
        allow_turn_on_rail = allow_manual_turn_on_rails( here,  corrected_turn_dir );
        if( !allow_turn_on_rail ) {
            allow_turn_on_rail = allow_auto_turn_on_rails( here, corrected_turn_dir );
        }
        if( allow_turn_on_rail ) {
            turn_dir = corrected_turn_dir;
        }
    }


    tileray mdir;
    if( skidding || should_fall ) {


        mdir = move;
    } else if( turn_dir != face.dir() && ( !can_use_rails || allow_turn_on_rail ) ) {

        mdir.init( turn_dir );
    } else {

        mdir = face;
    }

    tripoint_rel_ms dp;
    if( std::abs( velocity ) >= 20 && !falling_only ) {
        mdir.advance( velocity < 0 ? -1 : 1 );
        dp.x() = mdir.dx();
        dp.y() = mdir.dy();
    }

    if( should_fall ) {
        dp.z() = -1;
        is_flying = false;
    } else {
        dp.z() = requested_z_change;
        requested_z_change = 0;
        if( dp.z() > 0 && is_rotorcraft( here ) ) {
            is_flying = true;
        }
    }

    return here.move_vehicle( *this, dp, mdir );
}

bool vehicle::level_vehicle( map &here )
{
    if( is_flying && is_rotorcraft( here ) ) {
        return true;
    }
    is_on_ramp = false;

    std::map<int, bool> no_support;
    for( vehicle_part &prt : parts ) {
        if( prt.info().location != vpart_location_structure ) {
            continue;
        }
        const tripoint_bub_ms part_pos = bub_part_pos( here, prt );
        if( no_support.find( part_pos.z() ) == no_support.end() ) {
            no_support[part_pos.z()] = part_pos.z() > -OVERMAP_DEPTH;
        }
        if( no_support[part_pos.z()] ) {
            no_support[part_pos.z()] = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, part_pos ) &&
                                       !here.supports_above( part_pos + tripoint::below );
        }
        if( !is_on_ramp &&
            ( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, tripoint_bub_ms( part_pos.x(), part_pos.y(),
                             part_pos.z() - 1 ) ) ||
              here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, tripoint_bub_ms( part_pos.x(), part_pos.y(),
                             part_pos.z() + 1 ) ) ) ) {
            is_on_ramp = true;
        }
    }
    std::set<int> dropped_parts;

    bool center_drop = false;
    bool adjust_level = false;
    if( no_support.size() > 1 ) {
        for( int zlevel = -OVERMAP_DEPTH; zlevel <= OVERMAP_DEPTH; zlevel++ ) {
            if( no_support.find( zlevel ) == no_support.end() || !no_support[zlevel] ) {
                continue;
            }
            center_drop |= pos_abs().z() == zlevel;
            adjust_level = true;

            for( size_t prt = 0; prt < parts.size(); prt++ ) {
                if( abs_part_pos( prt ).z() == zlevel ) {
                    dropped_parts.insert( static_cast<int>( prt ) );
                }
            }
        }
    }
    if( adjust_level ) {
        here.displace_vehicle( *this, tripoint_rel_ms::below, center_drop, dropped_parts );
        return false;
    } else {
        return true;
    }
}

void vehicle::check_falling_or_floating()
{
    map &here = get_map();


    if( is_flying && is_rotorcraft( here ) ) {
        is_falling = false;
        in_deep_water = false;
        in_water = false;
        return;
    }

    is_falling = true;
    is_flying = false;

    auto has_support = [&here]( const tripoint_bub_ms & position, const bool water_supports ) {

        if( position.z() == -OVERMAP_DEPTH ) {
            return true;
        }


        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SWIMMABLE, position ) ) {
            return water_supports;
        }
        if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, position ) ) {
            return true;
        }
        tripoint_bub_ms below( position + tripoint::below );
        return here.supports_above( below );
    };

    int supported_wheels = 0;
    for( int wheel_index : wheelcache ) {
        const tripoint_bub_ms position = bub_part_pos( here, wheel_index );
        if( has_support( position, false ) ) {
            ++supported_wheels;
        }
    }

    if( supported_wheels > 0 &&
        static_cast<size_t>( supported_wheels ) * 2 >= wheelcache.size() ) {
        is_falling = false;
        in_water = false;
        in_deep_water = false;
        return;
    }


    const std::set<tripoint_abs_ms> &pts = get_points();
    if( pts.empty() ) {

        is_falling = false;
        in_deep_water = false;
        in_water = false;
        is_flying = false;
        return;
    }

    size_t deep_water_tiles = 0;
    size_t water_tiles = 0;
    for( const tripoint_abs_ms &position : pts ) {
        const tripoint_bub_ms pos = here.get_bub( position );
        deep_water_tiles += here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos ) ? 1 : 0;
        water_tiles += here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos ) ? 1 : 0;
        if( !is_falling ) {
            continue;
        }
        is_falling = !has_support( pos, true );
    }

    in_deep_water = 3 * deep_water_tiles >= 2 * pts.size();

    in_water =  2 * water_tiles >= pts.size();
}

float map::vehicle_wheel_traction( const vehicle &veh, bool ignore_movement_modifiers )
{
    if( veh.is_in_water( true ) ) {
        return veh.can_float( *this ) ? 1.0f : -1.0f;
    }
    if( veh.is_watercraft() && veh.can_float( *this ) ) {
        return 1.0f;
    }

    if( veh.wheelcache.empty() ) {


        return 0.0f;
    }

    float traction_wheel_area = 0.0f;
    for( const int wheel_idx : veh.wheelcache ) {
        const vehicle_part &vp = veh.part( wheel_idx );
        const vpart_info &vpi = vp.info();
        const tripoint_bub_ms pp = veh.bub_part_pos( *this, vp );
        const ter_t &tr = ter( pp ).obj();
        if( tr.has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ||
            tr.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
            continue;
        }

        int move_mod = move_cost_ter_furn( pp );
        if( move_mod == 0 ) {
            return 0.0f;
        }

        if( ignore_movement_modifiers ) {
            traction_wheel_area += vp.contact_area();
            continue;
        }

        for( const veh_ter_mod &mod : vpi.wheel_info->terrain_modifiers ) {
            const bool ter_has_flag = tr.has_flag( mod.terrain_flag );
            if( ter_has_flag && mod.move_override ) {
                move_mod = mod.move_override;
                break;
            } else if( !ter_has_flag && mod.move_penalty ) {
                move_mod += mod.move_penalty;
                break;
            }
        }

        move_mod = std::max( move_mod, move_mod + vp.move_penalty() );

        if( move_mod == 0 ) {
            debugmsg( "move_mod resulted in a 0, ignoring wheel" );
            continue;
        }


        constexpr float THICK_ICE_TRACTION_FACTOR = 0.3f;
        if( tr.has_flag( ter_furn_flag::TFLAG_THICK_ICE ) ) {
            traction_wheel_area += 2.0 * vp.contact_area() / move_mod * THICK_ICE_TRACTION_FACTOR;
        } else {
            traction_wheel_area += 2.0 * vp.contact_area() / move_mod;
        }
    }

    return traction_wheel_area;
}

float map::vehicle_wheel_traction( const vehicle &veh, const tripoint_bub_ms &at_origin,
                                   bool ignore_movement_modifiers )
{
    if( veh.is_in_water( true ) ) {
        return veh.can_float( *this ) ? 1.0f : -1.0f;
    }
    if( veh.is_watercraft() && veh.can_float( *this ) ) {
        return 1.0f;
    }

    if( veh.wheelcache.empty() ) {
        return 0.0f;
    }

    float traction_wheel_area = 0.0f;
    for( const int wheel_idx : veh.wheelcache ) {
        const vehicle_part &vp = veh.part( wheel_idx );
        const vpart_info &vpi = vp.info();
        const tripoint_bub_ms pp = at_origin + vp.precalc[0];
        const ter_t &tr = ter( pp ).obj();
        if( tr.has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ||
            tr.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
            continue;
        }

        int move_mod = move_cost_ter_furn( pp );
        if( move_mod == 0 ) {
            return 0.0f;
        }

        if( ignore_movement_modifiers ) {
            traction_wheel_area += vp.contact_area();
            continue;
        }

        for( const veh_ter_mod &mod : vpi.wheel_info->terrain_modifiers ) {
            const bool ter_has_flag = tr.has_flag( mod.terrain_flag );
            if( ter_has_flag && mod.move_override ) {
                move_mod = mod.move_override;
                break;
            } else if( !ter_has_flag && mod.move_penalty ) {
                move_mod += mod.move_penalty;
                break;
            }
        }

        move_mod = std::max( move_mod, move_mod + vp.move_penalty() );

        if( move_mod == 0 ) {
            debugmsg( "move_mod resulted in a 0, ignoring wheel" );
            continue;
        }

        constexpr float THICK_ICE_TRACTION_FACTOR = 0.3f;
        if( tr.has_flag( ter_furn_flag::TFLAG_THICK_ICE ) ) {
            traction_wheel_area += 2.0 * vp.contact_area() / move_mod * THICK_ICE_TRACTION_FACTOR;
        } else {
            traction_wheel_area += 2.0 * vp.contact_area() / move_mod;
        }
    }

    return traction_wheel_area;
}

units::angle map::shake_vehicle( vehicle &veh, const int velocity_before,
                                 const units::angle &direction )
{
    map &here = get_map();
    const int d_vel = std::abs( veh.velocity - velocity_before ) / 100;

    std::vector<rider_data> riders = veh.get_riders();

    units::angle coll_turn = 0_degrees;
    for( const rider_data &r : riders ) {
        const int ps = r.prt;
        Creature *rider = r.psg;
        if( rider == nullptr ) {
            debugmsg( "throw passenger: empty passenger at part %d", ps );
            continue;
        }

        const tripoint_bub_ms rider_pos = rider->pos_bub( here );
        const tripoint_bub_ms part_pos = veh.bub_part_pos( here, ps );
        if( rider->pos_bub() != part_pos ) {
            debugmsg( "throw passenger: passenger at %d,%d,%d, part at %d,%d,%d",
                      rider_pos.x(), rider_pos.y(), rider_pos.z(),
                      part_pos.x(), part_pos.y(), part_pos.z() );
            veh.part( ps ).remove_flag( vp_flag::passenger_flag );
            continue;
        }

        Character *psg = dynamic_cast<Character *>( rider );
        monster *pet = dynamic_cast<monster *>( rider );

        bool throw_from_seat = false;
        int dmg = d_vel * rng_float( 1.0f, 2.0f );
        int move_resist = 1;
        if( psg ) {

            move_resist = psg->get_str() * 150 + 500;
            if( veh.part( ps ).info().has_flag( "SEAT_REQUIRES_BALANCE" ) ) {


                int resist_penalty = 500;
                if( psg->has_trait( trait_PROF_SKATER ) ) {
                    resist_penalty -= 150;
                }
                if( psg->has_trait( trait_DEFT ) ) {
                    resist_penalty -= 150;
                }
                move_resist -= resist_penalty;
            }
        } else {
            int pet_resist = 0;
            if( pet != nullptr ) {
                pet_resist = static_cast<int>( to_kilogram( pet->get_weight() ) * 200 );
            }
            move_resist = std::max( 100, pet_resist );
        }
        const int belt_idx = veh.part_with_feature( ps, VPFLAG_SEATBELT, true );
        if( belt_idx == -1 ) {

            throw_from_seat = d_vel * rng( 80, 120 ) > move_resist;
        } else {

            dmg -= veh.part( belt_idx ).info().bonus;
        }


        if( !throw_from_seat && ( 10 * d_vel ) > 6 * rng( 25, 50 ) ) {
            if( psg ) {
                psg->deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_bash, dmg ) );
                psg->add_msg_player_or_npc( m_bad,
                                            _( "You take %d damage by the power of the impact!" ),
                                            _( "<npcname> takes %d damage by the power of the "
                                               "impact!" ), dmg );
            } else {
                pet->apply_damage( nullptr, bodypart_id( "torso" ), dmg );
            }
        }

        if( psg && veh.player_in_control( here, *psg ) ) {
            const int lose_ctrl_roll = rng( 0, d_vel );



            if( lose_ctrl_roll > psg->get_dex() * 2 + psg->get_skill_level( skill_driving ) * 3 ) {
                psg->add_msg_player_or_npc( m_warning,
                                            _( "You lose control of the %s." ),
                                            _( "<npcname> loses control of the %s." ), veh.name );
                int turn_amount = rng( 1, 3 ) * std::sqrt( std::abs( veh.velocity ) ) / 30;
                if( turn_amount < 1 ) {
                    turn_amount = 1;
                }
                units::angle turn_angle = std::min( turn_amount * vehicles::steer_increment, 120_degrees );
                coll_turn = one_in( 2 ) ? turn_angle : -turn_angle;
            }
        }

        if( throw_from_seat ) {
            if( psg ) {
                psg->add_msg_player_or_npc( m_bad,
                                            _( "You are hurled from the %s's seat by "
                                               "the power of the impact!" ),
                                            _( "<npcname> is hurled from the %s's seat by "
                                               "the power of the impact!" ), veh.name );
                unboard_vehicle( part_pos );
            } else {
                add_msg_if_player_sees( part_pos, m_bad,
                                        _( "The %s is hurled from %s's by the power of the impact!" ),
                                        pet->disp_name(), veh.name );
            }

            g->fling_creature( rider, direction + rng_float( -30_degrees, 30_degrees ),
                               std::max( 10, d_vel - move_resist / 100 ) );
        }
    }

    return coll_turn;
}

bool vehicle::should_enable_fake( const tripoint_rel_ms &fake_precalc,
                                  const tripoint_rel_ms &parent_precalc,
                                  const tripoint_rel_ms &neighbor_precalc ) const
{

    tripoint abs_parent_neighbor_diff = ( parent_precalc - neighbor_precalc ).raw().abs();
    tripoint abs_fake_neighbor_diff = ( fake_precalc - neighbor_precalc ).raw().abs();
    return ( abs_parent_neighbor_diff.x == 1 && abs_parent_neighbor_diff.y == 1 ) &&
           ( ( abs_fake_neighbor_diff.x == 1 && abs_fake_neighbor_diff.y == 0 ) ||
             ( abs_fake_neighbor_diff.x == 0 && abs_fake_neighbor_diff.y == 1 ) );
}

void vehicle::update_active_fakes()
{
    for( const int fake_index : fake_parts ) {
        vehicle_part &part_fake = parts.at( fake_index );
        if( part_fake.removed ) {
            continue;
        }
        const vehicle_part &part_real = parts.at( part_fake.fake_part_to );
        const tripoint_rel_ms &fake_precalc = part_fake.precalc[0];
        const tripoint_rel_ms &real_precalc = part_real.precalc[0];
        const vpart_edge_info &real_edge = edges[part_real.mount];
        const bool is_protrusion = part_real.info().has_flag( "PROTRUSION" );

        if( real_edge.forward != -1 ) {
            const tripoint_rel_ms &forward = parts.at( real_edge.forward ).precalc[0];
            part_fake.is_active_fake = should_enable_fake( fake_precalc, real_precalc, forward );
        }
        if( real_edge.back != -1 && ( !part_fake.is_active_fake || real_edge.forward == -1 ) ) {
            const tripoint_rel_ms &back = parts.at( real_edge.back ).precalc[0];
            part_fake.is_active_fake = should_enable_fake( fake_precalc, real_precalc, back );
        }
        if( is_protrusion && part_fake.fake_protrusion_on >= 0 ) {
            part_fake.is_active_fake = parts.at( part_fake.fake_protrusion_on ).is_active_fake;
        }
    }
}
