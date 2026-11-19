#include "shadowcasting.h"

#include <cstdint>
#include <cstdlib>
#include <iterator>

#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "fragment_cloud.h" // IWYU pragma: keep
#include "line.h"
#include "list.h"
#include "point.h"




namespace
{
struct slope {
    slope( int_least8_t rise, int_least8_t run ) {

        this->run = std::abs( run );
        if( run < 0 ) {
            this->rise = -rise;
        } else {
            this->rise = rise;
        }
    }


    int_least8_t rise;
    int_least8_t run;
};

bool operator<( const slope &lhs, const slope &rhs )
{

    return lhs.rise * rhs.run < rhs.rise * lhs.run;
}
bool operator>( const slope &lhs, const slope &rhs )
{
    return rhs < lhs;
}
bool operator==( const slope &lhs, const slope &rhs )
{

    return lhs.rise * rhs.run == rhs.rise * lhs.run;
}

template<typename T>
struct span {
    span( const slope &s_major, const slope &e_major,
          const slope &s_minor, const slope &e_minor,
          const T &value, bool skip_first_row = false, bool skip_first_column = false ) :
        start_major( s_major ), end_major( e_major ), start_minor( s_minor ), end_minor( e_minor ),
        cumulative_value( value ), skip_first_row( skip_first_row ),
        skip_first_column( skip_first_column ) {}
    slope start_major;
    slope end_major;
    slope start_minor;
    slope end_minor;
    T cumulative_value;
    bool skip_first_row;
    bool skip_first_column;
};
}





template<typename T, bool( *is_transparent )( const T &, const T & ), T( *accumulate )( const T &, const T &, const int & )>
static void split_span( cata::list<span<T>> &spans,
                        typename cata::list<span<T>>::iterator &this_span,
                        T &current_transparency, const T &new_transparency, const T &last_intensity,
                        const int distance, slope &new_start_minor,
                        const slope &trailing_edge_major, const slope &leading_edge_major,
                        const slope &trailing_edge_minor, const slope &leading_edge_minor )
{
    const T next_cumulative_transparency = accumulate( this_span->cumulative_value,
                                           current_transparency, distance );

























    if( is_transparent( current_transparency, last_intensity ) ) {


        if( trailing_edge_major > this_span->start_major ) {
            spans.emplace( this_span,
                           this_span->start_major, trailing_edge_major,
                           this_span->start_minor, this_span->end_minor,
                           next_cumulative_transparency, false, this_span->skip_first_column );
        }



        if( trailing_edge_minor > this_span->start_minor ) {
            spans.emplace( this_span,
                           std::max( this_span->start_major, trailing_edge_major ),
                           std::min( this_span->end_major, leading_edge_major ),
                           this_span->start_minor, trailing_edge_minor,
                           next_cumulative_transparency, false, this_span->skip_first_column );
        }


        new_start_minor = trailing_edge_minor;
    }



    if( leading_edge_major < this_span->end_major ) {



        spans.emplace( std::next( this_span ),
                       leading_edge_major, this_span->end_major,
                       this_span->start_minor, this_span->end_minor,
                       this_span->cumulative_value, true, this_span->skip_first_column );
    }


    if( is_transparent( current_transparency, last_intensity ) &&
        is_transparent( new_transparency, last_intensity ) ) {
        this_span->skip_first_column = true;
    } else {
        this_span->skip_first_column = false;

    }


    this_span->start_major = std::max( this_span->start_major, trailing_edge_major );
    this_span->end_major = std::min( this_span->end_major, leading_edge_major );



    this_span->start_minor = new_start_minor;

    new_start_minor = leading_edge_minor;
    current_transparency = new_transparency;
}

