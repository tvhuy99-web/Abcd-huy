#pragma once
#ifndef CATA_SRC_VISITABLE_H
#define CATA_SRC_VISITABLE_H

#include <climits>
#include <functional>
#include <list>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "type_id.h"

class Character;
class item;
class item_location;

using item_filter = std::function<bool( const item & )>;

enum class VisitResponse : int {
    ABORT,
    NEXT,
    SKIP
};



enum class quality_count : int { providers, units };





class read_only_visitable
{
    public:











        virtual VisitResponse visit_items(
            const std::function<VisitResponse( const item_location & )> &func ) const = 0;






        item *find_parent( const item &it ) const;





        std::vector<item *> parents( const item &it ) const;


        bool has_item( const item &it ) const;


        bool has_item_with( const item_filter &filter ) const;


        virtual bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const;




        bool has_provider_quality( const quality_id &qual, int level, int qty,
                                   const Character *who,
                                   quality_count mode = quality_count::providers ) const;


        virtual int max_quality( const quality_id &qual ) const;

        std::pair<int, int> kcal_range( const itype_id &id,
                                        const item_filter &filter, Character &player_character ) const;







        virtual int charges_of( const itype_id &what, int limit = INT_MAX,
                                const item_filter &filter = return_true<item>,
                                const std::function<void( int )> &visitor = nullptr, bool in_tools = false ) const;









        virtual int amount_of( const itype_id &what, bool pseudo = true,
                               int limit = INT_MAX,
                               const item_filter &filter = return_true<item> ) const;


        bool has_amount( const itype_id &what, int qty, bool pseudo = true,
                         const item_filter &filter = return_true<item> ) const;


        std::vector<item *> items_with( const item_filter &filter );
        std::vector<const item *> items_with( const item_filter &filter ) const;

        virtual ~read_only_visitable() = default;

        bool has_tools( const itype_id &it, int quantity,
                        const item_filter &filter = return_true<item> ) const;
        bool has_components( const itype_id &it, int quantity,
                             const item_filter &filter = return_true<item> ) const;

        virtual bool has_charges( const itype_id &it, int quantity,
                                  const item_filter &filter = return_true<item> ) const;

    protected:
        struct provider_quality_key {
            quality_id qual;
            int level = 0;
            int qty = 0;
            const Character *who = nullptr;
            quality_count mode = quality_count::providers;
            bool operator<( const provider_quality_key &rhs ) const {
                if( std::tie( qual, level, qty, mode ) != std::tie( rhs.qual, rhs.level, rhs.qty,
                        rhs.mode ) ) {
                    return std::tie( qual, level, qty, mode ) <
                           std::tie( rhs.qual, rhs.level, rhs.qty, rhs.mode );
                }
                return std::less<>()( who, rhs.who );
            }
        };


        virtual std::optional<bool> recall_provider_quality( const provider_quality_key & ) const {
            return std::nullopt;
        }
        virtual void remember_provider_quality( const provider_quality_key &, bool ) const {}
};






class visitable : public read_only_visitable
{
    public:










        virtual std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                int count = INT_MAX ) = 0;


        item remove_item( item &it );
};

#endif
