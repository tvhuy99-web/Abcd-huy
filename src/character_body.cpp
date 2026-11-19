#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "activity_tracker.h"
#include "a11y_body.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_attire.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "display.h"
#include "effect.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "math_parser_diag_value.h"
#include "map.h"
#include "mapdata.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "rng.h"
#include "stomach.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"

static const addiction_id addiction_nicotine( "nicotine" );

static const bionic_id bio_sleep_shutdown( "bio_sleep_shutdown" );

static const character_modifier_id
character_modifier_stamina_recovery_breathing_mod( "stamina_recovery_breathing_mod" );

static const damage_type_id damage_heat( "heat" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_betablock( "betablock" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blisters( "blisters" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_cold( "cold" );
static const efftype_id effect_common_cold( "common_cold" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_flu( "flu" );
static const efftype_id effect_frostbite( "frostbite" );
static const efftype_id effect_frostbite_recovery( "frostbite_recovery" );
static const efftype_id effect_hot( "hot" );
static const efftype_id effect_hot_speed( "hot_speed" );
static const efftype_id effect_hunger_blank( "hunger_blank" );
static const efftype_id effect_hunger_engorged( "hunger_engorged" );
static const efftype_id effect_hunger_famished( "hunger_famished" );
static const efftype_id effect_hunger_full( "hunger_full" );
static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_near_starving( "hunger_near_starving" );
static const efftype_id effect_hunger_satisfied( "hunger_satisfied" );
static const efftype_id effect_hunger_starving( "hunger_starving" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );
static const efftype_id effect_hypovolemia( "hypovolemia" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_wet( "wet" );

static const json_character_flag json_flag_BARKY( "BARKY" );
static const json_character_flag json_flag_CANNOT_CHANGE_TEMPERATURE( "CANNOT_CHANGE_TEMPERATURE" );
static const json_character_flag json_flag_COLDBLOOD( "COLDBLOOD" );
static const json_character_flag json_flag_COLDBLOOD2( "COLDBLOOD2" );
static const json_character_flag json_flag_COLDBLOOD3( "COLDBLOOD3" );
static const json_character_flag json_flag_ECTOTHERM( "ECTOTHERM" );
static const json_character_flag json_flag_HEATSINK( "HEATSINK" );
static const json_character_flag json_flag_HEAT_IMMUNE( "HEAT_IMMUNE" );
static const json_character_flag json_flag_HUNGER_DISRUPTION( "HUNGER_DISRUPTION" );
static const json_character_flag json_flag_IGNORE_TEMP( "IGNORE_TEMP" );
static const json_character_flag json_flag_LIMB_LOWER( "LIMB_LOWER" );
static const json_character_flag json_flag_NO_THIRST( "NO_THIRST" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const morale_type morale_comfy( "morale_comfy" );
static const morale_type morale_pyromania_nearfire( "morale_pyromania_nearfire" );
static const morale_type morale_pyromania_nofire( "morale_pyromania_nofire" );

static const trait_id trait_CHITIN_FUR( "CHITIN_FUR" );
static const trait_id trait_CHITIN_FUR2( "CHITIN_FUR2" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_DEBUG_LS( "DEBUG_LS" );
static const trait_id trait_DEBUG_NOTEMP( "DEBUG_NOTEMP" );
static const trait_id trait_FELINE_FUR( "FELINE_FUR" );
static const trait_id trait_FUR( "FUR" );
static const trait_id trait_LIGHTFUR( "LIGHTFUR" );
static const trait_id trait_LUPINE_FUR( "LUPINE_FUR" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_SLIMY( "SLIMY" );
static const trait_id trait_URSINE_FUR( "URSINE_FUR" );

static const vitamin_id vitamin_blood( "blood" );

void Character::update_body_wetness( const w_point &weather )
{


    constexpr time_duration average_drying = 30_minutes;


    float trait_mult = 1.0;
    if( has_trait( trait_LIGHTFUR ) || has_trait( trait_FUR ) || has_trait( trait_FELINE_FUR ) ||
        has_trait( trait_LUPINE_FUR ) || has_trait( trait_CHITIN_FUR ) || has_trait( trait_CHITIN_FUR2 ) ||
        has_trait( trait_CHITIN_FUR3 ) ) {
        trait_mult = 2.0;
    }
    if( has_trait( trait_URSINE_FUR ) || has_trait( trait_SLIMY ) ) {
        trait_mult = 4.0;
    }


    float weather_mult = 1.0;
    weather_mult += ( ( weather.humidity - 66.0f ) - ( units::to_fahrenheit(
                          weather.temperature ) - 65.0f ) ) / 100.0f;
    weather_mult = std::max( 0.1f, weather_mult );

    for( const bodypart_id &bp : get_all_body_parts() ) {

        const units::temperature temp_conv = get_part_temp_conv( bp );
        const int drench_cap = get_part_drench_capacity( bp );

        if( !is_underwater() ) {
            const int wetness = get_part_wetness( bp );
            if( wetness == 0 ) {
                remove_effect( effect_wet, bp );
                continue;
            }



            float temp_mult = 1.0;
            if( temp_conv >= BODYTEMP_SCORCHING ) {
                temp_mult = 0.5;
            } else if( temp_conv >= BODYTEMP_VERY_HOT ) {
                temp_mult = 0.67;
            } else if( temp_conv >= BODYTEMP_HOT ) {
                temp_mult = 0.75;
            } else if( temp_conv > BODYTEMP_COLD ) {

            } else {

                temp_mult = 1.2;
            }


            const float clothing_mult = worn.clothing_wetness_mult( bp );
            const float drying_rate = bp->drying_rate;
            const time_duration drying = average_drying * trait_mult * weather_mult *
                                         temp_mult / clothing_mult;
            const float turns_to_dry = to_turns<float>( drying ) / drying_rate;
            const float dry_per_turn = static_cast<float>( drench_cap ) / turns_to_dry;
            mod_part_wetness( bp, roll_remainder( dry_per_turn ) * -1 );





            if( !bp->has_flag( json_flag_IGNORE_TEMP ) && !has_flag( json_flag_CANNOT_CHANGE_TEMPERATURE ) ) {
                mod_part_temp_cur( bp, -0.008_C_delta * clothing_mult );
            }
        }


        if( get_part_wetness( bp ) <= 0 ) {

            if( temp_conv >= BODYTEMP_HOT && drench_cap > 0 ) {
                set_part_wetness( bp, 1 );
            } else {
                set_part_wetness( bp, 0 );
            }
        }
        if( get_part_wetness( bp ) > drench_cap ) {
            set_part_wetness( bp, drench_cap );
        }


        if( get_part_wetness( bp ) > 0 ) {
            const float wetness_pct = get_part_wetness_percentage( bp );
            const int effect_level =
                wetness_pct < BODYWET_PERCENT_WET ? 1 :
                wetness_pct < BODYWET_PERCENT_SOAKED ? 2 :
                3;

            add_effect( effect_wet, 1_turns, bp, true, effect_level );
        }
    }
}

void Character::update_body()
{
    update_body( calendar::turn - 1_turns, calendar::turn );
    last_updated = calendar::turn;
}



static int ticks_between( const time_point &from, const time_point &to,
                          const time_duration &tick_length )
{
    return ( to_turn<int>( to ) / to_turns<int>( tick_length ) ) - ( to_turn<int>
            ( from ) / to_turns<int>( tick_length ) );
}

void Character::update_body( const time_point &from, const time_point &to )
{

    if( to <= last_updated ) {
        last_updated = to;
        return;
    }
    if( !is_npc() ) {
        update_stamina( to_turns<int>( to - from ) );
    }
    if( can_recover_oxygen() && oxygen < get_oxygen_max() ) {
        oxygen += std::max( static_cast<int>( to_turns<int>( to - from ) * get_stamina() * 5 * get_modifier(
                character_modifier_stamina_recovery_breathing_mod )  / get_stamina_max() ), 1 );
        oxygen = std::min( oxygen, get_oxygen_max() );
    }
    update_stomach( from, to );
    recalculate_enchantment_cache();
    if( ticks_between( from, to, 3_minutes ) > 0 ) {
        magic->update_mana( *this, to_turns<float>( 3_minutes ) );
    }
    const int five_mins = ticks_between( from, to, 5_minutes );
    if( five_mins > 0 ) {
        float sleepiness_mod = enchantment_cache->modify_value( enchant_vals::mod::SLEEPINESS, 1 );
        float sleepiness_regen_mod = enchantment_cache->modify_value( enchant_vals::mod::SLEEPINESS_REGEN,
                                     1 );
        activity_history.try_reduce_weariness( base_bmr(), sleepiness_mod, sleepiness_regen_mod );

        check_needs_extremes();
        update_needs( five_mins );
        regen( five_mins );


        mend( five_mins * to_turns<int>( 5_minutes ) );
        activity_history.reset_activity_level();
    }
    bool was_sleeping = get_value( "was_sleeping" ).str() == "true";
    if( in_sleep_state() && was_sleeping ) {
        needs_rates tmp_rates;
        calc_sleep_recovery_rate( tmp_rates );
        const int sleepiness_regen_rate = tmp_rates.recovery;
        const time_duration effective_time_slept = ( to - from ) * sleepiness_regen_rate;
        mod_daily_sleep( effective_time_slept );
        mod_continuous_sleep( effective_time_slept );
    }
    if( was_sleeping && !in_sleep_state() ) {
        if( get_continuous_sleep() >= 6_hours ) {
            set_value( "sleep_health_mult", 2 );
        }
        reset_continuous_sleep();
    }
    if( calendar::once_every( 12_hours ) ) {
        const int sleep_health_mult = get_value( "sleep_health_mult" ).dbl();
        mod_daily_health( sleep_health_mult * to_hours<int>( get_daily_sleep() ), 10 );
        set_value( "sleep_health_mult", 1 );
    }
    if( calendar::once_every( 1_days ) ) {
        reset_daily_sleep();
    }
    set_value( "was_sleeping", in_sleep_state() ? "true" : "false" );

    activity_history.new_turn( in_sleep_state() );


    if( calendar::once_every( 1_days ) ) {

        if( get_value( "got_to_half_stam" ).is_empty() ) {
            mod_daily_health( -4, -200 );
        } else {
            remove_value( "got_to_half_stam" );
        }

        set_value( "quarter_stam_counter", 0 );

        int cardio_accumultor = get_cardio_acc();
        if( cardio_accumultor > 0 ) {
            mod_daily_health( 1, 200 );
            if( cardio_accumultor >= 10 ) {
                mod_daily_health( 1, 200 );
            }
        }
        if( cardio_accumultor < 0 ) {
            mod_daily_health( -1, -200 );
            if( cardio_accumultor <= -10 ) {
                mod_daily_health( -1, -200 );
            }
        }
        if( cardio_accumultor >= get_cardio_acc_base() ) {
            mod_daily_health( 2, 200 );
        }
    }

    for( const auto &v : vitamin::all() ) {
        const time_duration rate = vitamin_rate( v.first );


        if( v.first == vitamin_blood && has_effect( effect_hypovolemia ) && get_thirst() > 240 ) {
            continue;
        }

        if( rate > 0_turns ) {
            int qty = ticks_between( from, to, rate );
            if( qty > 0 ) {
                vitamin_mod( v.first, 0 - qty );
            }

        } else if( rate < 0_turns ) {

            int qty = ticks_between( from, to, -rate );
            if( qty > 0 ) {
                vitamin_mod( v.first, qty );
            }
        }
        if( calendar::once_every( 24_hours ) && v.first->type() == vitamin_type::VITAMIN ) {
            const int &vit_quantity = get_daily_vitamin( v.first, true );
            const int RDA = vitamin_RDA( v.first, vit_quantity );
            if( RDA >= 50 ) {
                mod_daily_health( 1, 200 );
            }
            if( RDA >= 90 ) {
                mod_daily_health( 1, 200 );
            }


            reset_daily_vitamin( v.first );
        }
    }

    const int thirty_mins = ticks_between( from, to, 30_minutes );
    if( thirty_mins > 0 ) {
        update_health();
    }

    if( calendar::once_every( 1_minutes ) ) {
        update_bloodvol_index();
        update_heartrate_index();
        update_circulation();
    }

    if( is_avatar() && ticks_between( from, to, 24_hours ) > 0 ) {
        as_avatar()->update_cardio_acc();
        as_avatar()->advance_daily_calories();
    }

    if( calendar::once_every( 24_hours ) ) {
        do_skill_rust();
    }
}



































void Character::update_bodytemp()
{
    if( has_trait( trait_DEBUG_NOTEMP ) ) {
        set_all_parts_temp_conv( BODYTEMP_NORM );
        set_all_parts_temp_cur( BODYTEMP_NORM );
        return;
    }
    if( has_flag( json_flag_CANNOT_CHANGE_TEMPERATURE ) ) {
        return;
    }
    weather_manager &weather_man = get_weather();

    const units::temperature player_local_temp = weather_man.get_temperature( pos_bub() );
    const w_point weather = *weather_man.weather_precise;
    int vehwindspeed = 0;
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( pos_bub() );
    if( vp ) {
        vehwindspeed = std::abs( vp->vehicle().velocity / 100 );
    }
    const oter_id &cur_om_ter = overmap_buffer.ter( pos_abs_omt() );
    bool sheltered = g->is_sheltered( pos_bub() );
    const int windpower = get_local_windpower( weather_man.windspeed + vehwindspeed, cur_om_ter,
                          pos_abs(), weather_man.winddirection, sheltered );

    const bool has_bark = has_flag( json_flag_BARKY );
    const bool has_sleep = has_effect( effect_sleep );
    const bool has_sleep_state = has_sleep || in_sleep_state();
    const bool heat_immune = has_flag( json_flag_HEAT_IMMUNE );
    const bool has_heatsink = has_flag( json_flag_HEATSINK ) || heat_immune;
    const bool has_common_cold = has_effect( effect_common_cold );
    const std::pair<int, int> climate_control = climate_control_strength();
    const int climate_control_heat = climate_control.first;
    const int climate_control_chill = climate_control.second;
    const bool use_floor_warmth = can_use_floor_warmth();
    const furn_id &furn_at_pos = here.furn( pos_bub() );
    const std::optional<vpart_reference> boardable = vp.part_with_feature( "BOARDABLE", true );


    const units::temperature ambient_norm = has_sleep ? 31_C : 19_C;








    const float met_rate = metabolic_rate();
    const units::temperature_delta hunger_warmth = 4_C_delta * std::min( met_rate, 1.0f ) - 4_C_delta;

    const units::temperature_delta metabolism_warmth = std::max( 0.0f, met_rate - 1.0f ) * 2_C_delta;


    const float scaled_sleepiness = clamp( get_sleepiness(), 0,
                                           900 ) / static_cast<float>( sleepiness_levels::EXHAUSTED );
    const units::temperature_delta sleepiness_warmth = has_sleep ? 0_C_delta : -1.725_C_delta *
            scaled_sleepiness;


    const float scaled_sun_irradiance = incident_sun_irradiance( get_weather().weather_id,
                                        calendar::turn ) / max_sun_irradiance();
    const units::temperature_delta sunlight_warmth = !g->is_sheltered( pos_bub() ) ? 3_C_delta *
            scaled_sun_irradiance :
            0_C_delta;
    const int best_fire = get_best_fire( pos_bub() );

    const units::temperature_delta lying_warmth = use_floor_warmth ? floor_warmth(
                pos_bub() ) : 0_C_delta;
    const units::temperature water_temperature =
        get_weather().get_cur_weather_gen().get_water_temperature();



    const units::temperature_delta mutation_heat_low = bodytemp_modifier_traits( true );
    const units::temperature_delta mutation_heat_high = bodytemp_modifier_traits( false );

    const units::temperature_delta mutation_heat_bonus = mutation_heat_high - mutation_heat_low;

    const units::temperature_delta h_radiation = get_heat_radiation( pos_bub() );



    const int radiation_blister_count = h_radiation > 44_C_delta ? static_cast<int>( std::sqrt(
                                            units::to_fahrenheit_delta( h_radiation - 44_C_delta ) ) ) : 0;




    const units::temperature_delta bmi_heat_bonus = 0.1_C_delta * std::sqrt( std::max( 0.0f,
            ( get_bmi_fat() - 8.0f ) ) );
    std::map<bodypart_id, int> warmth_per_bp = worn.warmth( *this );
    std::map<bodypart_id, int> bonus_warmth_per_bp = bonus_item_warmth();
    std::map<bodypart_id, int> wind_res_per_bp = get_wind_resistance();


    std::map<bodypart_id, int> fire_armor_per_bp;

    for( const bodypart_id &bp : get_all_body_parts() ) {

        if( bp->has_flag( json_flag_IGNORE_TEMP ) ) {
            continue;
        }



        const double scaled_temperature = logarithmic_range( units::to_celsius( BODYTEMP_VERY_COLD ),
                                          units::to_celsius( BODYTEMP_VERY_HOT ),
                                          units::to_celsius( get_part_temp_cur( bp ) ) );

        const units::temperature_delta homeostasis_adjustment = 0.06_C_delta * ( 1.0 + scaled_temperature );
        const units::temperature_delta clothing_warmth_adjustment = homeostasis_adjustment *
                warmth_per_bp[bp];
        const units::temperature_delta clothing_warmth_adjusted_bonus = homeostasis_adjustment *
                bonus_warmth_per_bp[bp];


        const int bp_windpower = static_cast<int>( static_cast<float>( windpower ) *
                                 ( 1 - wind_res_per_bp[bp] / 100.0 ) );

        units::temperature_delta windchill = get_local_windchill( player_local_temp,
                                             get_local_humidity( weather.humidity, get_weather().weather_id, sheltered ),
                                             bp_windpower );

        static const auto is_lower = []( const bodypart_id & bp ) {
            return bp->has_flag( json_flag_LIMB_LOWER );
        };


        const units::temperature_delta bp_temp_min = bp->temp_min;
        const units::temperature_delta bp_temp_bonus = bp->temp_max - bp_temp_min;


        units::temperature_delta adjusted_temp = player_local_temp - ambient_norm;

        if( here.has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub() ) ||
            ( here.has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, pos_bub() ) && is_lower( bp ) ) ) {
            adjusted_temp = water_temperature - ambient_norm;
            windchill = 0_C_delta;
        }




        adjusted_temp /= 5.0;




        const units::temperature_delta bugged_windchill = units::from_celsius_delta(
                    units::to_fahrenheit_delta( windchill ) * 100.0 / 500.0 );



        const units::temperature temp = BODYTEMP_NORM + adjusted_temp + clothing_warmth_adjustment +
                                        bp_temp_min + bugged_windchill;
        set_part_temp_conv( bp, temp );

        mod_part_temp_conv( bp, hunger_warmth );

        mod_part_temp_conv( bp, sleepiness_warmth );

        mod_part_temp_conv( bp, mutation_heat_low );

        mod_part_temp_conv( bp, bmi_heat_bonus );


        int blister_count = has_bark ? -5 : 0;

        if( get_part_frostbite_timer( bp ) > 0 ) {
            mod_part_frostbite_timer( bp, -std::max( 5.0f, units::to_fahrenheit_delta( h_radiation ) ) );
        }
        blister_count += radiation_blister_count;

        const bool pyromania = has_trait( trait_PYROMANIA );



        if( has_heatsink ) {
            blister_count -= 20;
        }
        if( fire_armor_per_bp.empty() && blister_count > 0 ) {
            fire_armor_per_bp = get_all_armor_type( damage_heat );
        }
        if( blister_count - fire_armor_per_bp[bp] > 0 ) {
            add_effect( effect_blisters, 1_turns, bp );
            if( pyromania ) {
                add_morale( morale_pyromania_nearfire, 10, 10, 1_hours,
                            30_minutes );
                rem_morale( morale_pyromania_nofire );
            }
        } else if( pyromania && best_fire >= 1 ) {
            add_morale( morale_pyromania_nearfire, 5, 5, 30_minutes,
                        15_minutes );
            rem_morale( morale_pyromania_nofire );
        }

        mod_part_temp_conv( bp, sunlight_warmth );

        if( bp == body_part_head && has_effect( effect_flu ) ) {
            mod_part_temp_conv( bp, 3_C_delta );
        }
        if( has_common_cold ) {
            mod_part_temp_conv( bp, -1.5_C_delta );
        }

        temp_equalizer( bp, bp->connected_to );
        temp_equalizer( bp, bp->main_part );


        if( climate_control_heat || climate_control_chill ) {
            set_part_temp_conv( bp, temp_corrected_by_climate_control( get_part_temp_conv( bp ),
                                climate_control_heat, climate_control_chill ) );
        }


        units::temperature_delta bonus_fire_warmth = 0_C_delta;
        if( !has_sleep_state && best_fire > 0 ) {

            if( bp == body_part_foot_l || bp == body_part_foot_r ) {
                if( furn_at_pos != furn_str_id::NULL_ID() ) {

                    bonus_fire_warmth = best_fire * furn_at_pos.obj().bonus_fire_warmth_feet;
                } else if( boardable ) {
                    bonus_fire_warmth = best_fire * boardable->info().bonus_fire_warmth_feet;
                } else {

                    bonus_fire_warmth = best_fire * bp->fire_warmth_bonus;
                }
            } else {
                bonus_fire_warmth = best_fire * bp->fire_warmth_bonus;
            }

        }



        const double convergence_multiplier = 1 - std::exp( -0.002 );

        const units::temperature_delta comfortable_warmth = bonus_fire_warmth + lying_warmth;
        const units::temperature_delta bonus_warmth = comfortable_warmth + metabolism_warmth +
                mutation_heat_bonus + bp_temp_bonus;
        if( bonus_warmth > 0_C_delta ) {


            const units::temperature_delta delta = ( BODYTEMP_NORM - get_part_temp_conv(
                    bp ) ) / convergence_multiplier;
            units::temperature desired = BODYTEMP_NORM + delta;
            if( units::abs( BODYTEMP_NORM - desired ) < 2_C_delta ) {
                desired = BODYTEMP_NORM;
            } else if( desired > BODYTEMP_HOT ) {
                desired = BODYTEMP_HOT;
            }
            if( desired < get_part_temp_conv( bp ) ) {

            } else if( desired < get_part_temp_conv( bp ) + bonus_warmth ) {

                set_part_temp_conv( bp, desired );
            } else {

                mod_part_temp_conv( bp, bonus_warmth );
            }



            if( comfortable_warmth > 0_C_delta &&
                calendar::once_every( 1_minutes ) && get_effect_int( effect_cold ) == 0 &&
                get_effect_int( effect_hot ) == 0 &&
                get_part_temp_conv( bp ) > BODYTEMP_COLD && get_part_temp_conv( bp ) <= BODYTEMP_NORM ) {
                add_morale( morale_comfy, 1, 10, 2_minutes, 1_minutes, true );
            }
        }

        const units::temperature temp_before = get_part_temp_cur( bp );
        const units::temperature cur_temp_conv = get_part_temp_conv( bp );
        units::temperature_delta temp_difference = cur_temp_conv - temp_before;



        if( clothing_warmth_adjusted_bonus > 0_C_delta &&
            ( ( cur_temp_conv + clothing_warmth_adjusted_bonus ) < BODYTEMP_HOT ||
              get_part_temp_cur( bp ) < BODYTEMP_COLD ) ) {
            mod_part_temp_conv( bp, clothing_warmth_adjusted_bonus );
            const units::temperature new_temp_conv = get_part_temp_conv( bp );
            temp_difference = new_temp_conv - get_part_temp_cur( bp );
        }


        set_part_temp_cur( bp, temp_before + temp_difference * convergence_multiplier );

        const units::temperature temp_after = get_part_temp_cur( bp );

        if( temp_after < BODYTEMP_FREEZING ) {
            add_effect( effect_cold, 1_turns, bp, true, 3 );
        } else if( temp_after < BODYTEMP_VERY_COLD ) {
            add_effect( effect_cold, 1_turns, bp, true, 2 );
        } else if( temp_after < BODYTEMP_COLD ) {
            add_effect( effect_cold, 1_turns, bp, true, 1 );
        } else if( temp_after > BODYTEMP_SCORCHING && !heat_immune ) {
            add_effect( effect_hot, 1_turns, bp, true, 3 );

            if( bp->main_part == bp.id() && temp_after > BODYTEMP_SCORCHING + BODYTEMP_THRESHOLD ) {
                add_effect( effect_hot_speed, 1_turns, bp, true, 3 );
            } else {
                add_effect( effect_hot_speed, 1_turns, bp, true, 2 );
            }
        } else if( temp_after > BODYTEMP_VERY_HOT && !heat_immune ) {
            add_effect( effect_hot, 1_turns, bp, true, 2 );

            if( bp->main_part == bp.id() && temp_after > BODYTEMP_VERY_HOT + BODYTEMP_THRESHOLD ) {
                add_effect( effect_hot_speed, 1_turns, bp, true, 2 );
            } else {
                add_effect( effect_hot_speed, 1_turns, bp, true, 1 );
            }
        } else if( temp_after > BODYTEMP_HOT && !heat_immune ) {
            add_effect( effect_hot, 1_turns, bp, true, 1 );

            if( bp->main_part == bp.id() && temp_after > BODYTEMP_HOT + BODYTEMP_THRESHOLD ) {
                add_effect( effect_hot_speed, 1_turns, bp, true, 1 );
            } else {
                remove_effect( effect_hot_speed, bp );
            }
        } else {
            remove_effect( effect_cold, bp );
            remove_effect( effect_hot, bp );
            remove_effect( effect_hot_speed, bp );
        }

        update_frostbite( bp, bp_windpower, warmth_per_bp );


        if( temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING ) {
            //~ %s is bodypart
            add_msg_if_player( m_warning, _( "You feel your %s beginning to go numb from the cold!" ),
                               body_part_name( bp ) );
        } else if( temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD ) {
            //~ %s is bodypart
            add_msg_if_player( m_warning, _( "You feel your %s getting very cold." ),
                               body_part_name( bp ) );
        } else if( temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD ) {
            //~ %s is bodypart
            add_msg_if_player( m_warning, _( "You feel your %s getting chilly." ),
                               body_part_name( bp ) );
        } else if( temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING ) {
            //~ %s is bodypart
            add_msg_if_player( m_bad, _( "You feel your %s getting red hot from the heat!" ),
                               body_part_name( bp ) );
        } else if( temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT ) {
            //~ %s is bodypart
            add_msg_if_player( m_warning, _( "You feel your %s getting very hot." ),
                               body_part_name( bp ) );
        } else if( temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT ) {
            //~ %s is bodypart
            add_msg_if_player( m_warning, _( "You feel your %s getting warm." ),
                               body_part_name( bp ) );
        }








        if( in_sleep_state() && !has_effect( effect_narcosis ) ) {
            if( bp == body_part_torso && temp_after <= BODYTEMP_COLD && calendar::once_every( 1_hours ) ) {
                add_msg_if_player( m_warning, _( "You feel cold and shiver." ) );
            }
            if( temp_after <= BODYTEMP_VERY_COLD &&
                get_sleepiness() <= sleepiness_levels::DEAD_TIRED && !has_bionic( bio_sleep_shutdown ) ) {
                if( bp == body_part_torso ) {
                    add_msg_if_player( m_warning, _( "Your shivering prevents you from sleeping." ) );
                    wake_up();
                } else if( has_effect( effect_frostbite ) ) {
                    add_msg_if_player( m_warning,
                                       _( "You are too cold.  Your frostbite prevents you from sleeping." ) );
                    wake_up();
                }
            }
        }

        const units::temperature conv_temp = get_part_temp_conv( bp );


        if( conv_temp <= BODYTEMP_COLD && windchill < units::from_fahrenheit_delta( -10 ) &&
            one_in( 200 ) ) {
            add_msg_if_player( m_bad, _( "The wind is making your %s feel quite cold." ),
                               body_part_name( bp ) );
        } else if( conv_temp <= BODYTEMP_COLD && windchill < units::from_fahrenheit_delta( -20 ) &&
                   one_in( 100 ) ) {
            add_msg_if_player( m_bad,
                               _( "The wind is very strong; you should find some more wind-resistant clothing for your %s." ),
                               body_part_name( bp ) );
        } else if( conv_temp <= BODYTEMP_COLD && windchill < units::from_fahrenheit_delta( -30 ) &&
                   one_in( 50 ) ) {
            add_msg_if_player( m_bad,
                               _( "Your clothing is not providing enough protection from the wind for your %s!" ),
                               body_part_name( bp ) );
        }
    }
}

void Character::update_frostbite( const bodypart_id &bp, const int FBwindPower,
                                  const std::map<bodypart_id, int> &warmth_per_bp )
{



























    const float player_local_temp = units::to_fahrenheit( get_weather().get_temperature( pos_bub() ) );
    const units::temperature temp_after = get_part_temp_cur( bp );

    if( bp == body_part_mouth || bp == body_part_foot_r ||
        bp == body_part_foot_l || bp == body_part_hand_r || bp == body_part_hand_l ) {


        int wetness_percentage = 100 * get_part_wetness_percentage( bp );


        double adjusted_warmth = warmth_per_bp.at( bp ) - wetness_percentage;
        int Ftemperature = static_cast<int>( player_local_temp + 0.2 * adjusted_warmth );

        int intense = get_effect_int( effect_frostbite, bp );



        if( temp_after < BODYTEMP_COLD && ( ( Ftemperature < 30 && Ftemperature >= 10 ) ||
                                            ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                                              -4 * Ftemperature + 3 * FBwindPower - 20 >= 0 ) ) ) {
            if( get_part_frostbite_timer( bp ) < 2000 ) {
                mod_part_frostbite_timer( bp, 3 );
            }
            if( one_in( 100 ) && !has_effect( effect_frostbite, bp.id() ) ) {
                add_msg_if_player( m_warning, _( "Your %s will be frostnipped in the next few hours." ),
                                   body_part_name( bp ) );
            }

        } else if( temp_after < BODYTEMP_COLD &&
                   ( ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower < 20 &&
                       -4 * Ftemperature + 3 * FBwindPower - 20 < 0 ) ||
                     ( Ftemperature < 10 && Ftemperature >= -5 && FBwindPower >= 20 ) ||
                     ( Ftemperature < -5 && FBwindPower < 10 ) ||
                     ( Ftemperature < -5 && FBwindPower >= 10 &&
                       -4 * Ftemperature + 3 * FBwindPower - 170 >= 0 ) ) ) {
            mod_part_frostbite_timer( bp, 8 );
            if( one_in( 100 ) && intense < 2 ) {
                add_msg_if_player( m_warning, _( "Your %s will be frostbitten within the hour!" ),
                                   body_part_name( bp ) );
            }

        } else if( temp_after < BODYTEMP_COLD &&
                   ( ( Ftemperature < -5 && FBwindPower >= 10 &&
                       -4 * Ftemperature + 3 * FBwindPower - 170 < 0 ) ||
                     ( Ftemperature < -35 && FBwindPower >= 10 ) ) ) {
            mod_part_frostbite_timer( bp, 72 );
            if( one_in( 100 ) && intense < 2 ) {
                add_msg_if_player( m_warning, _( "Your %s will be frostbitten any minute now!" ),
                                   body_part_name( bp ) );
            }

        } else {
            mod_part_frostbite_timer( bp, -3 );
        }

        int frostbite_timer = get_part_frostbite_timer( bp );

        if( frostbite_timer < 0 ) {
            set_part_frostbite_timer( bp, 0 );
        } else if( frostbite_timer > 4200 ) {

            set_part_frostbite_timer( bp, 4200 );
        }
        frostbite_timer = get_part_frostbite_timer( bp );

        if( frostbite_timer >= 3600 ) {
            add_effect( effect_frostbite, 1_turns, bp, true, 2 );
            remove_effect( effect_frostbite_recovery, bp );

        } else if( frostbite_timer >= 1800 ) {
            if( intense == 2 ) {
                add_effect( effect_frostbite_recovery, 1_turns, bp, true );
            }
            add_effect( effect_frostbite, 1_turns, bp, true, 1 );

        } else if( frostbite_timer == 0 ) {
            remove_effect( effect_frostbite, bp );
            remove_effect( effect_frostbite_recovery, bp );
        }
    }
}

void Character::update_stomach( const time_point &from, const time_point &to )
{
    const needs_rates rates = calc_needs_rates();

    const bool debug_ls = has_trait( trait_DEBUG_LS );
    const bool foodless = debug_ls || !needs_food();
    const bool no_thirst = has_flag( json_flag_NO_THIRST );
    const bool mycus = has_trait( trait_M_DEPENDENT );
    const float kcal_per_time = get_bmr() / ( 12.0f * 24.0f );
    const int five_mins = ticks_between( from, to, 5_minutes );
    const int half_hours = ticks_between( from, to, 30_minutes );
    const units::volume stomach_capacity = stomach.capacity( *this );

    if( five_mins > 0 ) {

        food_summary digested_to_guts = stomach.digest( *this, rates, five_mins, half_hours );

        food_summary digested_to_body = guts.digest( *this, rates, five_mins, half_hours );

        mod_thirst( -units::to_milliliter<int>( digested_to_guts.water ) / 5 );


        for( const auto &vitamin : digested_to_guts.nutr.vitamins() ) {

            std::map<vitamin_id, int> drug_vitamins;
            const vitamin_type &vitamin_type = vitamin.first->type();
            if( vitamin_type == vitamin_type::DRUG ) {
                drug_vitamins[vitamin.first] = vitamin.second;
            }
            vitamins_mod( effect_vitamin_mod( drug_vitamins ) );
        }

        guts.ingest( digested_to_guts );

        mod_stored_kcal( digested_to_body.nutr.kcal() );
        vitamins_mod( effect_vitamin_mod( digested_to_body.nutr.vitamins() ) );
        log_activity_level( activity_history.average_activity() );

        if( !foodless && rates.hunger > 0.0f ) {
            mod_hunger( roll_remainder( rates.hunger * five_mins ) );


            mod_stored_calories( -std::floor( five_mins * kcal_per_time * 1000 ) );
        }
    }

    if( foodless ) {
        return;
    }
    if( stomach.time_since_ate() > 10_minutes ) {
        if( stomach.contains() >= stomach_capacity && get_hunger() > -61 ) {

            set_hunger( -61 );
        } else if( stomach.contains() >= stomach_capacity / 2 && get_hunger() > -21 ) {

            set_hunger( -21 );
        } else if( stomach.contains() >= stomach_capacity / 8 && get_hunger() > -1 ) {

            set_hunger( -1 );
        } else if( stomach.contains() == 0_ml ) {
            if( guts.get_calories() == 0 && get_stored_kcal() < get_healthy_kcal() && get_hunger() < 300 ) {

                set_hunger( 300 );
            } else if( get_hunger() < 100 && ( ( guts.get_calories() == 0 &&
                                                 get_stored_kcal() >= get_healthy_kcal() ) || get_stored_kcal() < get_healthy_kcal() ) ) {
                set_hunger( 100 );
            } else if( get_hunger() < 0 ) {
                set_hunger( 0 );
            }
        }
    } else



    {
        if( stomach.contains() >= stomach_capacity && get_hunger() > -61 ) {

            set_hunger( -61 );
        } else if( stomach.contains() >= stomach_capacity * 3 / 4 && get_hunger() > -21 ) {

            set_hunger( -21 );
        } else if( stomach.contains() >= stomach_capacity / 2 && get_hunger() > -1 ) {

            set_hunger( -1 );
        } else if( stomach.contains() > 0_ml && get_kcal_percent() > 0.95 ) {

            set_hunger( 0 );
        }
    }

    if( !foodless && rates.thirst > 0.0f ) {
        mod_thirst( roll_remainder( rates.thirst * five_mins ) );
    }


    if( mycus || no_thirst ) {
        set_thirst( 0 );
    }

    const bool calorie_deficit = has_calorie_deficit();
    const units::volume contains = stomach.contains();
    const units::volume cap = stomach.capacity( *this );

    efftype_id hunger_effect;

    const bool just_ate = stomach.time_since_ate() < 15_minutes;

    const bool recently_ate = stomach.time_since_ate() < 3_hours;

    if( calorie_deficit ) {






        if( stomach.would_be_engorged_with( *this, 0_ml, calorie_deficit ) ) {
            hunger_effect = effect_hunger_engorged;
        } else if( stomach.would_be_full_with( *this, 0_ml, calorie_deficit ) ) {
            hunger_effect = effect_hunger_full;
        } else if( just_ate && contains > cap / 2 ) {
            hunger_effect = effect_hunger_satisfied;
        } else if( just_ate ) {
            hunger_effect = effect_hunger_hungry;
        } else if( recently_ate ) {
            hunger_effect = effect_hunger_very_hungry;
        } else if( get_bmi_fat() < character_weight_category::underweight ) {
            hunger_effect = effect_hunger_near_starving;
        } else if( get_bmi_fat() < character_weight_category::emaciated ) {
            hunger_effect = effect_hunger_starving;
        } else {
            hunger_effect = effect_hunger_famished;
        }
    } else {







        if( stomach.would_be_engorged_with( *this, 0_ml, calorie_deficit ) ) {
            hunger_effect = effect_hunger_engorged;
        } else if( stomach.would_be_full_with( *this, 0_ml, calorie_deficit ) ) {
            hunger_effect = effect_hunger_full;
        } else if( recently_ate && contains >= cap * 3 / 8 ) {
            hunger_effect = effect_hunger_satisfied;
        } else if( recently_ate || contains > 0_ml ) {
            hunger_effect = effect_hunger_blank;
        } else if( get_bmi_fat() > character_weight_category::overweight ) {
            hunger_effect = effect_hunger_hungry;
        } else {
            hunger_effect = effect_hunger_very_hungry;
        }
    }
    if( has_effect_with_flag( json_flag_HUNGER_DISRUPTION ) ) {
        hunger_effect = effect_hunger_blank;
    }
    if( !has_effect( hunger_effect ) ) {
        remove_effect( effect_hunger_engorged );
        remove_effect( effect_hunger_full );
        remove_effect( effect_hunger_satisfied );
        remove_effect( effect_hunger_hungry );
        remove_effect( effect_hunger_very_hungry );
        remove_effect( effect_hunger_near_starving );
        remove_effect( effect_hunger_starving );
        remove_effect( effect_hunger_famished );
        remove_effect( effect_hunger_blank );
        add_effect( hunger_effect, 24_hours, true );
    }
}

bodypart_id Character::body_window( const std::string &menu_header,
                                    bool show_all, bool precise,
                                    int normal_bonus, int, int,
                                    int bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const
{



    struct healable_bp {
        mutable bool allowed;
        bodypart_id bp;
        std::string name;
        int bonus;
    };

    std::vector<healable_bp> parts;
    for( const bodypart_id &part : this->get_all_body_parts( get_body_part_flags::only_main ) ) {

        parts.push_back( { false, part, part->name.translated(), normal_bonus } );
    }

    int max_bp_name_len = 0;
    for( const healable_bp &e : parts ) {
        max_bp_name_len = std::max( max_bp_name_len, utf8_width( e.name ) );
    }

    uilist bmenu;
    bmenu.desc_enabled = true;
    bmenu.text = menu_header;

    bmenu.hilight_disabled = true;
    bool is_valid_choice = false;
    int default_selection_idx = 0;
    int default_selection_treatment_rank = 0;



    bool no_feeling = is_avatar() && has_flag( json_flag_PAIN_IMMUNE );

    for( size_t i = 0; i < parts.size(); i++ ) {
        const healable_bp &e = parts[i];
        const bodypart_id &bp = e.bp;
        const int maximal_hp = get_part_hp_max( bp );
        const int current_hp = get_part_hp_cur( bp );
        const float cur_hp_pcnt = current_hp / static_cast<float>( maximal_hp );


        const nc_color state_col = display::limb_color( *this, bp, bleed > 0, bite > 0.0f, infect > 0.0f );
        const bool has_curable_effect = state_col != c_light_gray;

        const nc_color all_state_col = display::limb_color( *this, bp, true, true, true );

        const bool limb_is_broken = is_limb_broken( bp );
        const bool limb_is_mending = worn_with_flag( flag_SPLINT, bp );





        int treatment_rank = 0;

        if( show_all || has_curable_effect ) { // NOLINT(bugprone-branch-clone)
            e.allowed = true;
        } else if( limb_is_broken ) { // NOLINT(bugprone-branch-clone)
            e.allowed = false;
        } else if( current_hp < maximal_hp && ( e.bonus != 0 || bandage_power > 0.0f  ||
                                                disinfectant_power > 0.0f ) ) {
            e.allowed = true;
        } else {
            e.allowed = false;
        }

        std::string msg;
        std::string desc;
        bool bleeding = has_effect( effect_bleed, bp.id() );
        bool bitten = has_effect( effect_bite, bp.id() );
        bool infected = has_effect( effect_infected, bp.id() );
        bool bandaged = has_effect( effect_bandaged, bp.id() );
        const int b_power = get_effect_int( effect_bandaged, bp );
        const int d_power = get_effect_int( effect_disinfected, bp );
        int new_b_power = static_cast<int>( std::floor( bandage_power ) );
        if( bandaged ) {
            const effect &eff = get_effect( effect_bandaged, bp );
            if( new_b_power > eff.get_max_intensity() ) {
                new_b_power = eff.get_max_intensity();
            }

        }
        int new_d_power = static_cast<int>( std::floor( disinfectant_power ) );

        const auto &aligned_name = std::string( max_bp_name_len - utf8_width( e.name ), ' ' ) + e.name;
        std::string hp_str;
        if( limb_is_mending ) {
            desc += colorize( _( "It is broken, but has been set, and just needs time to heal." ),
                              c_blue ) + "\n";
            if( no_feeling ) {
                hp_str = colorize( "==%==", c_blue );
            } else {
                const effect &eff = get_effect( effect_mending, bp );
                const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

                const int num = mend_perc / 20;
                hp_str = colorize( std::string( num, '#' ) + std::string( 5 - num, '=' ), c_blue );
                if( precise ) {
                    hp_str = string_format( "%s %3d%%", hp_str, mend_perc );
                }
            }
        } else if( limb_is_broken ) {
            desc += colorize( _( "It is broken.  It needs a splint or surgical attention." ), c_red ) + "\n";
            hp_str = "==%==";
        } else if( no_feeling ) {
            if( cur_hp_pcnt < 0.125f ) {
                hp_str = colorize( _( "Very Bad" ), c_red );
            } else if( cur_hp_pcnt < 0.375f ) {
                hp_str = colorize( _( "Bad" ), c_light_red );
            } else if( cur_hp_pcnt < 0.625f ) {
                hp_str = colorize( _( "So-so" ), c_yellow );
            } else if( cur_hp_pcnt < 0.875f ) {
                hp_str = colorize( _( "Okay" ), c_light_green );
            } else {
                hp_str = colorize( _( "Good" ), c_green );
            }
        } else {
            std::pair<std::string, nc_color> h_bar = get_hp_bar( current_hp, maximal_hp, false );
            hp_str = colorize( h_bar.first, h_bar.second ) +
                     colorize( std::string( 5 - utf8_width( h_bar.first ), '.' ), c_white );

            if( precise ) {
                hp_str = string_format( "%s %3d/%d", hp_str, current_hp, maximal_hp );
            }
        }
        msg += colorize( aligned_name, all_state_col ) + " " + hp_str;


        if( bleeding ) {
            const int bleeding_intensity = get_effect_int( effect_bleed, bp );
            desc += string_format( _( "Bleeding: %s" ),
                                   colorize( get_effect( effect_bleed, bp ).get_speed_name(),
                                             colorize_bleeding_intensity( bleeding_intensity ) ) );
            if( bleed > 0 ) {
                int percent = static_cast<int>( bleed * 100 / bleeding_intensity );
                percent = std::min( percent, 100 );
                desc += " -> " + colorize( string_format( _( "%d %% improvement" ), percent ), c_green );
                treatment_rank += 70 + bleeding_intensity;
            }
            desc += "\n";
        }


        if( e.allowed && ( new_b_power > 0 || b_power > 0 ) ) {
            desc += string_format( _( "Bandaged: %s" ), texitify_healing_power( b_power ) );
            if( new_b_power > 0 ) {
                desc += string_format( " -> %s", texitify_healing_power( new_b_power ) );
                if( new_b_power <= b_power ) {
                    desc += _( " (no improvement)" );
                } else {
                    treatment_rank += ( 1 - cur_hp_pcnt ) * 10;
                    treatment_rank += new_b_power - b_power;
                    treatment_rank += b_power == 0 ? 10 : 0;
                }
            }
            desc += "\n";
        }


        if( e.allowed && ( d_power > 0 || new_d_power > 0 ) ) {
            desc += string_format( _( "Disinfected: %s" ), texitify_healing_power( d_power ) );
            if( new_d_power > 0 ) {
                desc += string_format( " -> %s",  texitify_healing_power( new_d_power ) );
                if( new_d_power <= d_power ) {
                    desc += _( " (no improvement)" );
                } else {
                    treatment_rank += 30 + ( 1 - cur_hp_pcnt ) * 10;
                    treatment_rank += new_d_power - d_power;
                    treatment_rank += d_power == 0 ? 10 : 0;
                }
            }
            desc += "\n";
        }


        if( bitten ) {
            desc += string_format( "%s: ", get_effect( effect_bite, bp ).get_speed_name() );
            if( bite > 0 ) {
                desc += colorize( string_format( _( "Chance to clean and disinfect: %d %%" ),
                                                 static_cast<int>( bite * 100 ) ), c_light_green );
                treatment_rank += 60 + bite * 10;
            } else {
                desc += colorize( _( "It has a deep bite wound that needs cleaning." ), c_red );
            }
            desc += "\n";
        }


        if( infected ) {
            desc += string_format( "%s: ", get_effect( effect_infected, bp ).get_speed_name() );
            if( infect > 0 ) {
                desc += colorize( string_format( _( "Chance to cure infection: %d %%" ),
                                                 static_cast<int>( infect * 100 ) ), c_light_green ) + "\n";
                treatment_rank += 60 + infect * 10;
            } else {
                desc += colorize(
                            _( "It has a deep wound that looks to have a septic infection.  Antibiotics might be required." ),
                            c_red );
            }
            desc += "\n";
        }


        if( ( !e.allowed && !limb_is_broken ) || ( show_all && current_hp == maximal_hp &&
                !limb_is_broken && !bitten && !infected && !bleeding ) ) {
            desc += colorize( _( "Healthy." ), c_green ) + "\n";
        }
        if( !e.allowed ) {
            desc += colorize( _( "You don't expect any effect from using this." ), c_yellow );
        } else {
            is_valid_choice = true;
            if( treatment_rank > default_selection_treatment_rank ) {

                default_selection_treatment_rank = treatment_rank;
                default_selection_idx = i;
            }
        }
        bmenu.addentry_desc( i, e.allowed, MENU_AUTOASSIGN, msg, desc );







        bmenu.entries.back().a11y_value = a11y::limb_health_description( *this, bp );
    }

    if( !is_valid_choice ) {
        bmenu.init();
        bmenu.desc_enabled = false;
        bmenu.text = _( "No limb would benefit from it." );
        bmenu.addentry( parts.size(), true, 'q', "%s", _( "Cancel" ) );
    } else {
        bmenu.set_selected( default_selection_idx );
    }

    bmenu.query();
    if( bmenu.ret >= 0 && static_cast<size_t>( bmenu.ret ) < parts.size() &&
        parts[bmenu.ret].allowed ) {
        return parts[bmenu.ret].bp;
    } else {
        return bodypart_str_id::NULL_ID();
    }
}

float Character::get_heartrate_index() const
{
    return heart_rate_index;
}

void Character::set_heartrate_effect_mod( int mod )
{
    heart_rate_effect_mod = mod;
}

void Character::modify_heartrate_effect_mod( int mod )
{
    heart_rate_effect_mod += mod;
}

int Character::get_heartrate_effect_mod() const
{
    return heart_rate_effect_mod;
}

void Character::update_heartrate_index()
{



    const float player_local_temp = units::to_fahrenheit( get_weather().get_temperature( pos_bub() ) );
    float temperature_modifier = 0.0f;
    if( has_flag( json_flag_COLDBLOOD ) ) {
        temperature_modifier = 0.002f;
    }
    if( has_flag( json_flag_COLDBLOOD2 ) ) {
        temperature_modifier = 0.00333f;
    }
    if( has_flag( json_flag_COLDBLOOD3 ) || has_flag( json_flag_ECTOTHERM ) ) {
        temperature_modifier = 0.005f;
    }
    const float hr_temp_mod = ( player_local_temp - 65 ) * temperature_modifier;
    const float stamina_level = static_cast<float>( get_stamina() ) / get_stamina_max();

    const float hr_stamina_mod = 1.6f * ( 1.0f - stamina_level );

    const int stim_level = get_stim();
    float hr_stim_mod = 0.0f;
    if( stim_level > 0 ) {



        hr_stim_mod = 2 - 2 / ( 1 + 0.001 * stim_level * stim_level );
    }
    float hr_nicotine_mod = 0.0f;
    if( get_effect_dur( effect_cig ) > 0_turns ) {

        if( get_effect_dur( effect_cig ) >
            10_minutes * ( addiction_level( addiction_nicotine ) + 1 ) ) {
            hr_nicotine_mod = 0.2f;
        } else {
            hr_nicotine_mod = 0.1f;
        }
    }














    const int cur_pain = get_perceived_pain();
    float hr_pain_mod = 0.0f;
    if( cur_pain > 5 ) {
        hr_pain_mod = 0.01 * ( cur_pain - 5 );
    }

    if( hr_pain_mod > 0.3f ) {
        hr_pain_mod = 0.3f;
    }

    float hr_adrenaline_mod = 0.0f;
    if( has_effect( effect_adrenaline ) ) {
        const int adrenaline_level = get_effect_int( effect_adrenaline );



        if( adrenaline_level == 1 ) {
            hr_adrenaline_mod = -0.1f;
        } else if( adrenaline_level == 2 ) {
            hr_adrenaline_mod = 0.2f;
        }
    }


    if( has_effect( effect_betablock ) ) {
        const int betablock_level = get_effect_int( effect_betablock );



        float betablock_mod = 1.0f;
        if( betablock_level == 1 ) {
            betablock_mod = 0.4f;
        } else if( betablock_level == 2 ) {
            betablock_mod = 0.2f;
        } else if( betablock_level == 3 ) {
            betablock_mod = 0.0f;
        }
        hr_adrenaline_mod *= betablock_mod;
        hr_pain_mod *= betablock_mod;
    }



    float hr_activity_mods = hr_adrenaline_mod + hr_pain_mod + hr_stamina_mod;
    if( hr_activity_mods > 0.7f ) {
        hr_activity_mods = 0.7f;
    }







    const float hr_bp_loss_mod = 0.0f;

    const int effect_mod = get_heartrate_effect_mod();

    constexpr float HR_EFFECT_INT_TO_FLOAT_MULT = 0.0001f;
    const float hr_effect_mod = effect_mod * HR_EFFECT_INT_TO_FLOAT_MULT;

    heart_rate_index = 1.0f + hr_temp_mod + hr_activity_mods + hr_stim_mod + hr_nicotine_mod
                       + hr_bp_loss_mod + hr_effect_mod;
}

float Character::get_bloodvol_index() const
{
    return blood_vol_index;
}


void Character::update_bloodvol_index()
{

    blood_vol_index = 1.0f - ( static_cast<float>( vitamin_get( vitamin_blood ) ) / static_cast<float>
                               ( vitamin_blood->min() ) );
}

float Character::get_circulation_resistance() const
{
    return circulation_resistance;
}

void Character::set_bp_effect_mod( int mod )
{
    bp_effect_mod = mod;
}

void Character::modify_bp_effect_mod( int mod )
{
    bp_effect_mod += mod;
}

int Character::get_bp_effect_mod() const
{
    return bp_effect_mod;
}

void Character::update_circulation_resistance()
{
    const int bp_effect_mod = get_bp_effect_mod();
    constexpr float BP_EFFECT_INT_TO_FLOAT_MULT = 0.0001f;
    const float bp_item_effect_mod = bp_effect_mod * BP_EFFECT_INT_TO_FLOAT_MULT;




    float bp_stance_mod = 0.0f;
    if( is_prone() ) {
        bp_stance_mod = 0.08f;
    }

    circulation_resistance = 1.0f + bp_item_effect_mod + bp_stance_mod;
}

void Character::update_circulation()
{
    update_circulation_resistance();
    circulation = get_bloodvol_index() * get_heartrate_index() * get_circulation_resistance();











}

float Character::get_respiration_rate() const
{
    return respiration_rate;
}

int Character::get_respiration_effect_mod() const
{
    return resp_rate_effect_mod;
}

void Character::modify_respiration_effect_mod( int mod )
{
    resp_rate_effect_mod += mod;
}

void Character::set_respiration_effect_mod( int mod )
{
    resp_rate_effect_mod = mod;
}

void Character::update_respiration_rate()
{
    const int effect_mod = get_respiration_effect_mod();
    constexpr float RESP_EFFECT_INT_TO_FLOAT_MULT = 0.0001f;
    respiration_rate = 1.0f + effect_mod * RESP_EFFECT_INT_TO_FLOAT_MULT;
}

bool Character::has_sub_bodypart( const sub_bodypart_id &sbp ) const
{
    bool character_has_sub_part = false;
    for( const bodypart_id &part : get_all_body_parts() ) {
        for( const sub_bodypart_id sub_part : part->sub_parts ) {
            if( sub_part == sbp ) {
                character_has_sub_part = true;
                break;
            }
        }
    }
    return character_has_sub_part;
}
