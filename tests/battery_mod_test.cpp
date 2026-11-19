#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "item.h"
#include "item_contents.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"


static const ammotype ammo_battery( "battery" );

static const flag_id json_flag_IRREMOVABLE( "IRREMOVABLE" );

static const itype_id itype_battery( "battery" );
static const itype_id itype_diving_flashlight_small_hipower( "diving_flashlight_small_hipower" );
static const itype_id itype_heavy_plus_battery_cell( "heavy_plus_battery_cell" );
static const itype_id itype_light_battery_cell( "light_battery_cell" );
static const itype_id itype_magazine_battery_medium_mod( "magazine_battery_medium_mod" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );



































TEST_CASE( "battery_tool_mod_test", "[battery][mod]" )
{
    item med_mod( itype_magazine_battery_medium_mod );

    SECTION( "battery mod properties" ) {

        CHECK( med_mod.is_toolmod() );
        CHECK_FALSE( med_mod.is_gun() );
        CHECK_FALSE( med_mod.is_tool() );
        CHECK_FALSE( med_mod.is_magazine() );


        CHECK( med_mod.type->mod->acceptable_ammo.count( ammo_battery ) == 1 );

        CHECK( med_mod.type->mod->ammo_modifier.empty() );


        CHECK( med_mod.ammo_types().empty() );


        CHECK( med_mod.toolmods().empty() );


        CHECK( med_mod.magazine_compatible().empty() );
        CHECK_FALSE( med_mod.is_reloadable() );
        CHECK_FALSE( med_mod.can_reload_with( item( itype_battery ), true ) );


        CHECK_FALSE( med_mod.magazine_integral() );
    }

    GIVEN( "tool compatible with light batteries" ) {
        item flashlight( itype_diving_flashlight_small_hipower );
        REQUIRE( flashlight.is_reloadable() );
        REQUIRE( flashlight.can_reload_with( item( itype_light_battery_cell ), true ) );


        REQUIRE_FALSE( flashlight.magazine_current() );
        REQUIRE( flashlight.toolmods().empty() );

        REQUIRE( flashlight.has_pocket_type( pocket_type::MOD ) );

        WHEN( "medium battery mod is installed" ) {
            med_mod.set_flag( json_flag_IRREMOVABLE );
            flashlight.put_in( med_mod, pocket_type::MOD );

            THEN( "tool modification is successful" ) {
                CHECK_FALSE( flashlight.toolmods().empty() );
                CHECK_FALSE( flashlight.get_contents().magazine_flag_restrictions().empty() );

                CHECK( flashlight.tname() == "high-power mini diving flashlight (off)+1" );
            }

            THEN( "tool contents remain empty unless you count the mod" ) {

                CHECK( flashlight.get_contents().empty() );

                CHECK_FALSE( flashlight.get_contents().empty_with_no_mods() );
            }

            THEN( "medium batteries can be installed" ) {
                CHECK( flashlight.is_reloadable() );
                CHECK( flashlight.can_reload_with( item( itype_medium_battery_cell ), true ) );
                CHECK( flashlight.has_pocket_type( pocket_type::MAGAZINE_WELL ) );
            }

            THEN( "medium battery is now the default" ) {
                itype_id mag_default = flashlight.magazine_default( false );
                CHECK_FALSE( mag_default.is_null() );
                CHECK( mag_default.str() == "medium_battery_cell" );
            }

            THEN( "light batteries no longer fit" ) {
                CHECK_FALSE( flashlight.can_reload_with( item( itype_light_battery_cell ), true ) );
                CHECK_FALSE( flashlight.magazine_compatible().count( itype_id( itype_light_battery_cell ) ) );
            }

            WHEN( "medium battery is installed" ) {
                item med_battery( itype_medium_battery_cell );
                ret_val<void> result = flashlight.put_in( med_battery, pocket_type::MAGAZINE_WELL );

                THEN( "battery installation succeeds" ) {
                    CHECK( result.success() );
                }

                THEN( "the flashlight has a battery" ) {
                    CHECK( flashlight.magazine_current() );
                }

                THEN( "tool contents are no longer empty" ) {
                    CHECK_FALSE( flashlight.get_contents().empty() );
                }
            }

            WHEN( "charged medium battery is installed" ) {
                item med_battery( itype_medium_battery_cell );

                const int bat_charges = med_battery.ammo_capacity( ammo_battery );
                med_battery.ammo_set( med_battery.ammo_default(), bat_charges );
                REQUIRE( med_battery.ammo_remaining( ) == bat_charges );
                flashlight.put_in( med_battery, pocket_type::MAGAZINE_WELL );

                THEN( "the flashlight has charges" ) {
                    CHECK( flashlight.ammo_remaining( ) == bat_charges );
                }

                AND_WHEN( "flashlight is activated" ) {
                    const use_function *use = flashlight.type->get_use( "transform" );
                    CHECK( use != nullptr );
                    const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );

                    Character *dummy = &get_avatar();
                    clear_avatar();
                    actor->use( dummy, flashlight, &get_map(), dummy->pos_bub() );


                    THEN( "mod remains installed" ) {
                        CHECK_FALSE( flashlight.toolmods().empty() );
                        CHECK_FALSE( flashlight.get_contents().magazine_flag_restrictions().empty() );
                    }
                    THEN( "battery remains installed" ) {
                        CHECK( flashlight.magazine_current() );
                    }
                    THEN( "medium battery remains the default" ) {
                        itype_id mag_default = flashlight.magazine_default( false );
                        CHECK_FALSE( mag_default.is_null() );
                        CHECK( mag_default.str() == "medium_battery_cell" );
                    }
                }
            }
        }
    }
}













