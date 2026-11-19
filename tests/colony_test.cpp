#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "colony.h"
#include "colony_list_test_helpers.h"



unsigned int xor_rand()
{
    static unsigned int x = 123456789;
    static unsigned int y = 362436069;
    static unsigned int z = 521288629;
    static unsigned int w = 88675123;

    const unsigned int t = x ^ ( x << 11 );


    x = y;
    y = z;
    z = w;

    return w = w ^ ( w >> 19 ) ^ ( t ^ ( t >> 8 ) );
}

TEST_CASE( "colony_basics", "[colony]" )
{
    cata::colony<int *> test_colony;


    CHECK( test_colony.empty() );

    int ten = 10;
    test_colony.insert( &ten );


    CHECK_FALSE( test_colony.empty() );

    CHECK( **test_colony.begin() == 10 );

    CHECK_FALSE( test_colony.begin() == test_colony.end() );

    test_colony.clear();


    CHECK( test_colony.begin() == test_colony.end() );

    int twenty = 20;
    for( int temp = 0; temp < 200; ++temp ) {
        test_colony.insert( &ten );
        test_colony.insert( &twenty );
    }

    int count = 0;
    int sum = 0;
    for( cata::colony<int *>::iterator it = test_colony.begin(); it < test_colony.end(); ++it ) {
        ++count;
        sum += **it;
    }


    CHECK( count == 400 );

    CHECK( sum == 6000 );

    cata::colony<int *>::iterator plus_20 = test_colony.begin();
    cata::colony<int *>::iterator plus_200 = test_colony.begin();
    advance( plus_20, 20 );
    advance( plus_200, 200 );


    CHECK( distance( test_colony.begin(), plus_20 ) == 20 );

    CHECK( distance( plus_200, test_colony.begin() ) == -200 );

    cata::colony<int *>::iterator next_iterator = next( test_colony.begin(), 5 );
    cata::colony<int *>::const_iterator cprev_iterator = prev( test_colony.cend(), 300 );


    CHECK( distance( test_colony.begin(), next_iterator ) == 5 );

    CHECK( distance( test_colony.cend(), cprev_iterator ) == -300 );

    cata::colony<int *>::iterator prev_iterator = prev( test_colony.end(), 300 );


    CHECK( cprev_iterator == prev_iterator );

    cata::colony<int *> test_colony_2;
    test_colony_2 = test_colony;

    cata::colony<int *> test_colony_3( test_colony );
    cata::colony<int *> test_colony_4( test_colony_2, test_colony_2.get_allocator() );


    CHECK( test_colony_2.size() == 400 );

    CHECK( test_colony_3.size() == 400 );

    CHECK( test_colony_4.size() == 400 );


    CHECK( test_colony == test_colony_2 );
    CHECK( test_colony_2 == test_colony_3 );
    CHECK( test_colony_3 == test_colony_4 );

    test_colony_2.insert( &ten );


    CHECK( test_colony_2 != test_colony_3 );

    count = 0;
    sum = 0;
    for( cata::colony<int *>::reverse_iterator it = test_colony.rbegin(); it != test_colony.rend();
         ++it ) {
        ++count;
        sum += **it;
    }


    CHECK( count == 400 );

    CHECK( sum == 6000 );

    cata::colony<int *>::reverse_iterator r_iterator = test_colony.rbegin();
    advance( r_iterator, 50 );


    CHECK( distance( test_colony.rbegin(), r_iterator ) == 50 );

    cata::colony<int *>::reverse_iterator r_iterator2 = next( r_iterator, 2 );


    CHECK( distance( test_colony.rbegin(), r_iterator2 ) == 52 );

    count = 0;
    sum = 0;
    for( cata::colony<int *>::iterator it = test_colony.begin(); it < test_colony.end();
         advance( it, 2 ) ) {
        ++count;
        sum += **it;
    }


    CHECK( count == 200 );

    CHECK( sum == 2000 );

    count = 0;
    sum = 0;
    for( int *p : test_colony ) {
        ++count;
        sum += *p;
    }


    CHECK( count == 400 );

    CHECK( sum == 6000 );

    count = 0;
    sum = 0;
    for( cata::colony<int *>::const_reverse_iterator it = --cata::colony<int *>::const_reverse_iterator(
                test_colony.crend() ); it != cata::colony<int *>::const_reverse_iterator( test_colony.crbegin() );
         --it ) {
        ++count;
        sum += **it;
    }


    CHECK( count == 399 );

    CHECK( sum == 5980 );

    count = 0;
    for( cata::colony<int *>::iterator it = ++cata::colony<int *>::iterator( test_colony.begin() );
         it < test_colony.end(); ++it ) {
        ++count;
        it = test_colony.erase( it );
    }


    CHECK( count == 200 );

    CHECK( test_colony.size() == 200 );

    const unsigned int temp_capacity = static_cast<unsigned int>( test_colony.capacity() );
    test_colony.shrink_to_fit();

    CHECK( test_colony.capacity() < temp_capacity );
    CHECK( test_colony.capacity() == 200 );

    count = 0;
    for( cata::colony<int *>::reverse_iterator it = test_colony.rbegin(); it != test_colony.rend();
         ++it ) {
        ++count;
        cata::colony<int *>::iterator temp = it.base();
        test_colony.erase( --temp );
    }


    CHECK( count == 200 );

    CHECK( test_colony.empty() );


    for( int temp = 0; temp < 200; ++temp ) {
        test_colony.insert( &ten );
        test_colony.insert( &twenty );
    }

    count = 0;
    for( cata::colony<int *>::iterator it = --cata::colony<int *>::iterator( test_colony.end() );
         it != test_colony.begin(); --it ) {
        ++count;
    }


    CHECK( count == 399 );

    count = 0;
    for( cata::colony<int *>::iterator it = --( cata::colony<int *>::iterator( test_colony.end() ) );
         it != test_colony.begin(); advance( it, -2 ) ) {
        ++count;
    }


    CHECK( count == 200 );

    test_colony_2 = std::move( test_colony );


    CHECK( test_colony_2.size() == 400 );

    // NOLINTNEXTLINE(bugprone-use-after-move)
    test_colony.insert( &ten );


    CHECK( test_colony.size() == 1 );

    cata::colony<int *> test_colony_5( test_colony_2 );
    cata::colony<int *> test_colony_6( std::move( test_colony_5 ), test_colony_2.get_allocator() );


    CHECK( test_colony_6.size() == 400 );

    test_colony_3 = test_colony_2;


    CHECK( test_colony_3.size() == 400 );

    test_colony_2.insert( &ten );

    test_colony_2.swap( test_colony_3 );


    CHECK( test_colony_2.size() == test_colony_3.size() - 1 );

    swap( test_colony_2, test_colony_3 );


    CHECK( test_colony_3.size() == test_colony_2.size() - 1 );


    CHECK( test_colony_2.max_size() > test_colony_2.size() );
}

