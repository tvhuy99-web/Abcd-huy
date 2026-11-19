#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "effect.h"
#include "emit.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "fire.h"
#include "fungal_effects.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "level_cache.h"
#include "map.h"
#include "map_field.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "maptile_fwd.h"
#include "material.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "scent_block.h"
#include "scent_map.h"
#include "string_formatter.h"
#include "submap.h"
#include "talker.h"
#include "teleport.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_electric( "electric" );
static const damage_type_id damage_heat( "heat" );

static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_corroding( "corroding" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_quadruped_full( "quadruped_full" );
static const efftype_id effect_quadruped_half( "quadruped_half" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_teargas( "teargas" );

static const flag_id json_flag_GAS_PROOF( "GAS_PROOF" );
static const flag_id json_flag_NO_UNLOAD( "NO_UNLOAD" );

static const furn_str_id furn_f_ash( "f_ash" );

static const itype_id itype_ash( "ash" );
static const itype_id itype_rock( "rock" );

static const json_character_flag json_flag_HEATSINK( "HEATSINK" );
static const json_character_flag json_flag_HEAT_IMMUNE( "HEAT_IMMUNE" );

static const material_id material_iflesh( "iflesh" );
static const material_id material_veggy( "veggy" );

static const species_id species_FUNGUS( "FUNGUS" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_pit( "t_pit" );

static const trait_id trait_ACIDPROOF( "ACIDPROOF" );
static const trait_id trait_GASTROPOD_FOOT( "GASTROPOD_FOOT" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN2( "M_SKIN2" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_THRESH_INSECT( "THRESH_INSECT" );
static const trait_id trait_THRESH_MARLOSS( "THRESH_MARLOSS" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );

using namespace map_field_processing;

void map::create_burnproducts( const tripoint_bub_ms &p, const item &fuel,
                               const units::mass &burned_mass )
{
    const std::map<material_id, int> &all_mats = fuel.made_of();
    if( all_mats.empty() ) {
        return;
    }
    const units::mass by_weight = burned_mass;
    const float mat_total = fuel.type->mat_portion_total == 0 ? 1 : fuel.type->mat_portion_total;
    for( const auto &mat : all_mats ) {
        for( const auto &bp : mat.first->burn_products() ) {
            itype_id id = bp.first;


            if( fuel.typeId() == id ) {
                continue;
            }
            const float eff = bp.second;

            const int n = std::floor( eff * ( by_weight / item::find_type( id )->weight ) *
                                      ( mat.second / mat_total ) );

            if( n <= 0 ) {
                continue;
            }
            spawn_item( p, id, n, 1, calendar::turn );
        }
    }
}


int map::burn_body_part( Character &you, field_entry &cur, const bodypart_id &bp, const int scale )
{
    int total_damage = 0;
    const int intensity = cur.get_field_intensity();
    const int damage = rng( 1, ( scale + intensity ) / 2 );

    if( you.get_armor_type( damage_acid, bp ) < damage ) {
        const dealt_damage_instance ddi = you.deal_damage( nullptr, bp, damage_instance( damage_acid,
                                          damage ) );
        total_damage += ddi.total_damage();
    }

    you.add_env_effect( effect_corroding, bp, 2 + intensity, time_duration::from_turns( rng( 2,
                        1 + intensity ) ), bp, false, 0 );
    return total_damage;
}

void map::process_fields()
{
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        auto &field_cache = get_cache( z ).field_cache;
        for( int x = 0; x < my_MAPSIZE; x++ ) {
            for( int y = 0; y < my_MAPSIZE; y++ ) {
                if( field_cache[ x + y * MAPSIZE ] ) {
                    submap *const current_submap = get_submap_at_grid( tripoint_rel_sm{ x, y, z } );
                    if( current_submap == nullptr ) {
                        debugmsg( "Tried to process field at (%d,%d,%d) but the submap is not loaded", x, y, z );
                        continue;
                    }
                    process_fields_in_submap( current_submap, { x, y, z } );
                    if( current_submap->field_count == 0 ) {
                        field_cache[ x + y * MAPSIZE ] = false;
                    }
                }
            }
        }
    }
}

bool ter_furn_has_flag( const ter_t &ter, const furn_t &furn, const ter_furn_flag flag )
{
    return ter.has_flag( flag ) || furn.has_flag( flag );
}

static int ter_furn_movecost( const ter_t &ter, const furn_t &furn )
{
    if( ter.movecost == 0 ) {
        return 0;
    }

    if( furn.movecost < 0 ) {
        return 0;
    }

    return ter.movecost + furn.movecost;
}


std::pair<tripoint_bub_ms, maptile> map::maptile_has_bounds( const tripoint_bub_ms &p,
        const bool bounds_checked )
{
    if( bounds_checked ) {

        return { p, maptile_at_internal( p ) };
    }

    return { p, maptile_at( p ) };
}

std::array<std::pair<tripoint_bub_ms, maptile>, 8> map::get_neighbors( const tripoint_bub_ms &p )
{


    const bool west = p.x() > 0;
    const bool north = p.y() > 0;
    const bool east = p.x() < SEEX * my_MAPSIZE - 1;
    const bool south = p.y() < SEEY * my_MAPSIZE - 1;
    return std::array< std::pair<tripoint_bub_ms, maptile>, 8 > { {
            maptile_has_bounds( p + eight_horizontal_neighbors[0], west &&north ),
            maptile_has_bounds( p + eight_horizontal_neighbors[1], north ),
            maptile_has_bounds( p + eight_horizontal_neighbors[2], east &&north ),
            maptile_has_bounds( p + eight_horizontal_neighbors[3], west ),
            maptile_has_bounds( p + eight_horizontal_neighbors[4], east ),
            maptile_has_bounds( p + eight_horizontal_neighbors[5], west &&south ),
            maptile_has_bounds( p + eight_horizontal_neighbors[6], south ),
            maptile_has_bounds( p + eight_horizontal_neighbors[7], east &&south ),
        }
    };
}

bool map::gas_can_spread_to( field_entry &cur, const maptile &dst )
{
    const field_entry *tmpfld = dst.get_field().find_field( cur.get_field_type() );

    if( tmpfld == nullptr || tmpfld->get_field_intensity() < cur.get_field_intensity() ) {
        const ter_t &ter = dst.get_ter_t();
        const furn_t &frn = dst.get_furn_t();
        return ter_furn_movecost( ter, frn ) > 0 ||
               ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_PERMEABLE );
    }
    return false;
}

void map::gas_spread_to( field_entry &cur, maptile &dst, const tripoint_bub_ms &p )
{
    const field_type_id current_type = cur.get_field_type();
    const time_duration current_age = cur.get_field_age();
    const int current_intensity = cur.get_field_intensity();
    field_entry *f = dst.find_field( current_type );

    const time_duration age_fraction = current_age / current_intensity;
    if( f != nullptr ) {
        f->set_field_intensity( f->get_field_intensity() + 1 );
        cur.set_field_intensity( current_intensity - 1 );
        f->set_field_age( f->get_field_age() + age_fraction );
        cur.set_field_age( current_age - age_fraction );

    } else if( add_field( p, current_type, 1, 0_turns ) ) {
        f = dst.find_field( current_type );
        if( f != nullptr ) {
            f->set_field_age( age_fraction );
        } else {
            debugmsg( "While spreading the gas, field was added but doesn't exist." );
        }
        cur.set_field_intensity( current_intensity - 1 );
        cur.set_field_age( current_age - age_fraction );
    }
}

void map::spread_gas( field_entry &cur, const tripoint_bub_ms &p, int percent_spread,
                      const time_duration &outdoor_age_speedup, scent_block &sblk, const oter_id &om_ter )
{
    const bool sheltered = g->is_sheltered( p );
    weather_manager &weather = get_weather();
    const int winddirection = weather.winddirection;
    const int windpower = get_local_windpower( weather.windspeed, om_ter, get_abs( p ),
                          winddirection,
                          sheltered );

    const int current_intensity = cur.get_field_intensity();
    const field_type_id ft_id = cur.get_field_type();

    const int scent_neutralize = ft_id->get_intensity_level( current_intensity -
                                 1 ).scent_neutralization;

    if( scent_neutralize > 0 ) {

        for( const tripoint_bub_ms &tmp : points_in_radius( p, 1 ) ) {
            sblk.apply_gas( tmp, scent_neutralize );
        }
    }


    if( is_outside( p ) ) {
        const time_duration current_age = cur.get_field_age();
        cur.set_field_age( current_age + outdoor_age_speedup );
    }


    if( current_intensity <= 1 || rng( 1, 100 - windpower ) > percent_spread ) {
        return;
    }



    if( p.z() > -OVERMAP_DEPTH ) {
        const tripoint_bub_ms down = p + tripoint_rel_ms::below;
        maptile down_tile = maptile_at_internal( down );
        if( gas_can_spread_to( cur, down_tile ) && valid_move( p, down, true, true ) ) {
            gas_spread_to( cur, down_tile, down );
            return;
        }
    }

    auto neighs = get_neighbors( p );
    size_t end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
    std::vector<size_t> spread;




    for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0;
         count != neighs.size();
         i = ( i + 1 ) % neighs.size(), count++ ) {
        const auto &neigh = neighs[i];
        if( gas_can_spread_to( cur, neigh.second ) ) {
            spread.push_back( i );
        }
    }

    if( !spread.empty() && one_in( spread.size() ) ) {

        if( sheltered || windpower < 5 ) {
            std::pair<tripoint_bub_ms, maptile> &n = neighs[ random_entry( spread ) ];
            gas_spread_to( cur, n.second, n.first );
        } else {
            std::vector<size_t> neighbour_vec;
            auto maptiles = get_wind_blockers( winddirection, p );

            const maptile &remove_tile = std::get<0>( maptiles );
            const maptile &remove_tile2 = std::get<1>( maptiles );
            const maptile &remove_tile3 = std::get<2>( maptiles );
            for( const size_t &i : spread ) {
                const maptile &neigh = neighs[i].second;
                if( ( neigh.pos_ != remove_tile.pos_ &&
                      neigh.pos_ != remove_tile2.pos_ &&
                      neigh.pos_ != remove_tile3.pos_ ) ||
                    x_in_y( 1, std::max( 2, windpower ) ) ) {
                    neighbour_vec.push_back( i );
                }
            }
            if( !neighbour_vec.empty() ) {
                std::pair<tripoint_bub_ms, maptile> &n = neighs[ random_entry( neighbour_vec ) ];
                gas_spread_to( cur, n.second, n.first );
            }
        }
    } else if( p.z() < OVERMAP_HEIGHT ) {
        const tripoint_bub_ms up = p + tripoint_rel_ms::above;
        maptile up_tile = maptile_at_internal( up );
        if( gas_can_spread_to( cur, up_tile ) && valid_move( p, up, true, true ) ) {
            gas_spread_to( cur, up_tile, up );
        }
    }
}

void map::furniture_terrain_emit_fields()
{
    if( calendar::once_every( 10_seconds ) ) {
        for( const tripoint_bub_ms &elem : get_furn_field_locations() ) {
            const furn_t &furn_to_emit = *furn( elem );
            for( const emit_id &e : furn_to_emit.emissions ) {
                emit_field( elem, e );
            }
        }
        for( const tripoint_bub_ms &elem : get_ter_field_locations() ) {
            const ter_t &ter_to_emit = *ter( elem );
            for( const emit_id &e : ter_to_emit.emissions ) {
                emit_field( elem, e );
            }
        }
    }
}




void map::create_hot_air( const tripoint_bub_ms &p, int intensity )
{
    field_type_id hot_air;
    switch( intensity ) {
        case 1:
            hot_air = fd_hot_air1;
            break;
        case 2:
            hot_air = fd_hot_air2;
            break;
        case 3:
            hot_air = fd_hot_air3;
            break;
        case 4:
            hot_air = fd_hot_air4;
            break;
        default:
            debugmsg( "Tried to spread hot air with intensity %d", intensity );
            return;
    }

    for( int counter = 0; counter < 5; counter++ ) {
        tripoint_bub_ms dst( p + point( rng( -1, 1 ), rng( -1, 1 ) ) );
        add_field( dst, hot_air, 1 );
    }
}





struct field_proc_data {
    scent_block &sblk;
    Character &player_character;
    const oter_id &om_ter;
    map &here;
    maptile &map_tile;
    field_type_id cur_fd_type_id;
    field_type const *cur_fd_type;
};







void map::process_fields_in_submap( submap *const current_submap,
                                    const tripoint_bub_sm &submap )
{
    const oter_id &om_ter = overmap_buffer.ter( coords::project_to<coords::omt>(
                                abs_sub + rebase_rel( submap ) ) );
    Character &player_character = get_player_character();
    scent_block sblk( submap, get_scent() );


    maptile map_tile( current_submap, point_sm_ms::zero );
    int &locx = map_tile.pos_.x();
    int &locy = map_tile.pos_.y();
    const point_bub_ms sm_offset = coords::project_to<coords::ms>( submap.xy() );

    field_proc_data pd{
        sblk,
        player_character,
        om_ter,
        *this,
        map_tile,
        fd_null,
        &( *fd_null )
    };


    for( locx = 0; locx < SEEX; locx++ ) {
        for( locy = 0; locy < SEEY; locy++ ) {


            field &curfield = current_submap->get_field( { static_cast<int>( locx ), static_cast<int>( locy ) } );



            if( !curfield.displayed_field_type() ) {
                continue;
            }


            const tripoint_bub_ms p{sm_offset + rebase_rel( map_tile.pos() ), submap.z()};

            for( auto it = curfield.begin(); it != curfield.end(); ) {

                field_entry &cur = it->second;
                const int prev_intensity = cur.is_field_alive() ? cur.get_field_intensity() : 0;

                pd.cur_fd_type_id = cur.get_field_type();
                pd.cur_fd_type = &( *pd.cur_fd_type_id );


                if( prev_intensity == 0 ) {
                    on_field_modified( p, *pd.cur_fd_type );
                    --current_submap->field_count;
                    curfield.remove_field( it++ );
                    continue;
                }


                if( cur.get_field_age() == 0_turns ) {
                    cur.do_decay();
                    if( !cur.is_field_alive() || cur.get_field_intensity() != prev_intensity ) {
                        on_field_modified( p, *pd.cur_fd_type );
                    }
                    ++it;
                    continue;
                }

                for( const FieldProcessorPtr &proc : pd.cur_fd_type->get_processors() ) {
                    proc( p, cur, pd );
                }

                cur.do_decay();
                if( !cur.is_field_alive() || cur.get_field_intensity() != prev_intensity ) {
                    on_field_modified( p, *pd.cur_fd_type );
                }
                ++it;
            }
        }
    }
    sblk.commit_modifications();
}

static void field_processor_upgrade_intensity( const tripoint_bub_ms &, field_entry &cur,
        field_proc_data & )
{

    const field_intensity_level &int_level = cur.get_intensity_level();
    if( int_level.intensity_upgrade_chance > 0 &&
        int_level.intensity_upgrade_duration > 0_turns &&
        calendar::once_every( int_level.intensity_upgrade_duration ) &&
        one_in( int_level.intensity_upgrade_chance ) ) {
        cur.mod_field_intensity( 1 );
    }
}

static void field_processor_underwater_dissipation( const tripoint_bub_ms &, field_entry &cur,
        field_proc_data &pd )
{

    if( pd.map_tile.get_ter_t().has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) ) {
        cur.mod_field_age( pd.cur_fd_type->underwater_age_speedup );
    }
}

static void field_processor_fd_acid( const tripoint_bub_ms &p, field_entry &cur,
                                     field_proc_data &pd )
{

    if( p.z() <= -OVERMAP_DEPTH ) {
        return;
    }


    tripoint_bub_ms dst = p + tripoint::below;
    if( pd.here.valid_move( p, dst, true, true ) ) {
        field_entry *acid_there = pd.here.get_field( dst, fd_acid );
        if( !acid_there ) {
            pd.here.add_field( dst, fd_acid, cur.get_field_intensity(), cur.get_field_age() );
        } else {



            const int sum_intensity = cur.get_field_intensity() + acid_there->get_field_intensity();
            const int new_intensity = std::min( 3, sum_intensity );


            const time_duration new_age = -1_minutes * ( sum_intensity - new_intensity );
            acid_there->set_field_intensity( new_intensity );
            acid_there->set_field_age( new_age );
        }


        cur.set_field_intensity( 0 );
    }


}

static void field_processor_fd_extinguisher( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{

    if( field_entry *fire_here = pd.map_tile.find_field( fd_fire ) ) {

        const int min_int = std::min( fire_here->get_field_intensity(), cur.get_field_intensity() );
        pd.here.mod_field_intensity( p, fd_fire, -min_int );
        cur.mod_field_intensity( -min_int );
    }
}

static void field_processor_apply_slime( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{

    pd.sblk.apply_slime( p, cur.get_field_intensity() * pd.cur_fd_type->apply_slime_factor );
}


void field_processor_spread_gas( const tripoint_bub_ms &p, field_entry &cur, field_proc_data &pd )
{

    pd.here.spread_gas( cur, p, pd.cur_fd_type->percent_spread, pd.cur_fd_type->outdoor_age_speedup,
                        pd.sblk, pd.om_ter );
}

static void field_processor_fd_fungal_haze( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data & )
{

    if( one_in( 10 - 2 * cur.get_field_intensity() ) ) {

        fungal_effects().spread_fungus( p );
    }
}



static void field_processor_extra_radiation( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{

    const field_intensity_level &ilevel = cur.get_intensity_level();
    if( ilevel.extra_radiation_max > 0 ) {
        int extra_radiation = rng( ilevel.extra_radiation_min, ilevel.extra_radiation_max );
        pd.here.adjust_radiation( p, extra_radiation );
    }
}

void field_processor_wandering_field( const tripoint_bub_ms &p, field_entry &cur,
                                      field_proc_data &pd )
{

    const field_type_id wandering_field_type_id = pd.cur_fd_type->wandering_field;
    for( const tripoint_bub_ms &pnt : points_in_radius( p,
            cur.get_field_intensity() - 1 ) ) {
        field &wandering_field = pd.here.get_field( pnt );
        field_entry *tmpfld = wandering_field.find_field( wandering_field_type_id );
        if( tmpfld && tmpfld->get_field_intensity() < cur.get_field_intensity() ) {
            pd.here.mod_field_intensity( pnt, wandering_field_type_id, 1 );
        } else {
            pd.here.add_field( pnt, wandering_field_type_id, cur.get_field_intensity() );
        }
    }
}

void field_processor_fd_fire_vent( const tripoint_bub_ms &p, field_entry &cur, field_proc_data &pd )
{
    if( cur.get_field_intensity() > 1 ) {
        if( one_in( 3 ) ) {
            cur.set_field_intensity( cur.get_field_intensity() - 1 );
        }
        pd.here.create_hot_air( p, cur.get_field_intensity() );
    } else {
        pd.here.add_field( p, fd_flame_burst, 3, cur.get_field_age() );
        cur.set_field_intensity( 0 );
    }
}


void field_processor_fd_flame_burst( const tripoint_bub_ms &p, field_entry &cur,
                                     field_proc_data &pd )
{
    if( cur.get_field_intensity() > 1 ) {
        cur.set_field_intensity( cur.get_field_intensity() - 1 );
        pd.here.create_hot_air( p, cur.get_field_intensity() );
    } else {
        pd.here.add_field( p, fd_fire_vent, 3, cur.get_field_age() );
        cur.set_field_intensity( 0 );
    }
}

static void field_processor_fd_electricity( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{

    int current_intensity = cur.get_field_intensity();
    if( one_in( current_intensity * 2 ) ) {
        return;
    }

    const int spread_intensity_cap = 3 + std::max( ( current_intensity - 3 ) / 2, 0 );

    std::vector<tripoint_bub_ms> grounded_tiles;
    std::vector<tripoint_bub_ms> tiles_with_creatures;
    std::vector<tripoint_bub_ms> other_tiles;

    bool valid_candidates = false;
    for( const tripoint_bub_ms &dst : points_in_radius( p, 1 ) ) {
        if( !pd.here.inbounds( dst ) ) {
            continue;
        }

        const field_type_str_id &field_type = pd.here.get_applicable_electricity_field( dst );
        if( field_entry *field = pd.here.get_field( dst, field_type ) ) {
            if( field->get_field_intensity() >= spread_intensity_cap ) {
                continue;
            }
        }

        if( pd.here.impassable( dst ) ) {
            grounded_tiles.push_back( dst );
        } else {
            if( get_creature_tracker().creature_at( dst ) ) {
                tiles_with_creatures.push_back( dst );
            } else {
                other_tiles.push_back( dst );
            }
        }
        valid_candidates = true;
    }

    if( !valid_candidates ) {
        return;
    }

    const bool here_impassable = pd.here.impassable( p );
    int grounded_weight = here_impassable ? 1 : 6;
    int creature_weight = here_impassable ? 5 : 6;
    int other_weight = here_impassable ? 0 : 1;

    std::vector<tripoint_bub_ms> *target_vector = nullptr;
    while( current_intensity > 0 ) {

        if( here_impassable && one_in( 3 ) ) {

            cur.set_field_intensity( --current_intensity );
            continue;
        }

        const int vector_choice = bucket_index_from_weight_list( std::vector<int>( {
            grounded_tiles.empty() ? 0 : grounded_weight,
            tiles_with_creatures.empty() ? 0 : creature_weight,
            other_tiles.empty() ? 0 : other_weight
        } ) );

        switch( vector_choice ) {
            default:
            case 0:
                if( here_impassable && !one_in( 5 ) ) {
                    return;
                }
                target_vector = &grounded_tiles;
                break;
            case 1:
                target_vector = &tiles_with_creatures;
                break;
            case 2:
                target_vector = &other_tiles;
                break;
        }

        if( target_vector->empty() ) {
            return;
        }

        int vector_index = rng( 0, target_vector->size() - 1 );
        auto target_it = target_vector->begin() + vector_index;
        tripoint_bub_ms target_point = *target_it;

        const field_type_str_id &field_type = pd.here.get_applicable_electricity_field(
                target_point );


        if( field_entry *target_field = pd.here.get_field( target_point, field_type ) ) {
            int target_field_intensity = target_field->get_field_intensity();
            target_field->set_field_intensity( ++target_field_intensity );
            if( target_field_intensity >= spread_intensity_cap ) {
                target_vector->erase( target_it );
            }
        } else {
            pd.here.add_field( target_point, field_type, 1, cur.get_field_age() + 1_turns );
        }

        cur.set_field_intensity( --current_intensity );

        if( one_in( current_intensity * 2 ) ) {

            break;
        }
    }
}

void field_processor_monster_spawn( const tripoint_bub_ms &p, field_entry &cur,
                                    field_proc_data &pd )
{
    const field_intensity_level &int_level = cur.get_intensity_level();
    int monster_spawn_chance = int_level.monster_spawn_chance;
    int monster_spawn_count = int_level.monster_spawn_count;
    if( monster_spawn_count > 0 && monster_spawn_chance > 0 && one_in( monster_spawn_chance ) ) {
        for( ; monster_spawn_count > 0; monster_spawn_count-- ) {
            std::vector<MonsterGroupResult> spawn_details =
                MonsterGroupManager::GetResultFromGroup( int_level.monster_spawn_group, &monster_spawn_count );
            for( const MonsterGroupResult &mgr : spawn_details ) {
                if( !mgr.id ) {
                    continue;
                }
                if( const std::optional<tripoint_bub_ms> spawn_point =
                        random_point( points_in_radius( p, int_level.monster_spawn_radius ),
                [&pd]( const tripoint_bub_ms & n ) {
                return pd.here.passable_through( n );
                } ) ) {
                    const tripoint_bub_ms pt = spawn_point.value();
                    pd.here.add_spawn( mgr, pt );
                }
            }
        }
    }
}

static void field_processor_fd_push_items( const tripoint_bub_ms &p, field_entry &,
        field_proc_data &pd )
{
    map_stack items = pd.here.i_at( p );
    creature_tracker &creatures = get_creature_tracker();
    for( auto pushee = items.begin(); pushee != items.end(); ) {
        if( pushee->typeId() != itype_rock ||
            pushee->age() < 1_turns ) {
            pushee++;
        } else {
            std::vector<tripoint_bub_ms> valid;
            for( const tripoint_bub_ms &dst : points_in_radius( p, 1 ) ) {
                if( dst != p && pd.here.get_field( dst, fd_push_items ) ) {
                    valid.push_back( dst );
                }
            }
            if( !valid.empty() ) {
                item tmp = *pushee;
                tmp.set_age( 0_turns );
                pushee = items.erase( pushee );
                tripoint_bub_ms newp = random_entry( valid );
                pd.here.add_item_or_charges( newp, tmp );
                if( pd.player_character.pos_bub() == newp ) {
                    add_msg( m_bad, _( "A %s hits you!" ), tmp.tname() );
                    const bodypart_id hit = pd.player_character.get_random_body_part();
                    pd.player_character.deal_damage( nullptr, hit, damage_instance( damage_bash, 6 ) );
                    pd.player_character.check_dead_state( &pd.here );
                }

                if( npc *const n = creatures.creature_at<npc>( newp ) ) {

                    const bodypart_id hit = pd.player_character.get_random_body_part();
                    n->deal_damage( nullptr, hit, damage_instance( damage_bash, 6 ) );
                    add_msg_if_player_sees( newp, _( "A %1$s hits %2$s!" ), tmp.tname(), n->get_name() );
                    n->check_dead_state( &pd.here );
                } else if( monster *const mon = creatures.creature_at<monster>( newp ) ) {
                    mon->apply_damage( nullptr, bodypart_id( "torso" ),
                                       6 - mon->get_armor_type( damage_bash, bodypart_id( "torso" ) ) );
                    add_msg_if_player_sees( newp, _( "A %1$s hits the %2$s!" ), tmp.tname(), mon->name() );
                    mon->check_dead_state( &pd.here );
                }
            } else {
                pushee++;
            }
        }
    }
}

static void field_processor_fd_shock_vent( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{
    if( cur.get_field_intensity() > 1 ) {
        if( one_in( 5 ) ) {
            cur.set_field_intensity( cur.get_field_intensity() - 1 );
        }
    } else {
        cur.set_field_intensity( 3 );
        int num_bolts = rng( 3, 6 );
        for( int i = 0; i < num_bolts; i++ ) {
            point_rel_ms dir;
            while( dir == point_rel_ms::zero ) {
                dir = { rng( -1, 1 ), rng( -1, 1 ) };
            }
            int dist = rng( 4, 12 );
            point_bub_ms bolt = p.xy();
            for( int n = 0; n < dist; n++ ) {
                bolt += dir;
                pd.here.add_field( tripoint_bub_ms( bolt, p.z() ), fd_electricity, rng( 2, 3 ) );
                if( one_in( 4 ) ) {
                    if( dir.x() == 0 ) {
                        dir.x() = rng( 0, 1 ) * 2 - 1;
                    } else {
                        dir.x() = 0;
                    }
                }
                if( one_in( 4 ) ) {
                    if( dir.y() == 0 ) {
                        dir.y() = rng( 0, 1 ) * 2 - 1;
                    } else {
                        dir.y() = 0;
                    }
                }
            }
        }
    }
}

static void field_processor_fd_acid_vent( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{
    if( cur.get_field_intensity() > 1 ) {
        if( cur.get_field_age() >= 1_minutes ) {
            cur.set_field_intensity( cur.get_field_intensity() - 1 );
            cur.set_field_age( 0_turns );
        }
    } else {
        cur.set_field_intensity( 3 );
        for( const tripoint_bub_ms &t : points_in_radius( p, 5 ) ) {
            const field_entry *acid = pd.here.get_field( t, fd_acid );
            if( acid && acid->get_field_intensity() == 0 ) {
                int new_intensity = 3 - rl_dist( p, t ) / 2 + ( one_in( 3 ) ? 1 : 0 );
                if( new_intensity > 3 ) {
                    new_intensity = 3;
                }
                if( new_intensity > 0 ) {
                    pd.here.add_field( t, fd_acid, new_intensity );
                }
            }
        }
    }
}

void field_processor_fd_incendiary( const tripoint_bub_ms &p, field_entry &cur,
                                    field_proc_data &pd )
{

    tripoint_bub_ms dst( p + point( rng( -1, 1 ), rng( -1, 1 ) ) );

    if( pd.here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE, dst ) ||
        pd.here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, dst ) ||
        pd.here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_HARD, dst ) ) {
        pd.here.add_field( dst, fd_fire, 1 );
    }


    if( pd.here.flammable_items_at( dst ) ) {
        pd.here.add_field( dst, fd_fire, 1 );
    }

    pd.here.create_hot_air( p, cur.get_field_intensity() );
}

static void field_processor_make_rubble( const tripoint_bub_ms &p, field_entry &,
        field_proc_data &pd )
{


    pd.here.make_rubble( p );
}

static void field_processor_fd_fungicidal_gas( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{

    const ter_t &ter = pd.map_tile.get_ter_t();
    const furn_t &frn = pd.map_tile.get_furn_t();
    const int intensity = cur.get_field_intensity();
    if( ter.has_flag( ter_furn_flag::TFLAG_FUNGUS ) && one_in( 10 / intensity ) ) {
        pd.here.ter_set( p, ter_t_dirt );
    }
    if( frn.has_flag( ter_furn_flag::TFLAG_FUNGUS ) && one_in( 10 / intensity ) ) {
        pd.here.furn_set( p, furn_str_id::NULL_ID() );
    }
}

void field_processor_fd_fire( const tripoint_bub_ms &p, field_entry &cur, field_proc_data &pd )
{
    const field_type_id fd_fire = ::fd_fire;
    map &here = pd.here;
    maptile &map_tile = pd.map_tile;
    const oter_id om_ter = pd.om_ter;

    cur.set_field_age( std::max( -24_hours, cur.get_field_age() ) );

    bool sheltered = g->is_sheltered( p );
    weather_manager &weather = get_weather();
    int winddirection = weather.winddirection;
    int windpower = get_local_windpower( weather.windspeed, om_ter, get_map().get_abs( p ),
                                         winddirection,
                                         sheltered );
    const ter_t &ter = map_tile.get_ter_t();
    const furn_t &frn = map_tile.get_furn_t();


    const bool is_sealed = ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_SEALED ) &&
                           !ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_ALLOW_FIELD_EFFECT );

    int smoke = 0;
    int consumed = 0;

    time_duration time_added = 0_turns;

    const bool can_spread = !ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_FIRE_CONTAINER );


    const bool can_burn = ( ter.is_flammable() || frn.is_flammable() ) &&
                          !ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_FIRE_CONTAINER );


    if( !is_sealed && map_tile.get_item_count() > 0 ) {
        map_stack items_here = here.i_at( p );
        std::vector<item> new_content;
        for( auto it = items_here.begin(); it != items_here.end(); ) {
            if( it->will_explode_in_fire() ) {

                item copy = *it;
                it = items_here.erase( it );
                if( copy.detonate( p, new_content ) ) {

                    it = items_here.begin();
                }
            } else {
                ++it;
            }
        }

        fire_data frd( cur.get_field_intensity(), !can_spread );

        int max_consume = cur.get_field_intensity() * 2;

        for( auto fuel = items_here.begin(); fuel != items_here.end() && consumed < max_consume; ) {



            const units::mass old_weight = fuel->weight( false );
            bool destroyed = fuel->burn( frd );



            const units::mass new_weight = destroyed ? 0_gram : fuel->weight( false );
            if( old_weight != new_weight ) {
                here.create_burnproducts( p, *fuel, old_weight - new_weight );
            }

            if( destroyed ) {


                if( !fuel->has_flag( json_flag_NO_UNLOAD ) ) {
                    const std::list<item *> content_list = fuel->all_items_top();
                    for( item *it : content_list ) {
                        if( !it->is_irremovable() ) {
                            new_content.emplace_back( *it );
                        }
                    }
                }
                fuel = items_here.erase( fuel );
                consumed++;
            } else {
                ++fuel;
            }
        }

        here.spawn_items( p, new_content );
        smoke = roll_remainder( frd.smoke_produced );
        time_added = 1_turns * roll_remainder( frd.fuel_produced );
    }

    int part;

    vehicle *veh = here.veh_at_internal( p, part );
    if( veh != nullptr ) {
        veh->damage( here, part, cur.get_field_intensity() * 10, damage_heat, true );

    }
    if( can_burn ) {
        if( ter.has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) ) {

            cur.set_field_age( cur.get_field_age() + 4_minutes );
        }


        if( ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_FLAMMABLE ) ) {

            time_added += 1_turns * ( 5 - cur.get_field_intensity() );
            smoke += 2;
            smoke += static_cast<int>( windpower / 5 );
            if( cur.get_field_intensity() > 1 &&
                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                here.bash( p, 999, false, true, true );
            }

        } else if( ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_FLAMMABLE_HARD ) &&
                   one_in( 3 ) ) {

            time_added += 1_turns * ( 4 - cur.get_field_intensity() );
            smoke += 2;
            smoke += static_cast<int>( windpower / 5 );
            if( cur.get_field_intensity() > 1 &&
                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                here.bash( p, 999, false, true, true );
            }

        } else if( ter.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ) {

            time_added += 1_turns * ( 5 - cur.get_field_intensity() );
            smoke += 2;
            smoke += static_cast<int>( windpower / 5 );
            if( cur.get_field_intensity() > 1 &&
                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                here.bash( p, 999, false, true, true );
                here.spawn_item( p, itype_ash, 1, rng( 10, 1000 ) );
            }

        } else if( frn.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ) {

            time_added += 1_turns * ( 5 - cur.get_field_intensity() );
            smoke += 2;
            smoke += static_cast<int>( windpower / 5 );
            if( cur.get_field_intensity() > 1 &&
                one_in( 200 - cur.get_field_intensity() * 50 ) ) {
                here.furn_set( p, furn_f_ash );
                here.add_item_or_charges( p, item( itype_ash ) );
            }

        } else if( ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) && p.z() > -OVERMAP_DEPTH ) {

            tripoint_bub_ms dst = p + tripoint::below;
            if( here.valid_move( p, dst, true, true ) ) {
                maptile dst_tile = here.maptile_at_internal( dst );
                field_entry *fire_there = dst_tile.find_field( fd_fire );
                if( !fire_there ) {
                    here.add_field( dst, fd_fire, 1, 0_turns, true, cur.get_effect_source() );
                    cur.mod_field_intensity( -1 );
                } else {


                    int new_intensity = std::max( cur.get_field_intensity(),
                                                  fire_there->get_field_intensity() );

                    if( new_intensity < 3 &&
                        cur.get_field_intensity() == fire_there->get_field_intensity() ) {
                        new_intensity++;
                    }


                    if( fire_there->get_field_intensity() < 3 || one_in( 10 ) ) {
                        cur.set_field_intensity( cur.get_field_intensity() - 1 );
                    }
                    fire_there->set_field_intensity( new_intensity );
                }
                return;
            }
        }
    }

    if( time_added != 0_turns ) {
        cur.set_field_age( cur.get_field_age() - time_added );
    } else if( can_burn ) {



        cur.mod_field_age( 10_seconds * cur.get_field_intensity() );
    }



    auto neighs = here.get_neighbors( p );

    auto maptiles = here.get_wind_blockers( winddirection, p );
    maptile remove_tile = std::get<0>( maptiles );
    maptile remove_tile2 = std::get<1>( maptiles );
    maptile remove_tile3 = std::get<2>( maptiles );
    std::vector<size_t> neighbour_vec;
    neighbour_vec.reserve( neighs.size() );
    size_t end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );

    for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0;
         count != neighs.size();
         i = ( i + 1 ) % neighs.size(), count++ ) {
        const maptile &neigh = neighs[i].second;
        if( ( neigh.pos().x() != remove_tile.pos().x() && neigh.pos().y() != remove_tile.pos().y() ) ||
            ( neigh.pos().x() != remove_tile2.pos().x() && neigh.pos().y() != remove_tile2.pos().y() ) ||
            ( neigh.pos().x() != remove_tile3.pos().x() && neigh.pos().y() != remove_tile3.pos().y() ) ||
            x_in_y( 1, std::max( 2, windpower ) ) ) {
            neighbour_vec.push_back( i );
        }
    }

    const bool in_pit = ter.id.id() == ter_t_pit;


    int adjacent_fires = 0;


    if( can_spread ) {
        if( cur.get_field_intensity() > 1 && one_in( 3 ) ) {




            if( sheltered || windpower < 5 ) {
                end_it = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
                for( size_t i = ( end_it + 1 ) % neighs.size(), count = 0;
                     count != neighs.size() && cur.get_field_age() < 0_turns;
                     i = ( i + 1 ) % neighs.size(), count++ ) {
                    maptile &dst = neighs[i].second;
                    field_entry *dstfld = dst.find_field( fd_fire );

                    if( dstfld &&
                        ( dstfld->get_field_intensity() <= cur.get_field_intensity() ||
                          dstfld->get_field_age() > cur.get_field_age() ) &&
                        ( in_pit == ( dst.get_ter() == ter_t_pit ) ) ) {
                        if( dstfld->get_field_intensity() < 2 ) {


                            dstfld->mod_field_intensity( 1 );
                        }

                        dstfld->mod_field_age( -5_minutes );
                        cur.set_field_age( cur.get_field_age() + 5_minutes );
                    }
                    if( dstfld ) {
                        adjacent_fires++;
                    }
                }
            } else {
                end_it = static_cast<size_t>( rng( 0, neighbour_vec.size() - 1 ) );
                for( size_t i = ( end_it + 1 ) % neighbour_vec.size(), count = 0;
                     count != neighbour_vec.size() && cur.get_field_age() < 0_turns;
                     i = ( i + 1 ) % neighbour_vec.size(), count++ ) {
                    maptile &dst = neighs[neighbour_vec[i]].second;
                    field_entry *dstfld = dst.find_field( fd_fire );

                    if( dstfld &&
                        ( dstfld->get_field_intensity() <= cur.get_field_intensity() ||
                          dstfld->get_field_age() > cur.get_field_age() ) &&
                        ( in_pit == ( dst.get_ter() == ter_t_pit ) ) ) {
                        if( dstfld->get_field_intensity() < 2 ) {


                            dstfld->mod_field_intensity( 1 );
                        }

                        dstfld->set_field_age( dstfld->get_field_age() - 5_minutes );
                        cur.set_field_age( cur.get_field_age() + 5_minutes );
                    }

                    if( dstfld ) {
                        adjacent_fires++;
                    }
                }
            }
        } else if( cur.get_field_age() < 0_turns && cur.get_field_intensity() < 3 ) {






            int maximum_intensity = 1;





            if( cur.get_field_age() < -500_minutes ) {
                maximum_intensity = 3;
            } else {
                for( auto &neigh : neighs ) {
                    if( neigh.second.get_field().find_field( fd_fire ) ) {
                        adjacent_fires++;
                    }
                }
                maximum_intensity = 1 + ( adjacent_fires >= 3 ) + ( adjacent_fires >= 7 );

                if( maximum_intensity < 2 && cur.get_field_age() < -50_minutes ) {
                    maximum_intensity = 2;
                }
            }


            if( cur.get_field_intensity() < maximum_intensity && cur.get_field_age() < 0_turns ) {


                cur.set_field_intensity( cur.get_field_intensity() + 1 );
                cur.set_field_age( cur.get_field_age() + 10_minutes * cur.get_field_intensity() );
            }
        }
    }



    if( ( cur.get_field_intensity() == 3 ||
          here.ter( p ).obj().has_flag( ter_furn_flag::TFLAG_TREE ) ) && p.z() < OVERMAP_HEIGHT ) {
        const tripoint_bub_ms dst_p = p + tripoint::above;

        maptile dst = here.maptile_at_internal( dst_p );
        const ter_t &dst_ter = dst.get_ter_t();
        if( dst_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ||
            dst_ter.has_flag( ter_furn_flag::TFLAG_FLAMMABLE ) ||
            dst_ter.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ||
            dst_ter.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_HARD ) ) {
            field_entry *nearfire = dst.find_field( fd_fire );
            if( nearfire != nullptr ) {
                nearfire->mod_field_age( -2_turns );
            } else {
                here.add_field( dst_p, fd_fire, 1, 0_turns, true, cur.get_effect_source() );
            }

        }
    }


    if( sheltered || windpower < 5 ) {
        const size_t end_i = static_cast<size_t>( rng( 0, neighs.size() - 1 ) );
        for( size_t i = ( end_i + 1 ) % neighs.size(), count = 0;
             count != neighs.size();
             i = ( i + 1 ) % neighs.size(), count++ ) {
            if( one_in( cur.get_field_intensity() * 2 ) ) {

                continue;
            }

            tripoint_bub_ms &dst_p = neighs[i].first;
            maptile &dst = neighs[i].second;





            if( dst.find_field( fd_fire ) ) {

                continue;
            }

            field_entry *nearwebfld = dst.find_field( fd_web );
            int spread_chance = 25 * ( cur.get_field_intensity() - 1 );
            if( nearwebfld ) {
                spread_chance = 50 + spread_chance / 2;
            }

            const ter_t &dster = dst.get_ter_t();
            const furn_t &dsfrn = dst.get_furn_t();

            const int power = cur.get_field_intensity() + one_in( 5 );
            if( can_spread && rng( 1, 100 ) < spread_chance &&
                ( in_pit == ( dster.id.id() == ter_t_pit ) ) &&
                (
                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, ter_furn_flag::TFLAG_FLAMMABLE ) &&
                                      one_in( 2 ) ) ) ||
                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, ter_furn_flag::TFLAG_FLAMMABLE_ASH ) &&
                                      one_in( 2 ) ) ) ||
                    ( power >= 3 && ( ter_furn_has_flag( dster, dsfrn, ter_furn_flag::TFLAG_FLAMMABLE_HARD ) &&
                                      one_in( 5 ) ) ) ||
                    nearwebfld ||
                    ( one_in( 5 ) && dst.get_item_count() > 0 &&
                      here.flammable_items_at( p + eight_horizontal_neighbors[i] ) )
                ) ) {


                if( here.add_field( dst_p, fd_fire, 1, 2_minutes, true, cur.get_effect_source() ) ) {

                    cur.set_field_age( cur.get_field_age() + 1_minutes );
                }
                if( nearwebfld ) {
                    nearwebfld->set_field_intensity( 0 );
                }
            }
        }
    } else {
        const size_t end_i = static_cast<size_t>( rng( 0, neighbour_vec.size() - 1 ) );
        for( size_t i = ( end_i + 1 ) % neighbour_vec.size(), count = 0;
             count != neighbour_vec.size();
             i = ( i + 1 ) % neighbour_vec.size(), count++ ) {
            if( one_in( cur.get_field_intensity() * 2 ) ) {

                continue;
            }

            if( neighbour_vec.empty() ) {
                continue;
            }

            tripoint_bub_ms &dst_p = neighs[neighbour_vec[i]].first;
            maptile &dst = neighs[neighbour_vec[i]].second;





            if( dst.find_field( fd_fire ) ) {

                continue;
            }

            field_entry *nearwebfld = dst.find_field( fd_web );
            int spread_chance = 25 * ( cur.get_field_intensity() - 1 );
            if( nearwebfld ) {
                spread_chance = 50 + spread_chance / 2;
            }

            const ter_t &dster = dst.get_ter_t();
            const furn_t &dsfrn = dst.get_furn_t();

            const int power = cur.get_field_intensity() + one_in( 5 );
            if( can_spread && rng( 1, 100 ) < spread_chance &&
                ( in_pit == ( dster.id.id() == ter_t_pit ) ) &&
                (
                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, ter_furn_flag::TFLAG_FLAMMABLE ) &&
                                      one_in( 2 ) ) ) ||
                    ( power >= 2 && ( ter_furn_has_flag( dster, dsfrn, ter_furn_flag::TFLAG_FLAMMABLE_ASH ) &&
                                      one_in( 2 ) ) ) ||
                    ( power >= 3 && ( ter_furn_has_flag( dster, dsfrn, ter_furn_flag::TFLAG_FLAMMABLE_HARD ) &&
                                      one_in( 5 ) ) ) ||
                    nearwebfld ||
                    ( one_in( 5 ) && dst.get_item_count() > 0 &&
                      here.flammable_items_at( p + eight_horizontal_neighbors[i] ) )
                ) ) {


                if( here.add_field( dst_p, fd_fire, 1, 2_minutes, true, cur.get_effect_source() ) ) {

                    cur.set_field_age( cur.get_field_age() + 1_minutes );
                }
                if( nearwebfld ) {
                    nearwebfld->set_field_intensity( 0 );
                }
            }
        }
    }

    if( !ter_furn_has_flag( ter, frn, ter_furn_flag::TFLAG_SUPPRESS_SMOKE ) &&
        rng( 0, 100 - windpower ) <= smoke &&
        rng( 3, 35 ) < cur.get_field_intensity() * 10 ) {
        bool smoke_up = p.z() < OVERMAP_HEIGHT;
        if( smoke_up ) {
            tripoint_bub_ms up{p + tripoint::above};
            if( here.has_flag_ter( ter_furn_flag::TFLAG_NO_FLOOR, up ) ) {
                here.add_field( up, fd_smoke, rng( 1, cur.get_field_intensity() ), 0_turns, false );
            } else {

                smoke_up = false;
            }
        }

        if( !smoke_up ) {

            here.add_field( p, fd_smoke, cur.get_field_intensity(), 0_turns, false );
        }
    }




    if( adjacent_fires < 5 && rng( 0, 4 - adjacent_fires ) ) {
        here.create_hot_air( p, cur.get_field_intensity() );
    }
}

