#pragma once
#ifndef CATA_SRC_ACTIVE_ITEM_CACHE_H
#define CATA_SRC_ACTIVE_ITEM_CACHE_H

#include <cstddef>
#include <list>
#include <unordered_map>
#include <vector>

#include "coordinates.h"
#include "item_pocket.h"
#include "point.h"
#include "safe_reference.h"

class item;


struct item_reference {
    point_rel_ms location;
    safe_reference<item> item_ref;

    item *parent = nullptr;
    std::vector<item_pocket const *> pocket_chain;

    float spoil_multiplier() const;
    float insulation() const;
    bool has_watertight_container() const;
};

enum class special_item_type : int {
    none,
    corpse,
    explosive
};

template<typename T>
constexpr bool operator==( safe_reference<T> const &lhs, safe_reference<T> const &rhs )
{

    return !!lhs && !!rhs && lhs.get() == rhs.get();
}

namespace std
{
template <>
struct hash<special_item_type> {
    std::size_t operator()( const special_item_type &k ) const noexcept {
        return static_cast<size_t>( k );
    }
};
}

class active_item_cache
{
    private:
        std::unordered_map<int, std::list<item_reference>> active_items;
        std::unordered_map<special_item_type, std::list<item_reference>> special_items;
        std::unordered_map<int, std::unordered_map<item *, safe_reference<item>>> active_items_index;
    public:






        bool add( item &it, point_sm_ms location, item *parent = nullptr,
                  std::vector<item_pocket const *> const &pocket_chain = {} );
        bool add( item &it, point_rel_ms location, item *parent = nullptr,
                  std::vector<item_pocket const *> const &pocket_chain = {} );




        bool empty() const;





        std::vector<item_reference> get();









        std::vector<item_reference> get_for_processing();




        std::vector<item_reference> get_special( special_item_type type );

        void subtract_locations( const point_rel_ms &delta );
        void rotate_locations( int turns, const point_rel_ms &dim );
        void mirror( const point_rel_ms &dim, bool horizontally );
};

#endif
