#include "ammo.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clone_ptr.h"
#include "debug.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "iexamine.h"
#include "iexamine_actors.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "mapdata.h"
#include "type_id.h"
#include "value_ptr.h"
#include "worldfactory.h"

static const ammotype ammo_120mm( "120mm" );
static const ammotype ammo_afs_titanium_small( "afs_titanium_small" );
static const ammotype ammo_gunpowder( "gunpowder" );
static const ammotype ammo_gunpowder_artillery( "gunpowder_artillery" );
static const ammotype ammo_gunpowder_large_rifle( "gunpowder_large_rifle" );
static const ammotype ammo_gunpowder_magnum_pistol( "gunpowder_magnum_pistol" );
static const ammotype ammo_gunpowder_pistol( "gunpowder_pistol" );
static const ammotype ammo_gunpowder_rifle( "gunpowder_rifle" );
static const ammotype ammo_gunpowder_shotgun( "gunpowder_shotgun" );
static const ammotype ammo_mana( "mana" );
static const ammotype ammo_plutonium( "plutonium" );
static const ammotype ammo_rolling_paper( "rolling_paper" );
static const ammotype ammo_test_1g_weight( "test_1g_weight" );
static const ammotype ammo_test_graphite( "test_graphite" );
static const ammotype ammo_test_solid_1ml( "test_solid_1ml" );
static const ammotype ammo_throwing_stick_with_charges( "throwing_stick_with_charges" );
static const ammotype ammo_unfinished_char( "unfinished_char" );

static const mod_id MOD_INFORMATION_generic_guns( "generic_guns" );

namespace
{
using ammo_map_t = std::unordered_map<ammotype, ammunition_type>;

ammo_map_t &all_ammunition_types()
{
    static ammo_map_t the_map;
    return the_map;
}
}

ammunition_type::ammunition_type() : name_( no_translation( "null" ) )
{
}

void ammunition_type::load_ammunition_type( const JsonObject &jsobj )
{
    ammunition_type &res = all_ammunition_types()[ ammotype( jsobj.get_string( "id" ) ) ];
    jsobj.read( "name", res.name_ );
    jsobj.read( "default", res.default_ammotype_, true );
}


template<>
bool string_id<ammunition_type>::is_valid() const
{
    return all_ammunition_types().count( *this ) > 0;
}


template<>
const ammunition_type &string_id<ammunition_type>::obj() const
{
    const ammo_map_t &the_map = all_ammunition_types();

    const auto it = the_map.find( *this );
    if( it != the_map.end() ) {
        return it->second;
    }

    debugmsg( "Tried to get invalid ammunition: %s", c_str() );
    static const ammunition_type null_ammunition;
    return null_ammunition;
}

void ammunition_type::reset()
{
    all_ammunition_types().clear();
}

void ammunition_type::check_consistency()
{


    std::set<ammotype> whitelist = {
        ammo_unfinished_char,
        ammo_plutonium,
        ammo_gunpowder,
        ammo_mana,
        ammo_gunpowder_artillery,
        ammo_gunpowder_large_rifle,
        ammo_gunpowder_magnum_pistol,
        ammo_gunpowder_pistol,
        ammo_gunpowder_rifle,
        ammo_gunpowder_shotgun,
        ammo_afs_titanium_small,
        ammo_120mm,
        ammo_rolling_paper,
        ammo_throwing_stick_with_charges,
        ammo_test_1g_weight,
        ammo_test_solid_1ml,
        ammo_test_graphite
    };

    std::vector<mod_id> &loaded_mods = world_generator->active_world->active_mod_order;
    const bool skip_all_unused_checks = std::find( loaded_mods.begin(), loaded_mods.end(),
                                        MOD_INFORMATION_generic_guns ) != loaded_mods.end();

    for( const auto &ammo : all_ammunition_types() ) {
        const auto &id = ammo.first;
        const itype_id &at = ammo.second.default_ammotype_;

        if( !item::type_is_defined( at ) ) {
            debugmsg( "ammo type %s has invalid default ammo %s", id.c_str(), at.c_str() );
        }





        if( skip_all_unused_checks ) {
            continue;
        }


        std::vector<const itype *> shoots_this_ammo = Item_factory::find( [&]( const itype & t ) {
            item maybe_ammo_user( t.get_id() );

            return maybe_ammo_user.ammo_types().count( id ) ||
                   ( t.mod && t.mod->ammo_modifier.count( id ) );
        } );


        for( const ter_t &some_ter_furn : mapdata::terrain_data.get_all() ) {
            if( some_ter_furn.has_examine( "mortar" ) ) {
                for( auto &examiner : some_ter_furn.get_examine() ) {
                    mortar_examine_actor *actor = dynamic_cast<mortar_examine_actor *>( examiner.get() );
                    if( actor ) {
                        for( const ammotype &some_ammo : actor->get_ammotypes() ) {
                            if( some_ammo == id ) {

                                shoots_this_ammo.push_back( &*itype_id::NULL_ID() );
                                break;
                            }
                        }
                    }
                }
            }
        }


        for( const furn_t &some_ter_furn : mapdata::furniture_data.get_all() ) {
            if( some_ter_furn.has_examine( "mortar" ) ) {
                for( auto &examiner : some_ter_furn.get_examine() ) {
                    mortar_examine_actor *actor = dynamic_cast<mortar_examine_actor *>( examiner.get() );
                    if( actor ) {
                        for( const ammotype &some_ammo : actor->get_ammotypes() ) {
                            if( some_ammo == id ) {
                                shoots_this_ammo.push_back( &*itype_id::NULL_ID() );
                                break;
                            }
                        }
                    }
                }
            }
        }

        std::vector<const itype *> solid_types_as_this_ammo = Item_factory::find( [&]( const itype & t ) {
            return t.ammo && t.ammo->type == id && t.phase == phase_id::SOLID;
        } );


        const bool is_whitelisted = whitelist.count( id );


        const bool is_liquid_sole_type = solid_types_as_this_ammo.empty();

        if( shoots_this_ammo.empty() && !is_whitelisted && !is_liquid_sole_type ) {
            debugmsg( "ammotype %s exists but is unused", id.c_str() );
        }


        if( is_whitelisted && solid_types_as_this_ammo.size() > 1 ) {
            debugmsg( "whitelisted ammo type %s has more than one entry.", id.c_str() );
        }

    }
}

std::string ammunition_type::name() const
{
    return name_.translated();
}