static void field_processor_fd_last_known( const tripoint_bub_ms &p, field_entry &cur,
        field_proc_data &pd )
{
    ( void )pd;
    Creature *const loc_creature = get_creature_tracker().creature_at( p, false );

    if( cur.get_field_age() > 5_seconds || ( loc_creature != nullptr && !loc_creature->is_avatar() ) ) {
        cur.set_field_intensity( 0 );
    }
}








void map::player_in_field( Character &you )
{

    field &curfield = get_field( you.pos_bub( *this ) );

    bool inside = false;


    if( you.in_vehicle ) {
        if( const optional_vpart_position vp = veh_at( you.pos_abs() ) ) {
            inside = vp->is_inside();
        }
    }




    for( auto &field_list_it : curfield ) {
        field_entry &cur = field_list_it.second;
        if( !cur.is_field_alive() ) {
            continue;
        }


        const field_type_id ft = cur.get_field_type();
        if( ft == fd_acid ) {


            if( !you.in_vehicle && !you.has_trait( trait_ACIDPROOF ) ) {
                int total_damage = 0;
                const bool on_ground = you.is_on_ground();
                if( !on_ground ) {
                    for( const bodypart_id &bp : you.get_ground_contact_bodyparts() ) {
                        total_damage += burn_body_part( you, cur, bp, 2 );
                    }
                } else {

                    for( const bodypart_id &bp : you.get_ground_contact_bodyparts( true ) ) {
                        total_damage += burn_body_part( you, cur, bp, 2 );
                    }
                    total_damage += burn_body_part( you, cur, bodypart_id( "torso" ), 2 );

                    if( ( !you.has_two_arms_lifting() && one_in( 4 ) ) || one_in( 2 ) ) {
                        total_damage += burn_body_part( you, cur, bodypart_id( "arm_l" ), 1 );
                        total_damage += burn_body_part( you, cur, bodypart_id( "arm_r" ), 1 );
                        total_damage += burn_body_part( you, cur, bodypart_id( "head" ), 1 );
                    }
                }

                if( on_ground && total_damage > 0 ) {
                    you.add_msg_player_or_npc( m_bad, _( "The acid burns your body!" ),
                                               _( "The acid burns <npcname>'s body!" ) );
                } else if( total_damage > 0 ) {
                    std::vector<bodypart_id> bps = you.get_ground_contact_bodyparts();
                    you.add_msg_player_or_npc( m_bad,
                                               string_format( _( "The acid burns your %s!" ), you.string_for_ground_contact_bodyparts( bps ) ),
                                               string_format( _( "The acid burns <npcname>'s %s!" ),
                                                       you.string_for_ground_contact_bodyparts( bps ) ) );
                } else if( on_ground ) {
                    you.add_msg_if_player( m_warning, _( "You're lying in a pool of acid!" ) );
                } else if( !you.is_immune_field( fd_acid ) ) {
                    you.add_msg_if_player( m_warning, _( "You're standing in a pool of acid!" ) );
                }

                you.check_dead_state( this );
            }
        }
        if( ft == fd_sap ) {

            if( !you.in_vehicle ) {

                mod_field_intensity( you.pos_bub( *this ), ft, -1 );
            }
        }
        if( ft == fd_sludge ) {

            if( !you.has_trait( trait_GASTROPOD_FOOT ) && ( !you.in_vehicle ) ) {
                you.add_msg_if_player( m_bad, _( "The sludge is thick and sticky.  You struggle to pull free." ) );
                you.mod_moves( -cur.get_field_intensity() * 300 );
                cur.set_field_intensity( 0 );
            }
        }
        if( ft == fd_fire ) {

            if( !you.has_flag( json_flag_HEATSINK ) && !you.has_flag( json_flag_HEAT_IMMUNE ) ) {


                int adjusted_intensity = cur.get_field_intensity();

                if( you.in_vehicle ) {
                    if( inside ) {
                        adjusted_intensity -= 2;
                    } else {
                        adjusted_intensity -= 1;
                    }
                }


                if( cur.get_field_intensity() == 1 ) {
                    continue;
                }
                if( cur.get_field_intensity() == 2 && !one_in( 10 ) ) {
                    continue;
                }

                if( adjusted_intensity >= 1 ) {

                    static const std::array<std::string, 4> player_burn_msg = { {
                            translate_marker( "You burn your legs and feet!" ),
                            translate_marker( "You're burning up!" ),
                            translate_marker( "You're set ablaze!" ),
                            translate_marker( "Your whole body is burning!" )
                        }
                    };
                    static const std::array<std::string, 4> npc_burn_msg = { {
                            translate_marker( "<npcname> burns their legs and feet!" ),
                            translate_marker( "<npcname> is burning up!" ),
                            translate_marker( "<npcname> is set ablaze!" ),
                            translate_marker( "<npcname>s whole body is burning!" )
                        }
                    };
                    static const std::array<std::string, 4> player_warn_msg = { {
                            translate_marker( "You're standing in a fire!" ),
                            translate_marker( "You're waist-deep in a fire!" ),
                            translate_marker( "You're surrounded by raging fire!" ),
                            translate_marker( "You're lying in fire!" )
                        }
                    };

                    const int burn_min = adjusted_intensity;
                    const int burn_max = 3 * adjusted_intensity + 3;
                    std::list<bodypart_id> parts_burned;
                    int msg_num = adjusted_intensity - 1;
                    if( !you.is_on_ground() ) {
                        switch( adjusted_intensity ) {
                            case 3:
                                parts_burned.emplace_back( "hand_l" );
                                parts_burned.emplace_back( "hand_r" );
                                parts_burned.emplace_back( "arm_l" );
                                parts_burned.emplace_back( "arm_r" );
                                [[fallthrough]];
                            case 2:
                                parts_burned.emplace_back( "torso" );
                                [[fallthrough]];
                            case 1:
                                parts_burned.emplace_back( "foot_l" );
                                parts_burned.emplace_back( "foot_r" );
                                parts_burned.emplace_back( "leg_l" );
                                parts_burned.emplace_back( "leg_r" );
                        }
                    } else if( you.has_effect( effect_quadruped_full ) ||  you.has_effect( effect_quadruped_half ) ) {

                        msg_num = 3;
                        const std::vector<bodypart_id> all_parts = you.get_all_body_parts();
                        parts_burned.assign( all_parts.begin(), all_parts.end() );
                    } else {

                        msg_num = 3;
                        const std::vector<bodypart_id> all_parts = you.get_all_body_parts();
                        parts_burned.assign( all_parts.begin(), all_parts.end() );
                    }

                    int total_damage = 0;
                    for( const bodypart_id &part_burned : parts_burned ) {
                        const dealt_damage_instance dealt = you.deal_damage( cur.get_causer(), part_burned,
                                                            damage_instance( damage_heat, rng( burn_min, burn_max ) ) );
                        total_damage += dealt.type_damage( damage_heat );
                    }
                    if( total_damage > 0 ) {
                        you.add_msg_player_or_npc( m_bad, _( player_burn_msg[msg_num] ), _( npc_burn_msg[msg_num] ) );
                    } else {
                        you.add_msg_if_player( m_warning, _( player_warn_msg[msg_num] ) );
                    }
                    you.check_dead_state( this );
                }
            }

        }
        if( ft == fd_tear_gas ) {

            if( ( cur.get_field_intensity() > 1 || !one_in( 3 ) ) && ( !inside || one_in( 3 ) ) ) {
                you.add_env_effect( effect_teargas, bodypart_id( "mouth" ), 5, 20_seconds );
            }
            if( cur.get_field_intensity() > 1 && ( !inside || one_in( 3 ) ) ) {
                you.add_env_effect( effect_blind, bodypart_id( "eyes" ), cur.get_field_intensity() * 2,
                                    10_seconds );
            }
        }
        if( ft == fd_fungal_haze ) {
            if( !you.has_trait( trait_M_IMMUNE ) && ( !inside || one_in( 4 ) ) ) {
                you.add_env_effect( effect_fungus, bodypart_id( "mouth" ), 4, 10_minutes, true );
                you.add_env_effect( effect_fungus, bodypart_id( "eyes" ), 4, 10_minutes, true );
            }
        }

        if( cur.get_intensity_level().extra_radiation_min > 0 ) {
            const field_intensity_level &int_level = cur.get_intensity_level();

            const float rads = rng( int_level.extra_radiation_min + 1,
                                    int_level.extra_radiation_max * ( int_level.extra_radiation_max + 1 ) );
            const bool rad_proof = !you.irradiate( rads );

            if( int_level.extra_radiation_min > 0 && !rad_proof ) {
                you.add_msg_if_player( m_bad, int_level.radiation_hurt_message.translated() );
                you.hurtall( rng( int_level.radiation_hurt_damage_min, int_level.radiation_hurt_damage_max ),
                             nullptr );
            }
        }
        if( ft == fd_flame_burst ) {

            if( !inside ) {


                if( !you.has_flag( json_flag_HEATSINK ) ) {
                    you.add_msg_player_or_npc( m_bad, _( "You're torched by flames!" ),
                                               _( "<npcname> is torched by flames!" ) );
                    you.deal_damage( nullptr, bodypart_id( "leg_l" ), damage_instance( damage_heat, rng( 2, 6 ) ) );
                    you.deal_damage( nullptr, bodypart_id( "leg_r" ), damage_instance( damage_heat, rng( 2, 6 ) ) );
                    you.deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_heat, rng( 4, 9 ) ) );
                    you.check_dead_state( this );
                } else {
                    you.add_msg_player_or_npc( _( "These flames do not burn you." ),
                                               _( "Those flames do not burn <npcname>." ) );
                }
            }
        }
        if( ft == fd_electricity ) {

            if( cur.get_field_intensity() > 0 && !you.is_elec_immune() && !you.in_vehicle ) {
                const bodypart_id &main_part = bodypart_id( "torso" );
                const int dmg = std::max( 1, rng( cur.get_field_intensity() / 2, cur.get_field_intensity() ) );
                const int main_part_damage = you.deal_damage( nullptr, main_part,
                                             damage_instance( damage_electric, dmg ) ).total_damage();

                if( main_part_damage > 0 ) {
                    for( const bodypart_id &bp :
                         you.get_all_body_parts( get_body_part_flags::only_main ) ) {
                        if( bp == main_part ) {
                            continue;
                        }

                        you.apply_damage( nullptr, bp, dmg, true );
                    }
                } else {
                    you.add_msg_player_or_npc( _( "The electric cloud doesn't affect you." ),
                                               _( "The electric cloud doesn't seem to affect <npcname>." ) );
                }
            }
        }
        if( ft == fd_fatigue ) {

            if( !you.in_vehicle ) {

                if( rng( 0, 2 ) < cur.get_field_intensity() && you.is_avatar() ) {
                    add_msg( m_bad, _( "You're violently teleported!" ) );
                    you.hurtall( cur.get_field_intensity(), nullptr );
                    teleport::teleport_creature( you );
                }
            }
        }


        if( ft == fd_shock_vent || ft == fd_acid_vent ) {
            cur.set_field_intensity( 0 );
        }
        if( ft == fd_incendiary ) {

            if( you.has_trait( trait_M_SKIN2 ) ||
                you.has_trait( trait_M_SKIN3 ) ||
                cur.get_field_intensity() == 1 ) {
                you.add_msg_player_or_npc( m_bad, _( "The incendiary burns you!" ),
                                           _( "The incendiary burns <npcname>!" ) );
                you.hurtall( rng( 1, 3 ), nullptr );
            } else {
                you.add_msg_player_or_npc( m_bad, _( "The incendiary melts into your skin!" ),
                                           _( "The incendiary melts into <npcname>s skin!" ) );
                you.add_effect( effect_onfire, 8_turns, bodypart_id( "torso" ) );
                you.hurtall( rng( 2, 6 ), nullptr );
            }
        }

        if( ft == fd_fungicidal_gas || ft == fd_insecticidal_gas ) {

            if( !inside ) {

                if( !( you.worn_with_flag( json_flag_GAS_PROOF ) &&
                       you.get_env_resist( bodypart_id( "mouth" ) ) >= 15 &&
                       you.get_env_resist( bodypart_id( "eyes" ) ) >= 15 ) ) {
                    const int intensity = cur.get_field_intensity();
                    bool inhaled = you.add_env_effect( effect_poison, bodypart_id( "mouth" ), 5,
                                                       intensity * 1_minutes );
                    if( you.has_trait( trait_THRESH_MYCUS ) || you.has_trait( trait_THRESH_MARLOSS ) ||
                        ( ft == fd_insecticidal_gas &&
                          ( you.has_trait( trait_THRESH_INSECT ) ||
                            you.has_trait( trait_THRESH_SPIDER ) ) ) ) {
                        inhaled |= you.add_env_effect( effect_badpoison, bodypart_id( "mouth" ), 5, intensity * 1_minutes );
                        you.hurtall( rng( intensity, intensity * 2 ), nullptr );
                        you.add_msg_if_player( m_bad, _( "The %s burns your skin." ), cur.name() );
                    }

                    if( inhaled ) {
                        you.add_msg_if_player( m_bad, _( "The %s makes you feel sick." ), cur.name() );
                    }
                }
            }
        }

        if( const int chance = std::get<0>( ft->npc_complain_data ) ) {
            if( you.is_npc() && chance > 0 && one_in( chance ) ) {
                const auto &npc_complain_data = ft->npc_complain_data;
                ( static_cast<npc *>( &you ) )->complain_about( std::get<1>( npc_complain_data ),
                        std::get<2>( npc_complain_data ),
                        std::get<3>( npc_complain_data ).translated() );
            }
        }
    }
}