TEST_CASE( "battery_and_tool_properties", "[battery][tool][properties]" )
{
    const item bat_cell( itype_light_battery_cell );
    const item flashlight( itype_diving_flashlight_small_hipower );

    SECTION( "battery cell" ) {
        SECTION( "is a magazine" ) {
            CHECK( bat_cell.is_magazine() );
        }

        SECTION( "is not a tool" ) {
            CHECK_FALSE( bat_cell.is_tool() );
        }

        SECTION( "is not ammo" ) {
            CHECK_FALSE( bat_cell.is_ammo() );

            CHECK( bat_cell.ammo_type() == ammotype::NULL_ID() );
        }

        SECTION( "has compatible ammo types" ) {
            const std::set<ammotype> bat_ammos = bat_cell.ammo_types();
            CHECK_FALSE( bat_ammos.empty() );
            CHECK( bat_ammos.count( ammo_battery ) );
        }

        SECTION( "has capacity to hold battery ammo type" ) {
            CHECK( bat_cell.ammo_capacity( ammo_battery ) > 0 );
        }

        SECTION( "has battery ammo as default" ) {
            CHECK( bat_cell.ammo_default() == itype_battery );
        }

        SECTION( "is not counted by charges" ) {
            CHECK_FALSE( bat_cell.count_by_charges() );
        }
    }

    SECTION( "flashlight" ) {
        SECTION( "is a tool" ) {
            CHECK( flashlight.is_tool() );
        }

        SECTION( "is not a magazine" ) {
            CHECK_FALSE( flashlight.is_magazine() );
            CHECK_FALSE( flashlight.magazine_integral() );
        }

        SECTION( "is not ammo" ) {
            CHECK_FALSE( flashlight.is_ammo() );

            CHECK( flashlight.ammo_type() == ammotype::NULL_ID() );
        }

        SECTION( "is reloadable with a magazine" ) {
            CHECK( flashlight.is_reloadable() );
            CHECK( flashlight.can_reload_with( item( itype_light_battery_cell ), true ) );
        }

        SECTION( "has compatible magazines" ) {
            CHECK( flashlight.can_contain( *itype_light_battery_cell ).success() );
        }

        SECTION( "Does not fit medium or large magazines" ) {
            CHECK_FALSE( flashlight.can_contain( *itype_medium_battery_cell ).success() );
            CHECK_FALSE( flashlight.can_contain( *itype_heavy_plus_battery_cell ).success() );
        }

        SECTION( "has a default magazine" ) {
            itype_id mag_default = flashlight.magazine_default( false );
            CHECK_FALSE( mag_default.is_null() );
            CHECK( mag_default.str() == "light_battery_cell" );
        }

        SECTION( "can use battery ammo" ) {

            CHECK( flashlight.ammo_types().empty() );
            CHECK( flashlight.ammo_default().is_null() );


            CHECK_FALSE( flashlight.type->tool->ammo_id.empty() );
            CHECK( flashlight.type->tool->ammo_id.count( ammo_battery ) == 1 );
        }

        SECTION( "requires some ammo (charge) to use" ) {
            CHECK( flashlight.ammo_required() > 0 );
        }

        SECTION( "is not counted by charges" ) {
            CHECK_FALSE( flashlight.count_by_charges() );
        }
    }
}

TEST_CASE( "installing_battery_in_tool", "[battery][tool][install]" )
{
    item bat_cell( itype_light_battery_cell );
    item flashlight( itype_diving_flashlight_small_hipower );

    const int bat_charges = bat_cell.ammo_capacity( ammo_battery );
    REQUIRE( bat_charges > 0 );

    SECTION( "flashlight with no battery installed" ) {
        REQUIRE( !flashlight.magazine_current() );

        CHECK( flashlight.ammo_remaining( ) == 0 );
        CHECK( flashlight.ammo_capacity( ammo_battery ) == 0 );
        CHECK( flashlight.remaining_ammo_capacity() == 0 );
    }

    SECTION( "dead battery installed in flashlight" ) {

        bat_cell.ammo_set( bat_cell.ammo_default(), 0 );
        REQUIRE( bat_cell.ammo_remaining( ) == 0 );


        REQUIRE( flashlight.has_pocket_type( pocket_type::MAGAZINE_WELL ) );
        ret_val<void> result = flashlight.put_in( bat_cell, pocket_type::MAGAZINE_WELL );
        CHECK( result.success() );
        CHECK( flashlight.magazine_current() );


        CHECK( flashlight.ammo_remaining( ) == 0 );
    }

    SECTION( "charged battery installed in flashlight" ) {

        bat_cell.ammo_set( bat_cell.ammo_default(), bat_charges );
        REQUIRE( bat_cell.ammo_remaining( ) == bat_charges );


        REQUIRE( flashlight.has_pocket_type( pocket_type::MAGAZINE_WELL ) );
        ret_val<void> result = flashlight.put_in( bat_cell, pocket_type::MAGAZINE_WELL );
        CHECK( result.success() );
        CHECK( flashlight.magazine_current() );


        CHECK( flashlight.ammo_remaining( ) == bat_charges );
    }

    SECTION( "wrong size battery for flashlight" ) {
        item med_bat_cell( itype_medium_battery_cell );


        REQUIRE( flashlight.has_pocket_type( pocket_type::MAGAZINE_WELL ) );
        std::string dmsg = capture_debugmsg_during( [&flashlight, &med_bat_cell]() {
            ret_val<void> result = flashlight.put_in( med_bat_cell, pocket_type::MAGAZINE_WELL );
            CHECK_FALSE( result.success() );
        } );
        CHECK_THAT( dmsg,
                    Catch::Matchers::EndsWith( "holster does not accept this item type or form factor" ) );
        CHECK_FALSE( flashlight.magazine_current() );
    }
}