TEST_CASE( "colony_insert_and_erase", "[colony]" )
{
    cata::colony<int> test_colony;

    for( int i = 0; i < 500000; ++i ) {
        test_colony.insert( i );
    }


    CHECK( test_colony.size() == 500000 );

    cata::colony<int>::iterator found = std::find( test_colony.begin(), test_colony.end(), 5000 );
    cata::colony<int>::reverse_iterator found2 = std::find( test_colony.rbegin(), test_colony.rend(),
            5000 );


    CHECK( *found == 5000 );

    CHECK( *found2 == 5000 );

    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        it = test_colony.erase( it );
    }


    CHECK( test_colony.size() == 250000 );

    do {
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony.erase( it );
            } else {
                ++it;
            }
        }

    } while( !test_colony.empty() );


    CHECK( test_colony.empty() );

    test_colony.reset();
    test_colony.reshape( plf::limits( 100, test_colony.block_capacity_limits().max ) );

    for( int i = 0; i != 30000; ++i ) {
        test_colony.insert( 1 );
    }


    CHECK( test_colony.size() == 30000 );

    int count = 0;
    do {
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony.erase( it );
                ++count;
            } else {
                ++it;
            }
        }

    } while( count < 15000 );


    CHECK( test_colony.size() == static_cast<size_t>( 30000 - count ) );

    for( int i = 0; i < count; ++i ) {
        test_colony.insert( 1 );
    }


    CHECK( test_colony.size() == 30000 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
        if( ++count == 3 ) {
            count = 0;
            it = test_colony.erase( it );
        } else {
            test_colony.insert( 1 );
            ++it;
        }
    }


    CHECK( test_colony.size() == 45001 );

    do {
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 3 ) == 0 ) {
                ++it;
                test_colony.insert( 1 );
            } else {
                it = test_colony.erase( it );
            }
        }
    } while( !test_colony.empty() );


    CHECK( test_colony.empty() );

    for( int i = 0; i != 500000; ++i ) {
        test_colony.insert( 10 );
    }


    CHECK( test_colony.size() == 500000 );

    cata::colony<int>::iterator it = test_colony.begin();
    advance( it, 250000 );

    for( ; it != test_colony.end(); ) {
        it = test_colony.erase( it );
    }


    CHECK( test_colony.size() == 250000 );

    for( int i = 0; i < 250000; ++i ) {
        test_colony.insert( 10 );
    }

    cata::colony<int>::iterator end_it = test_colony.end();
    advance( end_it, -250000 );

    for( cata::colony<int>::iterator it = test_colony.begin(); it != end_it; ) {
        it = test_colony.erase( it );
    }


    CHECK( test_colony.size() == 250000 );

    for( int i = 0; i < 250000; ++i ) {
        test_colony.insert( 10 );
    }

    int sum = 0;
    for( int it : test_colony ) {
        sum += it;
    }


    CHECK( sum == 5000000 );

    end_it = test_colony.end();
    advance( end_it, -50001 );
    cata::colony<int>::iterator begin_it = test_colony.begin();
    advance( begin_it, 300000 );

    for( cata::colony<int>::iterator it = begin_it; it != end_it; ) {
        it = test_colony.erase( it );
    }


    CHECK( test_colony.size() == 350001 );

    for( int i = 0; i < 100000; ++i ) {
        test_colony.insert( 10 );
    }

    begin_it = test_colony.begin();
    advance( begin_it, 300001 );

    for( cata::colony<int>::iterator it = begin_it; it != test_colony.end(); ) {
        it = test_colony.erase( it );
    }


    CHECK( test_colony.size() == 300001 );

    it = test_colony.begin();
    advance( it, 2 );
    unsigned int index = static_cast<unsigned int>( cata::get_index_from_iterator( test_colony, it ) );


    CHECK( index == 2 );


    test_colony.erase( it );
    it = test_colony.begin();
    advance( it, 500 );
    index = static_cast<unsigned int>( cata::get_index_from_iterator( test_colony, it ) );


    CHECK( index == 500 );

    cata::colony<int>::iterator it2 = test_colony.get_iterator( &( *it ) );


    CHECK( it2 != test_colony.end() );

    it2 = cata::get_iterator_from_index( test_colony, 500 );


    CHECK( it2 == it );

    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
        it = test_colony.erase( it );
    }


    CHECK( test_colony.empty() );

    test_colony.reset();
    test_colony.reshape( plf::limits( 3, test_colony.block_capacity_limits().max ) );
    const unsigned int temp_capacity2 = static_cast<unsigned int>( test_colony.capacity() );
    test_colony.reserve( 1000 );


    CHECK( temp_capacity2 != test_colony.capacity() );
    CHECK( test_colony.capacity() >= 1000 );

    count = 0;
    for( int i = 0; i < 50000; ++i ) {
        for( int j = 0; j < 10; ++j ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                test_colony.insert( 1 );
                ++count;
            }
        }
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony.erase( it );
                --count;
            } else {
                ++it;
            }
        }
    }


    CHECK( test_colony.size() == static_cast<size_t>( count ) );
}

