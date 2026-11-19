#pragma once
#ifndef CATA_SRC_HORDE_MAP_H
#define CATA_SRC_HORDE_MAP_H

#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "horde_entity.h"
#include "point.h"
#include "type_id.h"


namespace horde_map_flavors
{
enum {
    active = 1,
    idle = 1 << 1,
    dormant = 1 << 2,
    immobile = 1 << 3
};
}

class monster;











class horde_map
{
        using map_type = std::unordered_map
                         <tripoint_om_sm, std::unordered_map<tripoint_abs_ms, horde_entity>>;

        map_type active_monster_map;


        map_type idle_monster_map;
        map_type dormant_monster_map;
        map_type immobile_monster_map;
        point_abs_om location;

    public:
        using node_type = std::unordered_map<tripoint_abs_ms, horde_entity>::node_type;
        void set_location( point_abs_om loc ) {
            location = loc;
        }
        point_abs_om get_location() {
            return location;
        }
        horde_entity *entity_at( const tripoint_om_ms &p );
        std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> entity_group_at(
            const tripoint_om_omt &p, int filter = horde_map_flavors::active | horde_map_flavors::idle |
                    horde_map_flavors::dormant | horde_map_flavors::immobile );
        std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> entity_group_at(
            const tripoint_om_sm &p, int filter = horde_map_flavors::active | horde_map_flavors::idle |
                    horde_map_flavors::dormant | horde_map_flavors::immobile );
        std::optional<std::unordered_map<tripoint_abs_ms, horde_entity>::iterator>
        spawn_entity( const tripoint_abs_ms &p, mtype_id id );
        std::optional<std::unordered_map<tripoint_abs_ms, horde_entity>::iterator> spawn_entity(
            const tripoint_abs_ms &p,
            const monster &mon );
        void signal_entities( const tripoint_abs_ms &origin, int volume );
        void insert( node_type &&node );
        void clear();
        void clear_chunk( const tripoint_om_sm &p );

        class iterator
        {
                using iterator_category = std::forward_iterator_tag;
                using value_type = std::pair<const tripoint_abs_ms, horde_entity>;
                using difference_type = int;
                using pointer = std::pair<const tripoint_abs_ms, horde_entity> *;
                using reference = std::pair<const tripoint_abs_ms, horde_entity> &;
                horde_map *parent = nullptr;
                map_type *outer_map = nullptr;
                map_type::iterator outer_iter;
                std::unordered_map<tripoint_abs_ms, horde_entity>::iterator inner_iter;
                int filter = horde_map_flavors::active | horde_map_flavors::idle | horde_map_flavors::dormant |
                             horde_map_flavors::immobile;
            public:
                friend horde_map;


                explicit iterator() = default;
                explicit iterator( const horde_map &p ) : parent( const_cast<horde_map *>( &p ) ),
                    outer_map( &parent->active_monster_map ),
                    outer_iter( outer_map->begin() ) {
                    insure_valid();
                }
                explicit iterator( const horde_map &p, int filt ) : parent( const_cast<horde_map *>( &p ) ),
                    outer_map( &parent->active_monster_map ),
                    outer_iter( outer_map->begin() ),
                    filter( filt ) {
                    insure_valid();
                }

                explicit iterator( const horde_map &p, map_type &m, map_type::iterator oi,
                                   std::unordered_map<tripoint_abs_ms, horde_entity>::iterator ii ) : parent( const_cast<horde_map *>
                                               ( &p ) ), outer_map( &m ),
                    outer_iter( oi ), inner_iter( ii ) {}
                void next_map();
                void insure_valid();
                iterator &operator++();
                iterator operator++( int );
                bool operator==( iterator other ) const;
                bool operator!=( iterator other ) const;
                reference operator*() const;
                pointer operator->() const;
        };
        iterator begin() const {
            return iterator( *this );
        }
        iterator end() const {
            return iterator();
        }
        iterator begin() {
            return iterator( *this );
        }
        iterator end() {
            return iterator();
        }
        iterator find( const tripoint_om_ms &loc );
        iterator erase( iterator iter );
        node_type extract( iterator iter );

        class view_proxy
        {
                horde_map *parent;
                int filter;
            public:
                view_proxy( horde_map *p, int filt ) : parent( p ), filter( filt ) {}
                iterator begin() {
                    return iterator( *parent, filter );
                }
                iterator end() {
                    return parent->end();
                }
        };
        view_proxy get_view( int use_filter ) {
            return view_proxy( this, use_filter );
        }
};

#endif