void map::maybe_apply_field_effect( const std::vector<field_effect> &vfe, Creature &critter ) const
{

    bool in_vehicle = false;
    bool inside_vehicle = false;

    if( const optional_vpart_position vp = veh_at( critter.pos_bub() ) ; vp.has_value() ) {
        in_vehicle = true;
        if( vp->is_inside() ) {
            inside_vehicle = true;
        }
    }

    for( const field_effect &fe : vfe ) {
        if( in_vehicle && fe.immune_in_vehicle ) {
            continue;
        }
        if( inside_vehicle && fe.immune_inside_vehicle ) {
            continue;
        }
        if( !inside_vehicle && fe.immune_outside_vehicle ) {
            continue;
        }
        if( in_vehicle && !one_in( fe.chance_in_vehicle ) ) {
            continue;
        }
        if( inside_vehicle && !one_in( fe.chance_inside_vehicle ) ) {
            continue;
        }
        if( !inside_vehicle && !one_in( fe.chance_outside_vehicle ) ) {
            continue;
        }

        const effect field_fx = fe.get_effect();
        if( critter.is_immune_effect( field_fx.get_id() ) ||
            critter.check_immunity_data( fe.immunity_data ) ) {
            continue;
        }
        bool effect_added = false;
        if( fe.is_environmental ) {
            effect_added = critter.add_env_effect( fe.id, fe.bp.id(), fe.intensity,  fe.get_duration() );
        } else {
            effect_added = true;
            critter.add_effect( field_fx.get_id(), field_fx.get_duration(), field_fx.get_bp(),
                                field_fx.is_permanent(), field_fx.get_intensity() );
        }
        if( effect_added ) {
            critter.add_msg_player_or_npc( fe.env_message_type, fe.get_message(), fe.get_message_npc() );
        }

    }

}