TEST_CASE( "colony_range_erase", "[colony]" )
{
    cata::colony<int> test_colony;

    int count = 0;
    for( ; count != 1000; ++count ) {
        test_colony.insert( count );
    }

    cata::colony<int>::iterator it1 = test_colony.begin();
    cata::colony<int>::iterator it2 = test_colony.begin();

    advance( it1, 500 );
    advance( it2, 800 );

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 700 );
    CHECK( test_colony.size() == 700 );

    it1 = test_colony.begin();
    it2 = test_colony.begin();

    advance( it1, 400 );
    advance( it2, 500 );

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 600 );
    CHECK( test_colony.size() == 600 );

    it2 = it1 = test_colony.begin();

    advance( it1, 4 );
    advance( it2, 9 );

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 595 );
    CHECK( test_colony.size() == 595 );

    it1 = test_colony.begin();
    it2 = test_colony.begin();

    advance( it2, 50 );

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 545 );
    CHECK( test_colony.size() == 545 );

    it1 = test_colony.begin();
    it2 = test_colony.end();


    advance( it1, 345 );
    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 345 );
    CHECK( test_colony.size() == 345 );

    test_colony.clear();

    for( count = 0; count != 3000; ++count ) {
        test_colony.insert( count );
    }

    for( cata::colony<int>::iterator it = test_colony.begin(); it < test_colony.end(); ++it ) {
        it = test_colony.erase( it );
    }

    it2 = test_colony.begin();
    it1 = test_colony.begin();

    advance( it1, 4 );
    advance( it2, 600 );
    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 904 );
    CHECK( test_colony.size() == 904 );

    test_colony.clear();

    for( count = 0; count < 3000; ++count ) {
        test_colony.insert( count );
    }

    for( cata::colony<int>::iterator it = test_colony.begin(); it < test_colony.end(); ++it ) {
        if( ( xor_rand() & 1 ) == 0 ) {
            it = test_colony.erase( it );
        }
    }

    if( test_colony.size() < 400 ) {
        for( count = 0; count != 400; ++count ) {
            test_colony.insert( count );
        }
    }

    it1 = test_colony.begin();
    it2 = test_colony.end();

    advance( it1, 400 );
    test_colony.erase( it1, it2 );

    count = 0;

    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }


    CHECK( count == 400 );
    CHECK( test_colony.size() == 400 );

    unsigned int size;
    unsigned int range1;
    unsigned int range2;
    unsigned int loop_count;
    unsigned int internal_loop_count;
    for( loop_count = 0; loop_count < 50; ++loop_count ) {
        test_colony.clear();

        for( count = 0; count < 1000; ++count ) {
            test_colony.insert( count );
        }

        internal_loop_count = 0;

        while( !test_colony.empty() ) {
            it1 = test_colony.begin();
            it2 = test_colony.begin();

            size = static_cast<unsigned int>( test_colony.size() );
            range1 = xor_rand() % size;
            range2 = range1 + 1 + ( xor_rand() % ( size - range1 ) );
            advance( it1, static_cast<int>( range1 ) );
            advance( it2, static_cast<int>( range2 ) );
            if( range2 >= size ) {
                it2 = test_colony.end();
            }

            test_colony.erase( it1, it2 );

            count = 0;
            for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
                ++count;
            }


            CAPTURE( loop_count, internal_loop_count );
            CHECK( test_colony.size() == static_cast<unsigned int>( count ) );

            if( test_colony.size() > 2 ) {

                test_colony.insert( 1 );
                test_colony.insert( 10 );
            }

            ++internal_loop_count;
        }
    }


    CHECK( test_colony.empty() );

    for( loop_count = 0; loop_count < 50; ++loop_count ) {
        test_colony.clear();
        internal_loop_count = 0;

        test_colony.insert( 10000, 10 );

        while( !test_colony.empty() ) {
            it1 = test_colony.begin();
            it2 = test_colony.begin();

            size = static_cast<unsigned int>( test_colony.size() );
            range1 = xor_rand() % size;
            range2 = range1 + 1 + ( xor_rand() % ( size - range1 ) );
            advance( it1, static_cast<int>( range1 ) );
            advance( it2, static_cast<int>( range2 ) );
            if( range2 >= size ) {
                it2 = test_colony.end();
            }

            test_colony.erase( it1, it2 );

            count = 0;
            for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
                ++count;
            }


            CAPTURE( loop_count, internal_loop_count );
            CHECK( test_colony.size() == static_cast<unsigned int>( count ) );

            if( test_colony.size() > 100 ) {

                const unsigned int extra_size = xor_rand() % 128;
                test_colony.insert( extra_size, 5 );


                CAPTURE( loop_count, internal_loop_count );
                CHECK( test_colony.size() == static_cast<unsigned int>( count ) + extra_size );

                count = 0;
                for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
                    ++count;
                }


                CHECK( test_colony.size() == static_cast<unsigned int>( count ) );
            }

            ++internal_loop_count;
        }
    }


    CHECK( test_colony.empty() );

    test_colony.erase( test_colony.begin(), test_colony.end() );


    CHECK( test_colony.empty() );

    test_colony.insert( 10, 1 );
    test_colony.erase( test_colony.begin(), test_colony.begin() );


    CHECK( test_colony.size() == 10 );
}