template<int xx_transform, int xy_transform, int yx_transform, int yy_transform, int z_transform, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *is_transparent )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
static void cast_horizontal_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &offset, const int offset_distance,
    const T numerator )
{
    const int radius = MAX_VIEW_DISTANCE - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;
    static half_open_rectangle<point_bub_ms> bounds( point_bub_ms::zero, point_bub_ms( MAPSIZE_X,
            MAPSIZE_Y ) );

    slope new_start_minor( 1, 1 );

    T last_intensity( 0.0 );
    tripoint_rel_ms delta;
    tripoint_bub_ms current;





    cata::list<span<T>> spans = { {
            slope( 0, 1 ), slope( 1, 1 ),
            slope( 0, 1 ), slope( 1, 1 ),
            T( LIGHT_TRANSPARENCY_OPEN_AIR )
        }
    };


    for( int distance = 1; distance <= radius; distance++ ) {
        delta.y() = distance;
        T current_transparency( 0.0f );

        for( auto this_span = spans.begin(); this_span != spans.end(); ) {
            bool started_block = false;

            for( delta.z() = 0; delta.z() <= distance; delta.z()++ ) {
















                const slope trailing_edge_major( delta.z() * 2 - 1, delta.y() * 2 + 1 );
                const slope leading_edge_major( delta.z() * 2 + 1, delta.y() * 2 - 1 );
                current.z() = offset.z() + delta.z() * z_transform;
                if( current.z() > max_z || current.z() < min_z ) {

                    continue;
                }
                if( this_span->start_major > leading_edge_major ) {


                    continue;
                }
                if( this_span->skip_first_row && this_span->start_major == leading_edge_major ) {







                    continue;
                }
                if( this_span->end_major < trailing_edge_major ) {


                    break;
                }

                bool started_span = false;
                const int z_index = current.z() + OVERMAP_DEPTH;
                for( delta.x() = 0; delta.x() <= distance; delta.x()++ ) {
                    current.x() = offset.x() + delta.x() * xx_transform + delta.y() * xy_transform;
                    current.y() = offset.y() + delta.x() * yx_transform + delta.y() * yy_transform;

                    const slope trailing_edge_minor( delta.x() * 2 - 1, delta.y() * 2 + 1 );
                    const slope leading_edge_minor( delta.x() * 2 + 1, delta.y() * 2 - 1 );

                    if( !bounds.contains( current.xy() ) ) {

                        continue;
                    }
                    if( this_span->start_minor > leading_edge_minor ) {

                        continue;
                    }
                    if( this_span->skip_first_column && this_span->start_minor == leading_edge_minor ) {



                        continue;
                    }

                    if( this_span->end_minor < trailing_edge_minor ) {

                        break;
                    }

                    T new_transparency = ( *input_arrays[z_index] )[current.x()][current.y()];





                    bool floor_block = false;
                    if( current.z() < offset.z() ) {
                        if( ( *floor_caches[z_index + 1] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    } else if( current.z() > offset.z() ) {
                        if( ( *floor_caches[z_index] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    }

                    if( !started_block ) {
                        started_block = true;
                        current_transparency = new_transparency;
                    }

                    const int dist = rl_dist( tripoint_rel_ms::zero, delta ) + offset_distance;
                    last_intensity = calc( numerator, this_span->cumulative_value, dist );

                    if( !floor_block ) {
                        ( *output_caches[z_index] )[current.x()][current.y()] =
                            std::max( ( *output_caches[z_index] )[current.x()][current.y()], last_intensity );
                    }

                    if( !started_span ) {

                        new_start_minor = leading_edge_minor;
                        started_span = true;
                    }

                    if( new_transparency == current_transparency ) {

                        new_start_minor = leading_edge_minor;
                        continue;
                    }


                    split_span<T, is_transparent, accumulate>( spans, this_span, current_transparency,
                            new_transparency, last_intensity,
                            distance, new_start_minor,
                            trailing_edge_major, leading_edge_major,
                            trailing_edge_minor, leading_edge_minor );
                }



                if( !is_transparent( current_transparency, last_intensity ) ) {
                    this_span->start_major = leading_edge_major;
                }
            }

            if(


                !started_block ||




                !is_transparent( current_transparency, last_intensity )
            ) {
                this_span = spans.erase( this_span );
            } else {

                this_span->cumulative_value = accumulate( this_span->cumulative_value,
                                              current_transparency, distance );
                ++this_span;
            }
        }
    }
}

template<int x_transform, int y_transform, int z_transform, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *is_transparent )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
static void cast_vertical_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &offset, const int offset_distance,
    const T numerator )
{
    const int radius = MAX_VIEW_DISTANCE - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;

    slope new_start_minor( 1, 1 );

    T last_intensity( 0.0 );
    tripoint_rel_ms delta;
    tripoint_bub_ms current;





    cata::list<span<T>> spans = { {
            slope( 0, 1 ), slope( 1, 1 ),
            slope( 0, 1 ), slope( 1, 1 ),
            T( LIGHT_TRANSPARENCY_OPEN_AIR )
        }
    };


    for( int distance = 1; distance <= radius; distance++ ) {
        delta.z() = distance;
        T current_transparency( 0.0f );

        for( auto this_span = spans.begin(); this_span != spans.end(); ) {
            bool started_block = false;
            for( delta.y() = 0; delta.y() <= distance; delta.y()++ ) {

                const slope trailing_edge_major( delta.y() * 2 - 1, delta.z() * 2 + 1 );
                const slope leading_edge_major( delta.y() * 2 + 1, delta.z() * 2 - 1 );
                current.y() = offset.y() + delta.y() * y_transform;
                if( current.y() < 0 || current.y() >= MAPSIZE_Y ) {

                    continue;
                }
                if( this_span->start_major > leading_edge_major ) {


                    continue;
                }
                if( this_span->skip_first_row && this_span->start_major == leading_edge_major ) {







                    continue;
                }
                if( this_span->end_major < trailing_edge_major ) {


                    break;
                }

                bool started_span = false;
                for( delta.x() = 0; delta.x() <= distance; delta.x()++ ) {
                    current.x() = offset.x() + delta.x() * x_transform;
                    current.z() = offset.z() + delta.z() * z_transform;

                    const slope trailing_edge_minor( delta.x() * 2 - 1, delta.z() * 2 + 1 );
                    const slope leading_edge_minor( delta.x() * 2 + 1, delta.z() * 2 - 1 );

                    if( current.x() < 0 || current.x() >= MAPSIZE_X ||
                        current.z() > max_z || current.z() < min_z ) {

                        continue;
                    }
                    if( this_span->start_minor > leading_edge_minor ) {

                        continue;
                    }
                    if( this_span->end_minor < trailing_edge_minor ) {

                        break;
                    }

                    const int z_index = current.z() + OVERMAP_DEPTH;

                    T new_transparency = ( *input_arrays[z_index] )[current.x()][current.y()];



                    bool floor_block = false;
                    if( current.z() < offset.z() ) {
                        if( ( *floor_caches[z_index + 1] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    } else if( current.z() > offset.z() ) {
                        if( ( *floor_caches[z_index] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    }

                    if( !started_block ) {
                        started_block = true;
                        current_transparency = new_transparency;
                    }

                    const int dist = rl_dist( tripoint_rel_ms::zero, delta ) + offset_distance;
                    last_intensity = calc( numerator, this_span->cumulative_value, dist );

                    if( !floor_block ) {
                        ( *output_caches[z_index] )[current.x()][current.y()] =
                            std::max( ( *output_caches[z_index] )[current.x()][current.y()], last_intensity );
                    }

                    if( !started_span ) {

                        new_start_minor = leading_edge_minor;
                        started_span = true;
                    }

                    if( new_transparency == current_transparency ) {

                        new_start_minor = leading_edge_minor;
                        continue;
                    }


                    split_span<T, is_transparent, accumulate>( spans, this_span, current_transparency,
                            new_transparency, last_intensity,
                            distance, new_start_minor,
                            trailing_edge_major, leading_edge_major,
                            trailing_edge_minor, leading_edge_minor );
                }



                if( !is_transparent( current_transparency, last_intensity ) ) {
                    this_span->start_major = leading_edge_major;
                }
            }

            if(


                !started_block ||




                !is_transparent( current_transparency, last_intensity )
            ) {
                this_span = spans.erase( this_span );
            } else {

                this_span->cumulative_value = accumulate( this_span->cumulative_value,
                                              current_transparency, distance );
                ++this_span;
            }
        }
    }
}

template<typename T, T( *calc )( const T &, const T &, const int & ),
         bool( *is_transparent )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, const int offset_distance, const T numerator,
    vertical_direction dir )
{
    if( dir == vertical_direction::DOWN || dir == vertical_direction::BOTH ) {




        cast_horizontal_zlight_segment < 0, 1, 1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 1, 0, 0, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 0, -1, 1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < -1, 0, 0, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 0, 1, -1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 1, 0, 0, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 0, -1, -1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < -1, 0, 0, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );




        cast_vertical_zlight_segment < 1, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );


        cast_vertical_zlight_segment < 1, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );


        cast_vertical_zlight_segment < -1, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );


        cast_vertical_zlight_segment < -1, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    }

    if( dir == vertical_direction::UP || dir == vertical_direction::BOTH ) {




        cast_horizontal_zlight_segment < 0, 1, 1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 1, 0, 0, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 0, -1, 1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < -1, 0, 0, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 0, 1, -1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 1, 0, 0, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < 0, -1, -1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );



        cast_horizontal_zlight_segment < -1, 0, 0, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );




        cast_vertical_zlight_segment < 1, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );


        cast_vertical_zlight_segment < 1, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );


        cast_vertical_zlight_segment < -1, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );


        cast_vertical_zlight_segment < -1, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    }
}



template void cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
    const array_of_grids_of<float> &output_caches,
    const array_of_grids_of<const float> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, int offset_distance, float numerator,
    vertical_direction dir );

template void cast_zlight<fragment_cloud, shrapnel_calc, shrapnel_check, accumulate_fragment_cloud>(
    const array_of_grids_of<fragment_cloud> &output_caches,
    const array_of_grids_of<const fragment_cloud> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, int offset_distance, fragment_cloud numerator,
    vertical_direction dir );