void map::creature_in_field( Creature &critter )
{
    if( critter.is_monster() ) {
        monster_in_field( *static_cast<monster *>( &critter ) );
    } else {
        Character *you = critter.as_character();
        if( you ) {
            player_in_field( *you );
        }
    }

    field &curfield = get_field( critter.pos_bub() );
    for( auto &field_entry_it : curfield ) {
        field_entry &cur_field_entry = field_entry_it.second;
        if( !cur_field_entry.is_field_alive() ) {
            continue;
        }
        const field_type_id cur_field_id = cur_field_entry.get_field_type();

        if( !critter.is_immune_field( cur_field_id ) ) {
            maybe_apply_field_effect( cur_field_entry.get_intensity_level().field_effects, critter );
        }

        if( cur_field_id->decrease_intensity_on_contact &&
            !critter.is_immune_field( cur_field_id ) ) {
            mod_field_intensity( critter.pos_bub(), cur_field_id, -1 );
        }
    }
}

void map::monster_in_field( monster &z )
{
    if( z.digging() ) {

        return;
    }
    if( veh_at( z.pos_abs() ) ) {

        return;
    }
    field &curfield = get_field( get_bub( z.pos_abs() ) );




    for( auto &field_list_it : curfield ) {
        field_entry &cur = field_list_it.second;
        if( !cur.is_field_alive() ) {
            continue;
        }



        int dam = 0;
        const field_type_id cur_field_type = cur.get_field_type();
        if( cur_field_type == fd_acid ) {
            if( !z.flies() ) {
                const int d = rng( cur.get_field_intensity(), cur.get_field_intensity() * 3 );
                z.deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_acid, d ) );
                z.check_dead_state( this );
            }

        }
        if( cur_field_type == fd_sap ) {
            z.mod_moves( -cur.get_field_intensity() * 5 );
            mod_field_intensity( get_bub( z.pos_abs() ), cur.get_field_type(), -1 );
        }
        if( cur_field_type == fd_sludge ) {
            if( !z.digs() && !z.flies() &&
                !z.has_flag( mon_flag_SLUDGEPROOF ) ) {
                z.mod_moves( -cur.get_field_intensity() * 300 );
                cur.set_field_intensity( 0 );
            }
        }
        if( cur_field_type == fd_fire ) {

            if( z.has_flag( mon_flag_FIREPROOF ) || z.has_flag( mon_flag_FIREY ) ) {
                continue;
            }
            if( has_flag_ter_or_furn( ter_furn_flag::TFLAG_FIRE_CONTAINER, get_bub( z.pos_abs() ) ) ) {
                continue;
            }
            if( cur.get_field_intensity() == 1 ) {
                continue;
            }

            if( cur.get_field_intensity() == 2 && !one_in( 10 ) ) {
                continue;
            }


            if( z.made_of_any( Creature::cmat_flesh ) ) {
                dam += 3;
            }
            if( z.made_of( material_veggy ) ) {
                dam += 12;
            }
            if( z.made_of_any( Creature::cmat_flammable ) ) {
                dam += 20;
            }
            if( z.made_of_any( Creature::cmat_flameres ) ) {
                dam -= 20;
            }
            if( z.flies() ) {
                dam -= 15;
            }


            dam -= z.get_armor_type( damage_heat, z.get_random_body_part_of_type( bp_type::torso ) );

            dam += rng( cur.get_field_intensity(), cur.get_field_intensity() * 2 );
            const bool affected_by_fire = !z.flies() || one_in( 3 );
            if( dam > 0 && affected_by_fire ) {
                z.mod_moves( -to_moves<int>( 1_seconds ) * 0.4 );

                if( x_in_y( dam, 100 ) ) {


                    z.add_effect( effect_onfire, 1_turns * rng( dam / 2, dam * 2 ) );
                }



                cur.mod_field_age( 1_minutes * dam );
            }
        }
        if( cur_field_type == fd_smoke ) {
            if( !z.has_flag( mon_flag_NO_BREATHE ) ) {
                if( cur.get_field_intensity() == 3 ) {
                    z.mod_moves( -to_moves<int>( 1_seconds ) * rng_float( 0.1, 0.2 ) );
                }

                if( z.made_of( material_veggy ) ) {
                    z.mod_moves( -to_moves<int>( 1_seconds ) * rng_float( 0.01, cur.get_field_intensity() * 0.12 ) );
                }
            }

        }
        if( cur_field_type == fd_tear_gas ) {
            if( z.made_of_any( Creature::cmat_fleshnveg ) && !z.has_flag( mon_flag_NO_BREATHE ) ) {
                if( cur.get_field_intensity() == 3 ) {
                    z.add_effect( effect_stunned, rng( 1_minutes, 2_minutes ) );
                } else if( cur.get_field_intensity() == 2 ) {
                    z.add_effect( effect_stunned, rng( 5_turns, 10_turns ) );
                } else {
                    z.add_effect( effect_stunned, rng( 1_turns, 5_turns ) );
                }
                if( z.has_flag( mon_flag_SEES ) ) {
                    z.add_effect( effect_blind, cur.get_field_intensity() * 8_turns );
                }
            }

        }
        if( cur_field_type == fd_relax_gas ) {
            if( z.made_of_any( Creature::cmat_fleshnveg ) && !z.has_flag( mon_flag_NO_BREATHE ) ) {
                z.add_effect( effect_stunned, rng( cur.get_field_intensity() * 4_turns,
                                                   cur.get_field_intensity() * 8_turns ) );
            }
        }
        if( cur_field_type == fd_dazzling ) {
            if( z.has_flag( mon_flag_SEES ) && !z.has_flag( mon_flag_ELECTRONIC ) ) {
                z.add_effect( effect_blind, cur.get_field_intensity() * 12_turns );
                z.add_effect( effect_stunned, cur.get_field_intensity() * rng( 5_turns, 12_turns ) );
            }

        }
        if( cur_field_type == fd_toxic_gas ) {
            if( !z.has_flag( mon_flag_NO_BREATHE ) ) {
                dam += cur.get_field_intensity();
                z.mod_moves( -cur.get_field_intensity() );
            }

        }
        if( cur_field_type == fd_nuke_gas ) {
            if( !z.has_flag( mon_flag_NO_BREATHE ) ) {
                if( cur.get_field_intensity() == 3 ) {
                    z.mod_moves( -to_moves<int>( 1_seconds ) * rng_float( 0.6, 1.2 ) );
                    dam += rng( 30, 50 );
                } else if( cur.get_field_intensity() == 2 ) {
                    z.mod_moves( -to_moves<int>( 1_seconds ) * rng_float( 0.2, 0.5 ) );
                    dam += rng( 10, 25 );
                } else {
                    z.mod_moves( -to_moves<int>( 1_seconds ) * rng_float( 0.0, 0.15 ) );
                    dam += rng( 0, 12 );
                }
                if( z.made_of( material_veggy ) ) {
                    z.mod_moves( -to_moves<int>( 1_seconds ) * rng_float( cur.get_field_intensity() * 0.05,
                                 cur.get_field_intensity() * 0.12 ) );
                    dam *= cur.get_field_intensity();
                }
            }

        }
        if( cur_field_type == fd_flame_burst ) {

            if( z.has_flag( mon_flag_FIREPROOF ) || z.has_flag( mon_flag_FIREY ) ) {
                return;
            }
            if( z.made_of_any( Creature::cmat_flesh ) ) {
                dam += 3;
            }
            if( z.made_of( material_veggy ) ) {
                dam += 12;
            }
            if( z.made_of( phase_id::LIQUID ) || z.made_of_any( Creature::cmat_flammable ) ) {
                dam += 50;
            }
            if( z.made_of_any( Creature::cmat_flameres ) ) {
                dam += -25;
            }
            dam += rng( 0, 8 );
            z.mod_moves( -to_moves<int>( 1_seconds ) * 0.2 );
        }
        if( cur_field_type == fd_electricity ) {

            const int field_dmg = std::max( 1, rng( cur.get_field_intensity() / 2,
                                                    cur.get_field_intensity() ) );
            z.deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_electric, field_dmg ) );
        }
        if( cur_field_type == fd_fatigue ) {
            if( rng( 0, 2 ) < cur.get_field_intensity() ) {
                dam += cur.get_field_intensity();
                teleport::teleport_creature( z );
            }
        }
        if( cur_field_type == fd_incendiary ) {

            if( z.has_flag( mon_flag_FIREPROOF ) || z.has_flag( mon_flag_FIREY ) ) {
                return;
            }
            if( z.made_of_any( Creature::cmat_flesh ) ) {
                dam += 3;
            }
            if( z.made_of( material_veggy ) ) {
                dam += 12;
            }
            if( z.made_of( phase_id::LIQUID ) || z.made_of_any( Creature::cmat_flammable ) ) {
                dam += 20;
            }
            if( z.made_of_any( Creature::cmat_flameres ) ) {
                dam += -5;
            }

            if( cur.get_field_intensity() == 1 ) {
                dam += rng( 2, 6 );
            } else if( cur.get_field_intensity() == 2 ) {
                dam += rng( 6, 12 );
                z.mod_moves( -to_moves<int>( 1_seconds ) * 0.2 );
                if( !z.made_of( phase_id::LIQUID ) && !z.made_of_any( Creature::cmat_flameres ) ) {
                    z.add_effect( effect_onfire, rng( 8_turns, 12_turns ) );
                }
            } else if( cur.get_field_intensity() == 3 ) {
                dam += rng( 10, 20 );
                z.mod_moves( -to_moves<int>( 1_seconds ) * 0.4 );
                if( !z.made_of( phase_id::LIQUID ) && !z.made_of_any( Creature::cmat_flameres ) ) {
                    z.add_effect( effect_onfire, rng( 12_turns, 16_turns ) );
                }
            }
        }
        if( cur_field_type == fd_fungal_haze ) {
            if( !z.type->in_species( species_FUNGUS ) &&
                !z.type->has_flag( mon_flag_NO_BREATHE ) &&
                !z.make_fungus() ) {

                const int intensity = cur.get_field_intensity();
                z.mod_moves( -rng( 10 * intensity, 30 * intensity ) );
                dam += rng( 0, 10 * intensity );
            }
        }
        if( cur_field_type == fd_fungicidal_gas ) {
            if( z.type->in_species( species_FUNGUS ) ) {
                const int intensity = cur.get_field_intensity();
                z.mod_moves( -rng( 10 * intensity, 30 * intensity ) );
                dam += rng( 4, 7 * intensity );
            }
        }
        if( cur_field_type == fd_insecticidal_gas ) {
            if( z.made_of( material_iflesh ) && !z.has_flag( mon_flag_INSECTICIDEPROOF ) ) {
                const int intensity = cur.get_field_intensity();
                z.mod_moves( -rng( 10 * intensity, 30 * intensity ) );
                dam += rng( 4, 7 * intensity );
            }
        }


        if( dam > 0 ) {
            z.apply_damage( cur.get_causer(), z.get_random_body_part_of_type( bp_type::torso ), dam, true );
            z.check_dead_state( this );
        }
    }
}