TEST_CASE( "colony_sort", "[colony]" )
{
    cata::colony<int> test_colony;

    test_colony.reserve( 50000 );

    for( int i = 0; i < 50000; ++i ) {
        test_colony.insert( xor_rand() & 65535 );
    }

    test_colony.sort();

    bool sorted = true;
    int prev = 0;

    for( int cur : test_colony ) {
        if( prev > cur ) {
            sorted = false;
            break;
        }
        prev = cur;
    }


    CHECK( sorted );

    test_colony.sort( std::greater<>() );

    prev = 65536;

    for( int cur : test_colony ) {
        if( prev < cur ) {
            sorted = false;
            break;
        }
        prev = cur;
    }


    CHECK( sorted );
}

TEST_CASE( "colony_insertion_methods", "[colony]" )
{
    cata::colony<int> test_colony = {1, 2, 3};


    CHECK( test_colony.size() == 3 );

    cata::colony<int> test_colony_2( test_colony.begin(), test_colony.end() );


    CHECK( test_colony_2.size() == 3 );

    cata::colony<int> test_colony_3( 5000, 2, plf::limits( 100, 250 ) );


    CHECK( test_colony_3.size() == 5000 );

    test_colony_2.insert( 500000, 5 );


    CHECK( test_colony_2.size() == 500003 );

    std::vector<int> some_ints( 500, 2 );

    test_colony_2.insert( some_ints.begin(), some_ints.end() );


    CHECK( test_colony_2.size() == 500503 );

    test_colony_3.clear();
    test_colony_2.clear();
    test_colony_2.reserve( 50000 );
    test_colony_2.insert( 60000, 1 );

    int sum = 0;

    for( int it : test_colony_2 ) {
        sum += it;
    }


    CHECK( test_colony_2.size() == 60000 );
    CHECK( sum == 60000 );

    test_colony_2.clear();
    test_colony_2.reserve( 5000 );
    test_colony_2.insert( 60, 1 );

    sum = 0;

    for( int it : test_colony_2 ) {
        sum += it;
    }


    CHECK( test_colony_2.size() == 60 );
    CHECK( sum == 60 );

    test_colony_2.insert( 6000, 1 );

    sum = 0;

    for( int it : test_colony_2 ) {
        sum += it;
    }


    CHECK( test_colony_2.size() == 6060 );
    CHECK( sum == 6060 );

    test_colony_2.reserve( 18000 );
    test_colony_2.insert( 6000, 1 );

    sum = 0;
    for( int it : test_colony_2 ) {
        sum += it;
    }


    CHECK( test_colony_2.size() == 12060 );
    CHECK( sum == 12060 );
}

