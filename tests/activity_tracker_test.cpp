#include <numeric>
#include <string>
#include <vector>

#include "activity_tracker.h"
#include "calendar.h"
#include "cata_catch.h"
#include "game_constants.h"
#include "rng.h"

static void test_activity_tracker( const std::vector<float> &values )
{
    activity_tracker tracker;
    for( float i : values ) {
        calendar::turn += 1_turns;
        tracker.new_turn( false );

        CHECK( tracker.activity( false ) == 1.0 );

        tracker.log_activity( rng_float( 0.0f, i - 0.01f ) );
        tracker.log_activity( i );
        tracker.log_activity( rng_float( 0.0f, i - 0.01f ) );

        CHECK( tracker.activity( false ) == i );
    }
    int end_value = values.back();

    CHECK( tracker.activity( false ) == end_value );
    const float expected_activity = std::accumulate( values.begin(), values.end(), 0.0f ) /
                                    static_cast<float>( values.size() );

    CHECK( tracker.average_activity() == expected_activity );
    tracker.reset_activity_level();

    CHECK( tracker.activity( false ) == end_value );

    CHECK( tracker.average_activity() == expected_activity );
    calendar::turn += 1_turns;

    CHECK( tracker.activity( false ) == 1.0f );
    tracker.new_turn( false );
    tracker.log_activity( 5.0f );

    CHECK( tracker.activity( false ) == 5.0f );
    CHECK( tracker.average_activity() == 5.0f );
    calendar::turn += 1_turns;
    tracker.new_turn( false );
    tracker.log_activity( 7.0f );

    CHECK( tracker.activity( false ) == 7.0f );
    CHECK( tracker.average_activity() == 6.0f );
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "activity_counter_from_1_to_300", "[activity_tracker]" )
{
    std::vector<float> values;
    for( int i = 1; i <= 300; ++i ) {
        values.push_back( i );
    }
    calendar::turn += 50000_turns;
    test_activity_tracker( values );
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "activity_tracker_from_300_to_1", "[activity_tracker]" )
{
    std::vector<float> values;
    for( int i = 300; i > 0; --i ) {
        values.push_back( i );
    }
    test_activity_tracker( values );
}

TEST_CASE( "activity_tracker_constant_value", "[activity_tracker]" )
{
    std::vector<float> values( 300, 1.0 );
    test_activity_tracker( values );
}

TEST_CASE( "activity_tracker_intermittent_values", "[activity_tracker]" )
{
    std::vector<float> values( 300, 1.0 );
    for( int i = 0; i < 300; i += 30 ) {
        values[ i ] = 10.0f;
    }
    test_activity_tracker( values );
}

TEST_CASE( "activity_tracker_string_representation", "[activity_tracker]" )
{
    activity_tracker tracker;

    tracker.reset_activity_level();
    REQUIRE( tracker.activity_level_str() == "SLEEP_EXERCISE" );


    tracker.log_activity( LIGHT_EXERCISE );
    CHECK( tracker.activity_level_str() == "LIGHT_EXERCISE" );
    tracker.log_activity( MODERATE_EXERCISE );
    CHECK( tracker.activity_level_str() == "MODERATE_EXERCISE" );

    tracker.log_activity( LIGHT_EXERCISE );
    CHECK( tracker.activity_level_str() == "MODERATE_EXERCISE" );
    tracker.log_activity( NO_EXERCISE );
    CHECK( tracker.activity_level_str() == "MODERATE_EXERCISE" );

    tracker.log_activity( ACTIVE_EXERCISE );
    CHECK( tracker.activity_level_str() == "ACTIVE_EXERCISE" );
    tracker.log_activity( EXTRA_EXERCISE );
    CHECK( tracker.activity_level_str() == "EXTRA_EXERCISE" );

    tracker.log_activity( EXTRA_EXERCISE );
    CHECK( tracker.activity_level_str() == "EXTRA_EXERCISE" );
}
