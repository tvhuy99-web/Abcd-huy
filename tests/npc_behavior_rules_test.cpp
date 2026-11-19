#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "enums.h"
#include "game.h"
#include "gates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "mapgen_helpers.h"
#include "memory_fast.h"
#include "npc.h"
#include "npctalk.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"

static const furn_str_id furn_f_chair( "f_chair" );

static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_door_o( "t_door_o" );

static const update_mapgen_id
update_mapgen_debug_npc_rules_test_avoid_doors( "debug_npc_rules_test_avoid_doors" );
static const update_mapgen_id
update_mapgen_debug_npc_rules_test_close_doors( "debug_npc_rules_test_close_doors" );

static const vproto_id vehicle_prototype_locked_as_hell_car( "locked_as_hell_car" );

static shared_ptr_fast<npc> setup_generic_rules_test( ally_rule rule_to_test,
        update_mapgen_id update_mapgen_id_to_apply )
{
    map &here = get_map();
    clear_map_without_vision();
    clear_vehicles();
    clear_avatar();
    Character &player = get_player_character();
    tripoint_bub_ms next_to = player.pos_bub() + point::north;
    REQUIRE( next_to != player.pos_bub() );
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    overmap_buffer.insert_npc( guy );
    g->load_npcs();
    clear_character( *guy );
    guy->setpos( here, next_to );
    talk_function::follow( *guy );

    REQUIRE( guy->is_player_ally() );
    npc_follower_rules &tester_rules = guy->rules;
    tester_rules = npc_follower_rules();
    tester_rules.clear_overrides();
    tester_rules.set_flag( rule_to_test );
    REQUIRE( tester_rules.has_flag( rule_to_test ) );
    const tripoint_abs_omt test_omt_pos = guy->pos_abs_omt() + point::north;
    manual_mapgen( test_omt_pos, manual_update_mapgen, update_mapgen_id_to_apply );
    return guy;
}

TEST_CASE( "NPC-rules-avoid-doors", "[npc_rules]" )
{








    const ally_rule rule_to_test = ally_rule::avoid_doors;
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test,
            update_mapgen_debug_npc_rules_test_avoid_doors );
    map &here = get_map();
    tripoint_bub_ms chair_target = test_subject->pos_bub();
    for( const tripoint_bub_ms &furn_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.furn( furn_loc ) == furn_f_chair ) {
            chair_target = furn_loc;
            break;
        }
    }

    REQUIRE( test_subject->pos_bub() != chair_target );
    test_subject->update_path( chair_target, true, true );

    REQUIRE( !test_subject->path.empty() );
    int path_position = 0;
    for( tripoint_bub_ms &loc : test_subject->path ) {
        std::string debug_log_msg = string_format( "Terrain at path position %d was %s",
                                    path_position, here.ter( loc ).id().c_str() );
        path_position++;
        CAPTURE( debug_log_msg );
        CHECK( here.ter( loc ).id() != ter_t_door_c );
    }
}

TEST_CASE( "NPC-rules-close-doors", "[npc_rules]" )
{






    const ally_rule rule_to_test = ally_rule::close_doors;
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test,
            update_mapgen_debug_npc_rules_test_close_doors );

    REQUIRE( !test_subject->rules.has_flag( ally_rule::avoid_doors ) );
    REQUIRE( !test_subject->rules.has_flag( ally_rule::avoid_locks ) );
    map &here = get_map();


    tripoint_bub_ms door_unlock_position;
    for( const tripoint_bub_ms &ter_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.ter( ter_loc ) == ter_t_door_locked ) {
            door_unlock_position = ter_loc + point::south;
            break;
        }
    }
    here.set_outside_cache_dirty( door_unlock_position.z() );
    here.build_outside_cache( door_unlock_position.z() );
    REQUIRE( !here.is_outside( door_unlock_position ) );

    tripoint_bub_ms chair_target = test_subject->pos_bub();
    for( const tripoint_bub_ms &furn_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.furn( furn_loc ) == furn_f_chair ) {
            chair_target = furn_loc;
            break;
        }
    }

    REQUIRE( test_subject->pos_bub() != chair_target );
    test_subject->update_path( chair_target, true, true );

    REQUIRE( !test_subject->path.empty() );


    test_subject->goto_to_this_pos = here.get_abs( test_subject->path.back() );


    std::vector<tripoint_bub_ms> path_taken = test_subject->path;
    int turns_taken = 0;
    while( turns_taken++ < 100 && test_subject->pos_bub() != chair_target ) {
        test_subject->set_moves( 100 );
        test_subject->process_activity();
        test_subject->move();
    }


    CHECK( turns_taken < 100 );
    CHECK( test_subject->pos_bub() == chair_target );

    for( tripoint_bub_ms &loc : path_taken ) {


        CHECK( here.ter( loc ).id() != ter_t_door_o );
    }

}