TEST_CASE( "colony_perfect_forwarding", "[colony]" )
{
    cata::colony<perfect_forwarding_test> test_colony;

    int lvalue = 0;
    int &lvalueref = lvalue;

    test_colony.emplace( 7, lvalueref );

    CHECK( ( *test_colony.begin() ).success );
    CHECK( lvalueref == 1 );
}

TEST_CASE( "colony_emplace", "[colony]" )
{
    cata::colony<small_struct> test_colony;
    int sum1 = 0;
    int sum2 = 0;

    for( int i = 0; i < 100; ++i ) {
        test_colony.emplace( i );
        sum1 += i;
    }

    for( small_struct &s : test_colony ) {
        sum2 += s.number;
    }


    CHECK( sum1 == sum2 );

    CHECK( test_colony.size() == 100 );
}

TEST_CASE( "colony_group_size_and_capacity", "[colony]" )
{
    cata::colony<int> test_colony;
    test_colony.reshape( plf::limits( 50, 100 ) );

    test_colony.insert( 27 );


    CHECK( test_colony.capacity() == 50 );

    for( int i = 0; i != 100; ++i ) {
        test_colony.insert( i );
    }


    CHECK( test_colony.capacity() == 200 );

    test_colony.clear();
    test_colony.reshape( plf::limits( 200, 255 ) );

    test_colony.insert( 27 );


    CHECK( test_colony.capacity() == 200 );

    for( int i = 0; i != 3300; ++i ) {
        test_colony.insert( i );
    }


    CHECK( test_colony.capacity() == 3460 );

    test_colony.reshape( plf::limits( 150, 150 ) );


    CHECK( test_colony.capacity() == 3450 );

    test_colony.reshape( plf::limits( 200, 200 ) );


    CHECK( test_colony.capacity() == 3400 );
}

