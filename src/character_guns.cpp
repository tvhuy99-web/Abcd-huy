#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_selector.h"
#include "player_activity.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "value_ptr.h"
#include "vehicle_selector.h"
#include "visitable.h"

static const itype_id itype_large_repairkit( "large_repairkit" );
static const itype_id itype_small_repairkit( "small_repairkit" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

template <typename T, typename Output>
static void find_ammo_helper( T &src, const item &obj, bool empty, Output out, bool nested,
                              bool now = true )
{
    src.visit_items( [&nested, &out, &obj, empty, now]( item_location node ) {


        if( node.get_item() == &obj ) {
            return VisitResponse::SKIP;
        }


        if( !node.has_parent() && node->made_of_from_type( phase_id::LIQUID ) ) {
            return VisitResponse::SKIP;
        }


        if( node->is_frozen_liquid() ) {
            return VisitResponse::SKIP;
        }


        if( node.has_parent() && node.parent_item()->is_magazine() ) {
            return VisitResponse::SKIP;
        }


        if( node.has_parent() && node.get_item() == node.parent_item()->magazine_current() ) {
            return VisitResponse::SKIP;
        }


        if( node->is_magazine() && !node->ammo_remaining( ) && !empty ) {
            return VisitResponse::SKIP;
        }

        if( node->has_flag( flag_SPEEDLOADER ) && obj.magazine_integral() ) {

            if( !node->ammo_remaining( ) ) {
                return VisitResponse::SKIP;
            }



            if( obj.can_reload_with( *node, now ) ) {
                out = node;
            }
            return VisitResponse::SKIP;
        }

        if( obj.can_reload_with( *node, now ) ) {
            out = node;
        }


        return !node.has_parent() || nested ? VisitResponse::NEXT : VisitResponse::SKIP;

    } );
}

std::vector<item_location> Character::get_ammo( const ammotype &at ) const
{
    return cache_get_items_with( "is_ammo", &item::is_ammo, [at]( const item_location & it ) {
        return it->ammo_type() == at;
    } );
}

std::vector<item_location> Character::find_ammo( const item &obj, bool empty, int radius,
        bool now ) const
{
    map &here = get_map();

    std::vector<item_location> res = cache_get_items_with( "is_ammo",
    &item::is_ammo, [&obj, now]( const item_location & it ) {
        return obj.can_reload_with( *it, now );
    } );

    std::vector<item_location> mag_locs = cache_get_items_with( "is_magazine",
    &item::is_magazine, [&obj, &empty, now]( const item_location & it ) {
        if( &obj == &*it ) {
            return false;
        }
        if( it.parent_item() && &*it == it.parent_item()->magazine_current() ) {
            return false;
        }
        if( !it->ammo_remaining() && !empty ) {
            return false;
        }
        if( it->has_flag( flag_SPEEDLOADER ) && ( !it->ammo_remaining() || !obj.magazine_integral() ) ) {
            return false;
        }
        return obj.can_reload_with( *it, now );
    } );

    res.insert( res.end(), mag_locs.begin(), mag_locs.end() );

    if( radius >= 0 ) {
        for( map_cursor &cursor : map_selector( pos_bub(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false, now );
        }
        for( vehicle_cursor &cursor : vehicle_selector( here, pos_bub( here ), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false, now );
        }
    }

    return res;
}

std::pair<int, int> Character::gunmod_installation_odds( const item_location &gun,
        const item &mod ) const
{

    if( !mod.has_flag( flag_INSTALL_DIFFICULT ) || has_trait( trait_DEBUG_HS ) ) {
        return std::make_pair( 100, 0 );
    }

    int roll = 100;
    int risk = 0;
    float chances = 1.0f;

    for( const auto &e : mod.type->min_skills ) {

        skill_id sk = e.first.str() == "weapon" ? gun->gun_skill() : e.first;
        chances += std::max( get_greater_skill_or_knowledge_level( sk )  - e.second, 0.0f );
    }

    roll = std::min( static_cast<double>( chances ), 5.0 ) / 6.0 * 100;

    roll += ( std::clamp( get_focus(), 60, 140 ) - 100 ) / 4;

    roll += ( get_dex() - 12 ) * 2;
    roll += ( get_int() - 12 ) * 2;

    roll -= gun->damage_level() * 10;
    roll = std::min( std::max( roll, 0 ), 100 );


    risk = ( 100 - roll ) * ( ( 10.0 - std::min( gun->type->gun->durability, 9 ) ) / 10.0 );

    return std::make_pair( roll, risk );
}

void Character::gunmod_add( item &gun, item &mod )
{
    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Tried to add incompatible gunmod" );
        return;
    }

    if( !has_item( gun ) && !has_item( mod ) ) {
        debugmsg( "Tried gunmod installation but mod/gun not in player possession" );
        return;
    }


    if( !has_trait( trait_DEBUG_HS ) && !can_use( mod, gun ) ) {
        return;
    }

    itype_id mod_type = mod.typeId();
    std::string mod_name = mod.tname();

    if( !is_wielding( gun ) && !wield( gun ) ) {
        add_msg_if_player( _( "You can't wield the %1$s." ), gun.tname() );
        return;
    }


    item_location wielded_gun = get_wielded_item();
    std::vector<item *> mods = items_with( [&mod_type]( const item & it ) {
        return it.typeId() == mod_type;
    } );

    if( mods.empty() ) {
        add_msg_if_player( _( "You no longer have a %s and can't continue crafting." ), mod_name );
        return;
    }

    item &moved_mod = *mods.front();

    auto odds = gunmod_installation_odds( wielded_gun, moved_mod );
    int roll = odds.first;
    int risk = odds.second;

    std::string tool;
    int qty = 0;

    if( mod.is_irremovable() ) {
        if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                       colorize( moved_mod.tname(), moved_mod.color_in_inventory() ),
                       colorize( wielded_gun->tname(), wielded_gun->color_in_inventory() ) ) ) {
            add_msg_if_player( _( "Never mind." ) );
            return;
        }
    }


    if( roll < 100 ) {
        uilist prompt;
        prompt.text = string_format( _( "Attach your %1$s to your %2$s?" ), moved_mod.tname(),
                                     wielded_gun->tname() );

        std::vector<std::function<void()>> actions;

        prompt.addentry( -1, true, 'w',
                         string_format( _( "Try without tools (%i%%) risking damage (%i%%)" ), roll, risk ) );
        actions.emplace_back( [&] {} );

        prompt.addentry( -1, has_charges( itype_small_repairkit, 100 ), 'f',
                         string_format( _( "Use 100 charges of firearm repair kit (%i%%)" ), std::min( roll * 2, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "small_repairkit";
            qty = 100;
            roll *= 2;
            risk /= 2;
        } );

        prompt.addentry( -1, has_charges( itype_large_repairkit, 25 ), 'g',
                         string_format( _( "Use 25 charges of gunsmith repair kit (%i%%)" ), std::min( roll * 3, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "large_repairkit";
            qty = 25;
            roll *= 3;
            risk = 0;
        } );

        prompt.query();
        if( prompt.ret < 0 ) {
            add_msg_if_player( _( "Never mind." ) );
            return;
        }
        actions[prompt.ret]();
    }

    const int moves = !has_trait( trait_DEBUG_HS ) ? to_moves<int>
                      ( moved_mod.type->gunmod->install_time ) : 0;

    assign_activity( gunmod_add_activity_actor( moves, tool ) );
    activity.targets.emplace_back( wielded_gun );
    activity.targets.emplace_back( *this, &moved_mod );
    activity.values.push_back( 0 );
    activity.values.push_back( roll );
    activity.values.push_back( risk );
    activity.values.push_back( qty );
}

bool Character::gunmod_remove( item &gun, item &mod )
{
    std::vector<item *> mods = gun.gunmods();
    size_t gunmod_idx = mods.size();
    for( size_t i = 0; i < mods.size(); i++ ) {
        if( mods[i] == &mod ) {
            gunmod_idx = i;
            break;
        }
    }
    if( gunmod_idx == mods.size() ) {
        debugmsg( "Cannot remove non-existent gunmod" );
        return false;
    }

    if( !gunmod_remove_activity_actor::gunmod_unload( *this, mod ) ) {
        return false;
    }


    const int moves = has_trait( trait_DEBUG_HS ) ? 0 : to_moves<int>
                      ( mod.type->gunmod->install_time ) / 2;
    item_location gun_loc = item_location( *this, &gun );
    assign_activity( gunmod_remove_activity_actor( moves, gun_loc, static_cast<int>( gunmod_idx ) ) );
    return true;
}

bool Character::has_gun_for_ammo( const ammotype &at ) const
{
    return cache_has_item_with( "is_gun", &item::is_gun, [&at]( const item & it ) {
        return it.ammo_types().count( at );
    } );
}

bool Character::has_magazine_for_ammo( const ammotype &at ) const
{
    if( cache_has_item_with( "is_magazine", &item::is_magazine, [&at]( const item & it ) {
    return !it.has_flag( flag_NO_RELOAD ) && it.ammo_types().count( at );
    } ) ) {
        return true;
    }
    return cache_has_item_with( "is_gun", &item::is_gun, [&at]( const item & it ) {
        return !it.has_flag( flag_NO_RELOAD ) &&
               ( ( it.magazine_integral() && it.ammo_types().count( at ) ) ||
                 ( it.magazine_current() != nullptr && it.magazine_current()->ammo_types().count( at ) ) );
    } );
}
