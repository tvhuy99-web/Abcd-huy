#pragma once
#ifndef CATA_SRC_BALLISTICS_H
#define CATA_SRC_BALLISTICS_H

#include <cstddef>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "weakpoint.h"
#include "weighted_list.h"

class Creature;
class dispersion_sources;
class map;
class vehicle;
struct dealt_projectile_attack;
struct projectile;


struct projectile_attack_aim {

    double missed_by = 0;

    double missed_by_tiles = 0;

    double dispersion = 0;
};




projectile_attack_aim projectile_attack_roll( const dispersion_sources &dispersion, double range,
        double target_size );






void projectile_attack( dealt_projectile_attack &attack, const projectile &proj_arg,
                        const tripoint_bub_ms &source, const tripoint_bub_ms &target_arg,
                        const dispersion_sources &dispersion, Creature *origin = nullptr, const vehicle *in_veh = nullptr,
                        const weakpoint_attack &wp_attack = weakpoint_attack() );

void projectile_attack( dealt_projectile_attack &attack, const projectile &proj_arg,
                        map *here, const tripoint_bub_ms &source, const tripoint_bub_ms &target_arg,
                        const dispersion_sources &dispersion, Creature *origin = nullptr, const vehicle *in_veh = nullptr,
                        const weakpoint_attack &wp_attack = weakpoint_attack() );



















template<typename T, typename Wrapper>
class targeting_graph
{
        struct node {
            T val;
            double weight;
            std::vector<const node *> connections;
        };
        std::map<T, node> graph;
        T root;

        const node &add_node( const T &val ) {
            node added;
            added.val = val;
            added.weight = Wrapper::weight( val );
            return graph.emplace( val, added ).first->second;
        }

        void add( const T &from, const T &to ) {
            graph[from].connections.emplace_back( &add_node( to ) );
        }

    public:
        void generate( const T &center, const std::vector<T> &parts ) {
            add_node( center );
            root = center;


            std::map<T, std::set<T>> connects_to;
            for( const T &cursor : parts ) {

                if( Wrapper::connection( cursor ) == cursor ) {
                    continue;
                }

                auto result = connects_to.emplace( Wrapper::connection( cursor ), std::set<T>() );

                result.first->second.emplace( cursor );
            }

            std::list<T> next;
            next.push_back( center );
            while( !next.empty() ) {
                const T current = next.front();
                next.pop_front();

                const auto connected_iter = connects_to.find( current );
                if( connected_iter != connects_to.end() ) {
                    for( const T &cursor : connected_iter->second ) {
                        add( current, cursor );
                        next.push_back( cursor );
                    }
                    connects_to.erase( connected_iter );
                }

                for( std::pair<const T, std::set<T>> &entry : connects_to ) {
                    if( entry.second.find( current ) == entry.second.end() ) {
                        continue;
                    }
                    next.push_back( entry.first );
                    add( current, entry.first );
                    entry.second.erase( current );
                }
            }
        }

        T select( const double range_min, const double range_max, double value ) const {


            std::list<const node *> path;

            path.push_back( &graph.at( root ) );

            while( !path.back()->connections.empty() ) {

                weighted_float_list<size_t> next;
                const std::vector<const node *> &connections = path.back()->connections;
                for( size_t i = 0; i < connections.size(); ++i ) {
                    next.add( i, connections[i]->weight );
                }
                path.push_back( connections[*next.pick()] );
            }





            value -= range_min;


            double total_weight = 0.0;
            for( const node *nd : path ) {
                total_weight += nd->weight;
            }



            double scale_factor = ( range_max - range_min ) / total_weight;

            double accumulated_weight = 0.0;
            for( const node *nd : path ) {
                accumulated_weight += nd->weight * scale_factor;

                if( accumulated_weight > value ) {
                    return nd->val;
                }
            }


            return path.back()->val;
        }
};

#endif