TEST_CASE( "colony_splice", "[colony]" )
{
    cata::colony<int> test_colony_1;
    cata::colony<int> test_colony_2;

    SECTION( "small splice 1" ) {
        int i = 0;
        for( ; i < 20; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 20 );
        }

        test_colony_1.splice( test_colony_2 );

        i = 0;
        for( cata::colony<int>::iterator it = test_colony_1.begin(); it < test_colony_1.end(); ++it ) {
            CHECK( i++ == *it );
        }
    }

    SECTION( "small splice 2" ) {
        int i = 0;
        for( ; i < 100; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100 );
        }

        test_colony_1.splice( test_colony_2 );

        i = 0;
        for( int it : test_colony_1 ) {
            CHECK( i++ == it );
        }
    }

    SECTION( "large splice" ) {
        int i = 0;
        for( ; i < 100000; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100000 );
        }

        test_colony_1.splice( test_colony_2 );

        i = 0;
        for( int it : test_colony_1 ) {
            CHECK( i++ == it );
        }
    }

    SECTION( "erase and splice 1" ) {
        for( int i = 0; i < 100; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100 );
        }

        for( cata::colony<int>::iterator it = test_colony_2.begin(); it != test_colony_2.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony_2.erase( it );
            } else {
                ++it;
            }
        }

        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }
    }

    SECTION( "erase and splice 2" ) {
        for( int i = 0; i != 100; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100 );
        }

        for( cata::colony<int>::iterator it = test_colony_2.begin(); it != test_colony_2.end(); ) {
            if( ( xor_rand() & 3 ) == 0 ) {
                it = test_colony_2.erase( it );
            } else {
                ++it;
            }
        }

        for( cata::colony<int>::iterator it = test_colony_1.begin(); it != test_colony_1.end(); ) {
            if( ( xor_rand() & 1 ) == 0 ) {
                it = test_colony_1.erase( it );
            } else {
                ++it;
            }
        }

        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }
    }

    SECTION( "unequal size splice 1" ) {
        test_colony_1.reshape( plf::limits( 200, 200 ) );
        test_colony_2.reshape( plf::limits( 200, 200 ) );

        for( int i = 0; i < 100; ++i ) {
            test_colony_1.insert( i + 150 );
        }

        for( int i = 0; i < 150; ++i ) {
            test_colony_2.insert( i );
        }

        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }
    }

    SECTION( "unequal size splice 2" ) {
        test_colony_1.reset();
        test_colony_1.reshape( plf::limits( 200, 200 ) );
        test_colony_2.reset();
        test_colony_2.reshape( plf::limits( 200, 200 ) );

        for( int i = 0; i < 100; ++i ) {
            test_colony_1.insert( 100 - i );
        }

        for( int i = 0; i < 150; ++i ) {
            test_colony_2.insert( 250 - i );
        }

        test_colony_1.splice( test_colony_2 );

        int prev = 255;
        for( int it : test_colony_1 ) {
            CHECK( prev >= it );
            prev = it;
        }
    }

    SECTION( "large unequal size erase and splice" ) {
        for( int i = 0; i < 100000; ++i ) {
            test_colony_1.insert( i + 200000 );
        }

        for( int i = 0; i < 200000; ++i ) {
            test_colony_2.insert( i );
        }

        for( cata::colony<int>::iterator it = test_colony_2.begin(); it != test_colony_2.end(); ) {
            if( ( xor_rand() & 1 ) == 0 ) {
                it = test_colony_2.erase( it );
            } else {
                ++it;
            }
        }

        for( cata::colony<int>::iterator it = test_colony_1.begin(); it != test_colony_1.end(); ) {
            if( ( xor_rand() & 1 ) == 0 ) {
                it = test_colony_1.erase( it );
            } else {
                ++it;
            }
        }

        test_colony_1.erase( --test_colony_1.end() );
        test_colony_2.erase( --test_colony_2.end() );


        const size_t expected_size = test_colony_1.size() + test_colony_2.size();
        test_colony_1.splice( test_colony_2 );
        CHECK( test_colony_1.size() == expected_size );
        CHECK( test_colony_2.empty() );

        do {
            for( cata::colony<int>::iterator it = test_colony_1.begin(); it != test_colony_1.end(); ) {
                if( ( xor_rand() & 3 ) == 0 ) {
                    it = test_colony_1.erase( it );
                } else if( ( xor_rand() & 7 ) == 0 ) {
                    test_colony_1.insert( 433 );
                    ++it;
                } else {
                    ++it;
                }
            }

        } while( !test_colony_1.empty() );


        CHECK( test_colony_1.empty() );
    }
}
