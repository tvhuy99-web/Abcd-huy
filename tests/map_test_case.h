#ifndef CATA_TESTS_MAP_TEST_CASE_H
#define CATA_TESTS_MAP_TEST_CASE_H

#include <functional>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "coordinates.h"
#include "point.h"
#include "type_id.h"



























class map_test_case
{
    public:

        struct tile {

            char setup_c;

            char expect_c;

            tripoint_bub_ms p;

            point p_local;
        };




        std::vector<std::string> setup;



        std::vector<std::string> expected_results;









        std::optional<char> anchor_char = std::nullopt;






        tripoint_bub_ms anchor_map_pos = tripoint_bub_ms::zero;





        void for_each_tile( const std::function<void( tile )> &callback );





        std::vector<std::vector<std::string>> map_tiles_str( const
                                           std::function<void( tile, std::ostringstream &out )> &callback );





        template<typename R>
        std::vector<std::vector<R>> map_tiles( const std::function<R( tile )> &callback );


        int get_height() const;


        int get_width() const;


        tripoint_bub_ms get_origin();


        void set_anchor_char_from( const std::set<char> &chars );




        void transpose();


        void reflect_x();


        void reflect_y();








        std::string generate_transform_combinations();







        void validate_anchor_point( const tripoint_bub_ms &p );

    private:


        std::optional<tripoint_bub_ms> origin = std::nullopt;


        bool checks_complete = false;

        void do_internal_checks();

        void for_each_tile( const tripoint_bub_ms &tmp_origin,
                            const std::function<void( tile & )> &callback ) const;

};


namespace map_test_case_common
{
using tile_predicate = std::function<bool( map_test_case::tile )>;






tile_predicate operator+(
    const std::function<void( map_test_case::tile )> &f,
    const std::function<void( map_test_case::tile )> &g );





tile_predicate operator&&( const tile_predicate &f, const tile_predicate &g );





tile_predicate operator||( const tile_predicate &f, const tile_predicate &g );

namespace tiles
{












tile_predicate ifchar( char c, const tile_predicate &f );







tile_predicate ter_set(
    ter_str_id ter,
    tripoint shift = tripoint::zero
);





inline const tile_predicate fail = []( map_test_case::tile t )
{
    FAIL( "Setup char is not handled: " << t.setup_c );
    return false;
};

inline const tile_predicate noop = []( map_test_case::tile )
{
    return true;
};

}

namespace printers
{

std::string format_2d_array( const std::vector<std::vector<std::string>> &info );


std::string fields( map_test_case &t, int zshift = 0 );

std::string transparency( map_test_case &t, int zshift = 0 );

std::string seen( map_test_case &t, int zshift = 0 );

std::string lm( map_test_case &t, int zshift = 0 );

std::string apparent_light( map_test_case &t, int zshift = 0 );

std::string obstructed( map_test_case &t, int zshift = 0 );

std::string floor( map_test_case &t, int zshift = 0 );

std::string expected( map_test_case &t );

}

}


template<typename R>
std::vector<std::vector<R>> map_test_case::map_tiles(
                             const std::function<R( map_test_case::tile )> &callback )
{
    std::vector<std::vector<R>> ret;

    for_each_tile( [&]( tile t ) -> void {
        if( t.p_local.x == 0 )
        {
            ret.push_back( std::vector<R>() );
        }
        ret[t.p_local.y].push_back( callback( t ) );
    } );

    return ret;
}

#endif