std::tuple<maptile, maptile, maptile> map::get_wind_blockers( const int &winddirection,
        const tripoint_bub_ms &pos )
{
    static const std::array<std::pair<int, std::tuple< point_rel_ms, point_rel_ms, point_rel_ms >>, 9>
    outputs = { {
            { 330, std::make_tuple( point_rel_ms::east, point_rel_ms::north_east, point_rel_ms::south_east ) },
            { 301, std::make_tuple( point_rel_ms::south_east, point_rel_ms::east, point_rel_ms::south ) },
            { 240, std::make_tuple( point_rel_ms::south, point_rel_ms::south_west, point_rel_ms::south_east ) },
            { 211, std::make_tuple( point_rel_ms::south_west, point_rel_ms::west, point_rel_ms::south ) },
            { 150, std::make_tuple( point_rel_ms::west, point_rel_ms::north_west, point_rel_ms::south_west ) },
            { 121, std::make_tuple( point_rel_ms::north_west, point_rel_ms::north, point_rel_ms::west ) },
            { 60, std::make_tuple( point_rel_ms::north, point_rel_ms::north_west, point_rel_ms::north_east ) },
            { 31, std::make_tuple( point_rel_ms::north_east, point_rel_ms::east, point_rel_ms::north ) },
            { 0, std::make_tuple( point_rel_ms::east, point_rel_ms::north_east, point_rel_ms::south_east ) }
        }
    };

    tripoint_bub_ms removepoint;
    tripoint_bub_ms removepoint2;
    tripoint_bub_ms removepoint3;
    for( const std::pair<int, std::tuple< point_rel_ms, point_rel_ms, point_rel_ms >> &val : outputs ) {
        if( winddirection >= val.first ) {
            removepoint = pos + std::get<0>( val.second );
            removepoint2 = pos + std::get<1>( val.second );
            removepoint3 = pos + std::get<2>( val.second );
            break;
        }
    }

    const maptile remove_tile = maptile_at( removepoint );
    const maptile remove_tile2 = maptile_at( removepoint2 );
    const maptile remove_tile3 = maptile_at( removepoint3 );
    return std::make_tuple( remove_tile, remove_tile2, remove_tile3 );
}

