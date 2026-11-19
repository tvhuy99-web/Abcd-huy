#pragma once
#ifndef CATA_SRC_VEHICLE_SELECTOR_H
#define CATA_SRC_VEHICLE_SELECTOR_H

#include <climits>
#include <cstddef>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"
#include "visitable.h"

class map;
class vehicle;

class vehicle_cursor : public visitable
{
    public:
        vehicle_cursor( vehicle &veh, std::ptrdiff_t part, bool ignore_vpart = false ) : veh( veh ),
            part( part ), ignore_vpart( ignore_vpart ) {}
        vehicle &veh;
        std::ptrdiff_t part;
        bool ignore_vpart;


        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const override;
        int max_quality( const quality_id &qual ) const override;
        VisitResponse visit_items( const std::function<VisitResponse( const item_location & )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;
};

class vehicle_selector : public visitable
{
    public:
        using value_type = vehicle_cursor;
        using size_type = std::vector<value_type>::size_type;
        using iterator = std::vector<value_type>::iterator;
        using const_iterator = std::vector<value_type>::const_iterator;
        using reference = std::vector<value_type>::reference;
        using const_reference = std::vector<value_type>::const_reference;








        explicit vehicle_selector( map &here,  const tripoint_bub_ms &pos, int radius = 0,
                                   bool accessible = true,
                                   bool visibility_only = false );








        vehicle_selector( map &here, const tripoint_bub_ms &pos, int radius, bool accessible,
                          const vehicle &ignore );


        vehicle_selector( const vehicle_selector &that ) = delete;
        vehicle_selector &operator=( const vehicle_selector & ) = delete;
        vehicle_selector( vehicle_selector && ) = default;

        size_type size() const {
            return data.size();
        }
        iterator begin() {
            return data.begin();
        }
        iterator end() {
            return data.end();
        }
        const_iterator begin() const {
            return data.cbegin();
        }
        const_iterator end() const {
            return data.cend();
        }
        const_iterator cbegin() const {
            return data.cbegin();
        }
        const_iterator cend() const {
            return data.cend();
        }
        reference front() {
            return data.front();
        }
        const_reference front() const {
            return data.front();
        }
        reference back() {
            return data.back();
        }
        const_reference back() const {
            return data.back();
        }


        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const override;
        int max_quality( const quality_id &qual ) const override;
        VisitResponse visit_items( const std::function<VisitResponse( const item_location & )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;

    private:
        std::vector<value_type> data;
};

#endif