TEST_CASE( "NPC-rules-avoid-locks", "[npc_rules]" )
{











    const ally_rule rule_to_test = ally_rule::avoid_locks;
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test,
            update_mapgen_debug_npc_rules_test_close_doors );

    REQUIRE( !test_subject->rules.has_flag( ally_rule::avoid_doors ) );
    map &here = get_map();


    tripoint_bub_ms door_position;
    for( const tripoint_bub_ms &ter_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.ter( ter_loc ) == ter_t_door_locked ) {
            door_position = ter_loc;
            break;
        }
    }
    tripoint_bub_ms door_unlock_position = door_position + point::south;
    tripoint_bub_ms past_the_door = door_position + point::north;
    here.set_outside_cache_dirty( door_position.z() );
    here.build_outside_cache( door_position.z() );
    REQUIRE( !here.is_outside( door_unlock_position ) );

    test_subject->update_path( door_unlock_position, true, true );
    REQUIRE( !test_subject->path.empty() );
    test_subject->path.clear();
    test_subject->update_path( past_the_door, true, true );


    test_subject->path.clear();

    const tripoint_bub_ms car_center_pos = test_subject->pos_bub() + tripoint_rel_ms{0, 10, 0};
    const tripoint_bub_ms car_door_pos = car_center_pos + point_rel_ms{0, -2};
    const tripoint_bub_ms car_door_unlock_pos = car_door_pos + point::south;
    const tripoint_bub_ms outside_car_door_pos = car_door_pos + point::north;


    vehicle *test_vehicle = here.add_vehicle( vehicle_prototype_locked_as_hell_car,
                            car_center_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );


    std::vector<vehicle_part *> door_parts_at_target = test_vehicle->get_parts_at(
                &here, car_door_pos, "LOCKABLE_DOOR", part_status_flag::available );
    REQUIRE( !door_parts_at_target.empty() );
    vehicle_part *door = door_parts_at_target.front();

    door->open = false;

    door->locked = true;


    std::vector<vehicle_part *> door_lock_parts_at_target = test_vehicle->get_parts_at(
                &here, car_door_pos, "DOOR_LOCKING", part_status_flag::available );
    REQUIRE( !door_lock_parts_at_target.empty() );
    vehicle_part *door_lock = door_lock_parts_at_target.front();
    door_lock->locked = true;
    door_lock->open = false;
    REQUIRE( ( door_lock->is_available() && door_lock->locked ) );


    test_subject->setpos( here, car_door_unlock_pos );
    here.board_vehicle( car_door_unlock_pos, &*test_subject );

    CHECK( doors::can_unlock_door( here, *test_subject, car_door_pos ) );

    test_subject->update_path( outside_car_door_pos, true, true );


    CAPTURE( test_subject->path.size() );
    CHECK( test_subject->path.empty() );


    test_subject->rules.clear_flag( rule_to_test );
    test_subject->update_path( outside_car_door_pos, true, true );
    CHECK( !test_subject->path.empty() );

}