void map::emit_field( const tripoint_bub_ms &pos, const emit_id &src, float mul )
{
    if( !src.is_valid() ) {
        return;
    }

    dialogue d( get_talker_for( get_avatar() ), nullptr );
    const float chance = src->chance( d ) * mul;
    if( x_in_y( chance, 100 ) ) {
        const int qty = chance > 100.0f ? roll_remainder( src->qty( d ) * chance / 100.0f ) : src->qty( d );
        propagate_field( pos, src->field( d ), qty, src->intensity( d ) );
    }
}

void map::propagate_field( const tripoint_bub_ms &center, const field_type_id &type, int amount,
                           int max_intensity )
{
    using gas_blast = std::pair<float, tripoint_bub_ms>;
    std::priority_queue<gas_blast, std::vector<gas_blast>, pair_greater_cmp_first> open;
    std::set<tripoint_bub_ms> closed;
    open.emplace( 0.0f, center );

    const bool not_gas = type.obj().phase != phase_id::GAS;

    while( amount > 0 && !open.empty() ) {
        if( closed.count( open.top().second ) ) {
            open.pop();
            continue;
        }


        std::list<gas_blast> gas_front;
        gas_front.push_back( open.top() );
        const int cur_intensity = get_field_intensity( open.top().second, type );
        open.pop();
        while( !open.empty() && get_field_intensity( open.top().second, type ) == cur_intensity ) {
            if( closed.count( open.top().second ) == 0 ) {
                gas_front.push_back( open.top() );
            }

            open.pop();
        }

        int increment = std::max<int>( 1, amount / gas_front.size() );

        while( !gas_front.empty() ) {
            gas_blast gp = random_entry_removed( gas_front );
            closed.insert( gp.second );
            const int cur_intensity = get_field_intensity( gp.second, type );
            if( cur_intensity < max_intensity ) {
                const int bonus = std::min( max_intensity - cur_intensity, increment );
                mod_field_intensity( gp.second, type, bonus );
                amount -= bonus;
            } else {
                amount--;
            }

            if( amount <= 0 ) {
                return;
            }

            static const std::array<int, 8> x_offset = { { -1, 1,  0, 0,  1, -1, -1, 1  } };
            static const std::array<int, 8> y_offset = { {  0, 0, -1, 1, -1,  1, -1, 1  } };
            for( size_t i = 0; i < 8; i++ ) {
                tripoint_bub_ms pt = gp.second + point( x_offset[i], y_offset[i] );
                if( closed.count( pt ) > 0 ) {
                    continue;
                }

                if( impassable( pt ) && ( not_gas || !has_flag( ter_furn_flag::TFLAG_PERMEABLE, pt ) ) ) {
                    closed.insert( pt );
                    continue;
                }

                open.emplace( static_cast<float>( rl_dist( center, pt ) ), pt );
            }
        }
    }
}

