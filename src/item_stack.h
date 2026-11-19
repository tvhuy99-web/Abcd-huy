#pragma once
#ifndef CATA_SRC_ITEM_STACK_H
#define CATA_SRC_ITEM_STACK_H

#include <cstddef>
#include <functional>
#include <list>
#include <memory>

#include "colony.h"
#include "coords_fwd.h"
#include "item.h" // IWYU pragma: keep
#include "type_id.h"
#include "units_fwd.h"

class map;









class item_stack
{
    protected:
        cata::colony<item> *items;

    public:
        using iterator = cata::colony<item>::iterator;
        using const_iterator = cata::colony<item>::const_iterator;
        using reverse_iterator = cata::colony<item>::reverse_iterator;
        using const_reverse_iterator = cata::colony<item>::const_reverse_iterator;

        explicit item_stack( cata::colony<item> *items ) : items( items ) { }
        virtual ~item_stack() = default;

        size_t size() const;
        bool empty() const;
        virtual void insert( map &here, const item &newitem ) = 0;
        virtual iterator erase( const_iterator it ) = 0;
        virtual void clear();

        item &only_item();



        iterator get_iterator_from_pointer( item *it );
        iterator get_iterator_from_index( size_t idx );
        size_t get_index_from_iterator( const const_iterator &it );

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        reverse_iterator rbegin();
        reverse_iterator rend();
        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;


        virtual int count_limit() const = 0;

        virtual units::volume max_volume() const = 0;

        virtual units::volume stored_volume() const;
        units::volume free_volume() const;







        int amount_can_fit( const item &it ) const;

        item *stacks_with( const item &it );
        const item *stacks_with( const item &it ) const;









        template <typename veh_or_map_cursor>
        std::list<item> use_charges( const itype_id &type, int &quantity, const tripoint_bub_ms &pos,
                                     const veh_or_map_cursor &cur,
                                     const std::function<bool( const item & )> &filter, bool in_tools = false );
};

#endif
