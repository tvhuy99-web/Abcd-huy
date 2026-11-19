#pragma once
#ifndef CATA_SRC_CELLULAR_AUTOMATA_H
#define CATA_SRC_CELLULAR_AUTOMATA_H

#include <vector>

struct point;

namespace CellularAutomata
{










int neighbor_count( const std::vector<std::vector<int>> &cells, int width,
                    int height, const point &p );

















std::vector<std::vector<int>> generate_cellular_automaton(
                               int width, int height, int alive, int iterations,
                               int birth_limit, int stasis_limit );

}

#endif