std::vector<FieldProcessorPtr> map_field_processing::processors_for_type( const field_type &ft )
{
    std::vector<FieldProcessorPtr> processors;

    const bool intensity_upgrade_chance = std::any_of( ft.intensity_levels.begin(),
                                          ft.intensity_levels.end(),
    []( const field_intensity_level & elem ) {
        return elem.intensity_upgrade_chance > 0;
    } );
    const bool extra_radiation = std::any_of( ft.intensity_levels.begin(), ft.intensity_levels.end(),
    []( const field_intensity_level & elem ) {
        return elem.extra_radiation_max > 0;
    } );

    const bool has_monster_spawn_chance = std::any_of( ft.intensity_levels.begin(),
                                          ft.intensity_levels.end(),
    []( const field_intensity_level & elem ) {
        return elem.monster_spawn_chance > 0 && elem.monster_spawn_count > 0;
    } );

    if( intensity_upgrade_chance ) {
        processors.push_back( &field_processor_upgrade_intensity );
    }
    if( ft.underwater_age_speedup != 0_turns ) {
        processors.push_back( &field_processor_underwater_dissipation );
    }
    if( ft.apply_slime_factor > 0 ) {
        processors.push_back( &field_processor_apply_slime );
    }
    if( ft.gas_can_spread() ) {
        processors.push_back( &field_processor_spread_gas );
    }

    if( extra_radiation ) {
        processors.push_back( &field_processor_extra_radiation );
    }

    if( ft.wandering_field ) {
        processors.push_back( &field_processor_wandering_field );
    }
    if( has_monster_spawn_chance ) {
        processors.push_back( &field_processor_monster_spawn );
    }

    if( ft.legacy_make_rubble ) {
        processors.push_back( &field_processor_make_rubble );
    }


    if( ft.id == fd_acid ) {
        processors.push_back( &field_processor_fd_acid );
    }
    if( ft.id == fd_extinguisher ) {
        processors.push_back( &field_processor_fd_extinguisher );
    }
    if( ft.id == fd_fire ) {
        processors.push_back( &field_processor_fd_fire );



        processors.push_back( &field_processor_fd_fungicidal_gas );
    }
    if( ft.id == fd_fungal_haze ) {
        processors.push_back( &field_processor_fd_fungal_haze );
    }
    if( ft.id == fd_fire_vent ) {
        processors.push_back( &field_processor_fd_fire_vent );
    }
    if( ft.id == fd_flame_burst ) {
        processors.push_back( &field_processor_fd_flame_burst );
    }
    if( ft.id == fd_electricity || ft.id == fd_electricity_unlit ) {
        processors.push_back( &field_processor_fd_electricity );
    }
    if( ft.id == fd_push_items ) {
        processors.push_back( &field_processor_fd_push_items );
    }
    if( ft.id == fd_shock_vent ) {
        processors.push_back( &field_processor_fd_shock_vent );
    }
    if( ft.id == fd_acid_vent ) {
        processors.push_back( &field_processor_fd_acid_vent );
    }
    if( ft.id == fd_incendiary ) {
        processors.push_back( &field_processor_fd_incendiary );
    }
    if( ft.id == fd_fungicidal_gas ) {
        processors.push_back( &field_processor_fd_fungicidal_gas );
    }
    if( ft.id == fd_last_known ) {
        processors.push_back( &field_processor_fd_last_known );
    }

    return processors;
}

const field_type_str_id &map::get_applicable_electricity_field( const tripoint_bub_ms &p ) const
{
    return is_transparent( p ) ? fd_electricity : fd_electricity_unlit;
}
