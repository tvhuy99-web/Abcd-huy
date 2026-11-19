#pragma once
#ifndef CATA_SRC_SIMPLE_PATHFINDING_H
#define CATA_SRC_SIMPLE_PATHFINDING_H

#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

#include "coords_fwd.h"
#include "omdata.h"
#include "point.h"

namespace pf
{




template<typename Point>
struct directed_node {
    Point pos;
    om_direction::type dir;

    explicit directed_node( Point pos,
                            om_direction::type dir = om_direction::type::invalid ) : pos( pos ), dir( dir ) {}
};






template<typename Point>
struct directed_path {
    std::vector<directed_node<Point>> nodes;
};






template<typename Point>
struct simple_path {

    std::vector<Point> points;

    int dist;

    int cost;
};


struct node_score {


    int node_cost;


    int estimated_dest_cost;

    node_score( int node_cost, int estimated_dest_cost );

    static const node_score rejected;
};



template<typename Point>
using two_node_scoring_fn =
    std::function<node_score( directed_node<Point>, std::optional<directed_node<Point>> )>;


directed_path<point> greedy_path( const point &source, const point &dest, const point &max,
                                  const two_node_scoring_fn<point> &scorer );










template<typename Point, typename = std::enable_if_t<Point::dimension == 2>>
directed_path<Point> greedy_path( const Point &source, const Point &dest, const Point &max,
                                  const two_node_scoring_fn<Point> &scorer )
{
    directed_path<Point> res;
    const two_node_scoring_fn<point> point_scorer
    = [scorer]( directed_node<point> current, std::optional<directed_node<point>> prev ) {
        std::optional<directed_node<Point>> prev_node;
        if( prev ) {
            prev_node = directed_node<Point>( Point( prev->pos ), prev->dir );
        }
        return scorer( directed_node<Point>( Point( current.pos ), current.dir ), prev_node );
    };
    directed_path<point> path = greedy_path( source.raw(), dest.raw(), max.raw(), point_scorer );
    res.nodes.reserve( path.nodes.size() );
    for( const auto &node : path.nodes ) {
        res.nodes.emplace_back( Point( node.pos ), node.dir );
    }
    return res;
}

struct omt_score {


    int node_cost;



    bool allow_z_change;

    explicit omt_score( int node_cost, bool allow_z_change = false );

    static const omt_score rejected;
};

using omt_scoring_fn = std::function<omt_score( tripoint_abs_omt )>;










simple_path<tripoint_abs_omt> find_overmap_path( const tripoint_abs_omt &source,
        const tripoint_abs_omt &dest, int radius, const omt_scoring_fn &scorer,
        const std::function<void( size_t, size_t )> &progress_fn,
        const std::optional<int> &max_cost = std::nullopt, bool allow_diagonal = false );

}

#endif
