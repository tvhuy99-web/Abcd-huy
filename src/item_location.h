#pragma once
#ifndef CATA_SRC_ITEM_LOCATION_H
#define CATA_SRC_ITEM_LOCATION_H

#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "coords_fwd.h"
#include "pocket_type.h"
#include "units_fwd.h"
#include "visitable.h"

class Character;
class JsonObject;
class JsonOut;
class const_talker;
class item;
class item_pocket;
class pocket_constraint;
class map;
class map_cursor;
class talker;
class temp_crafting_inventory;
class vehicle_cursor;





item &null_item_reference();

template<typename T> class ret_val;







class item_location : public visitable
{
    public:
        enum class type : int {
            invalid = 0,
            character = 1,
            map = 2,
            vehicle = 3,
            container = 4,

            crafting_inventory = 5
        };

        item_location();

        static const item_location nowhere;

        item_location( Character &ch, item *which );
        item_location( const map_cursor &mc, item *which );
        item_location( const vehicle_cursor &vc, item *which );
        item_location( const item_location &container, item *which );
        item_location( temp_crafting_inventory &inv, item *which );


        void serialize( JsonOut &js ) const;
        void deserialize( const JsonObject &obj );

        bool operator==( const item_location &rhs ) const;
        bool operator!=( const item_location &rhs ) const;

        bool operator<( const item_location &rhs ) const;

        explicit operator bool() const;

        item &operator*();
        const item &operator*() const;

        item *operator->();
        const item *operator->() const;


        type where() const;

        bool valid() const;



        type where_recursive() const;


        tripoint_bub_ms pos_bub( const map &here ) const;
        tripoint_abs_ms pos_abs() const;



        std::string describe( const Character *ch = nullptr ) const;









        item_location obtain( Character &ch, int qty = -1 );



        int obtain_cost( const Character &ch, int qty = -1 ) const;



        void remove_item( item & = null_item_reference() );


        void on_contents_changed();

        void make_active();


        item *get_item();
        const item *get_item() const;

        void set_should_stack( bool should_stack ) const;


        item_location parent_item() const;
        item_pocket *parent_pocket() const;


        Character *carrier() const;


        const vehicle_cursor *veh_cursor() const;


        bool held_by( Character const &who ) const;






        bool has_parent() const;




        bool is_efile() const;





        bool is_invisible_installed_gunmod() const;




        units::volume volume_capacity() const;




        units::mass weight_capacity() const;




        bool check_parent_capacity_recursive() const;




        bool protected_from_liquids() const;





        pocket_constraint get_pocket_constraints_recursive( const item_pocket *pocket = nullptr ) const;
        ret_val<void> parents_can_contain_recursive( item *it ) const;
        ret_val<int> max_charges_by_parent_recursive( const item &it ) const;




        bool eventually_contains( item_location loc ) const;




        void overflow( map &here );






        bool can_reload_with( const item_location &ammo, bool now ) const;






        int get_quality( const std::string &quality, bool strict_boiling ) const;




        void favorite_settings_menu();

        VisitResponse visit_contents( const std::function<VisitResponse( const item_location & )> &func,
                                      const std::set<pocket_type> &allowed_pockets = {pocket_type::CONTAINER} ) const;
        VisitResponse visit_items( const std::function<VisitResponse( const item_location & )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item &e )> &filter,
                                           int count = INT_MAX ) override;
    private:
        class impl;

        std::shared_ptr<impl> ptr;
};
std::unique_ptr<talker> get_talker_for( item_location &it );
std::unique_ptr<const_talker> get_const_talker_for( const item_location &it );
std::unique_ptr<talker> get_talker_for( item_location *it );

namespace std
{
template <>
struct hash<item_location> {
    std::size_t operator()( const item_location &it ) const noexcept {
        return static_cast<size_t>( it.where() );
    }
};
}

struct item_locator_hint;



item_location find_item_by_uid( int64_t uid, const item_locator_hint &hint );

using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

#endif
