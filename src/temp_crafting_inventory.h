#pragma once
#ifndef CATA_SRC_TEMP_CRAFTING_INVENTORY_H
#define CATA_SRC_TEMP_CRAFTING_INVENTORY_H

// IWYU pragma: no_include <memory>  // IWYU being silly
#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cata_utility.h"
#include "colony.h"
#include "coords_fwd.h"
#include "item.h"
#include "item_location.h"
#include "type_id.h"
#include "visitable.h"

class Character;
class map;
class map_cursor;
class vehicle_cursor;





class temp_crafting_inventory : public visitable
{
    public:







        class query_cache_scope
        {
            public:
                query_cache_scope();
                ~query_cache_scope();
                query_cache_scope( const query_cache_scope & ) = delete;
                query_cache_scope &operator=( const query_cache_scope & ) = delete;
        };

        temp_crafting_inventory() = default;
        temp_crafting_inventory( const temp_crafting_inventory &v );
        temp_crafting_inventory &operator=( const temp_crafting_inventory &v );

        size_t size() const;

        void clear();





        void add_item_loc( const item_location &loc );






        void add_all_ref( const read_only_visitable &v );
        void add_all_ref( const Character &guy );
        void add_all_ref( const map_cursor &cur );
        void add_all_ref( const vehicle_cursor &cur );






        item &add_item_copy( const item &item );
        item &add_pseudo_item( const itype_id &id );
        item &add_pseudo_item( const item &item );


        VisitResponse visit_items( const std::function<VisitResponse( const item_location & )> &func ) const
        override;
        int charges_of( const itype_id &what, int limit = INT_MAX,
                        const std::function<bool( const item & )> &filter = return_true<item>,
                        const std::function<void( int )> &visitor = nullptr,
                        bool in_tools = false ) const override;
        int amount_of( const itype_id &what, bool pseudo = true, int limit = INT_MAX,
                       const std::function<bool( const item & )> &filter = return_true<item> ) const
        override;


        int count_item( const itype_id &item_type ) const;

        void form_from_zone( map &m, std::unordered_set<tripoint_abs_ms> &zone_pts,
                             const Character *pl = nullptr );
        void form_from_map( const tripoint_bub_ms &origin, int range, const Character *pl = nullptr,
                            bool clear_path = true );
        void form_from_map( map *here, const tripoint_bub_ms &origin, int range,
                            const Character *pl = nullptr,
                            bool clear_path = true );
        void form_from_map( map &m, std::vector<tripoint_bub_ms> pts, const Character *pl );

        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;

        item remove_item( item &it );

        void dump( std::vector<item *> &dest ) const;


        bool must_use_liq_container( const itype_id &id, int to_use ) const;

        bool must_use_hallu_poison( const itype_id &id, int to_use ) const;
        void update_liq_container_count( const itype_id &id, int count );
        void replace_liq_container_count( const std::map<itype_id, int> &newmap, bool use_max = false );
    private:

        struct root_ref {
            item_location raw;
            item_location loc;
            item_location get() const;
            bool operator==( const root_ref &rhs ) const {
                return raw == rhs.raw && loc == rhs.loc;
            }
        };


        struct type_index {
            std::unordered_map<itype_id, std::vector<root_ref>> by_type;
            std::vector<root_ref> ups;
        };


        bool prepare_query_cache() const;

        const type_index *cached_index() const;

        void visit_roots_holding( const itype_id &id,
                                  const std::function<VisitResponse( item_location )> &func ) const;
        void drop_caches() const;

        std::optional<bool> recall_provider_quality( const provider_quality_key &key ) const override;
        void remember_provider_quality( const provider_quality_key &key, bool answer ) const override;

        mutable std::optional<type_index> index;
        mutable std::map<provider_quality_key, bool> provider_quality_answers;
        mutable uint64_t cache_epoch = 0;


        cata::colony<item_location> items_loc;

        cata::colony<item_location> item_copies;

        cata::colony<item> temp_owned_items;


        std::map<itype_id, int> max_empty_liq_cont;

        std::map<itype_id, item *> pseudo_items;
};

#endif
