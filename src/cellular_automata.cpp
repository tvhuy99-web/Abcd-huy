#include "cellular_automata.h"

#include <algorithm>

#include "point.h"
#include "rng.h"

namespace CellularAutomata
{









int neighbor_count( const std::vector<std::vector<int>> &cells, const int width,
                    const int height, const point &p )
{
    int neighbors = 0;
    for( int ni = -1; ni <= 1; ni++ ) {
        for( int nj = -1; nj <= 1; nj++ ) {
            const point n( p + point( ni, nj ) );


            if( n.x < 0 || n.x >= width || n.y < 0 || n.y >= height ) {
                continue;
            }

            neighbors += cells[n.x][n.y];
        }
    }

    neighbors -= cells[p.x][p.y];

    return neighbors;
}

std::vector<std::vector<int>> generate_cellular_automaton(
                               const int width, const int height, const int alive, const int iterations,
                               const int birth_limit, const int stasis_limit )
{
    std::vector<std::vector<int>> current( width, std::vector<int>( height, 0 ) );
    std::vector<std::vector<int>> next( width, std::vector<int>( height, 0 ) );


    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            current[i][j] = x_in_y( alive, 100 );
        }
    }

    for( int iteration = 0; iteration < iterations; iteration++ ) {
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {


                if( i == 0 || i == width - 1 || j == 0 || j == height - 1 ) {
                    next[i][j] = 0;
                    continue;
                }


                const int neighbors = neighbor_count( current, width, height, point( i, j ) );



                const int to_live = current[i][j] ? stasis_limit : birth_limit;
                if( neighbors > to_live ) {
                    next[i][j] = 1;

                } else {
                    next[i][j] = 0;
                }
            }
        }


        std::swap( current, next );
    }

    return current;
}
}
