#pragma once
#ifndef CATA_SRC_ITEM_COMPONENTS_H
#define CATA_SRC_ITEM_COMPONENTS_H

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "type_id.h"
#include "value_ptr.h"

class JsonOut;
class JsonValue;
class item;
template<typename T>
class ret_val;

class item_components
{
    private:
        cata::heap<std::map<itype_id, std::vector<item>>> comps;
        using comp_iterator = std::map<itype_id, std::vector<item>>::iterator;
        using const_comp_iterator = std::map<itype_id, std::vector<item>>::const_iterator;

    public:
        using type_vector_pair = std::pair<const itype_id, std::vector<item>>;

        comp_iterator begin();
        comp_iterator end();
        const_comp_iterator begin() const;
        const_comp_iterator end() const;
        bool empty();
        bool empty() const;
        void clear();

        std::vector<item> operator[]( itype_id it_id );



        item only_item();
        item only_item() const;


        size_t size() const;
        void add( item &new_it );
        ret_val<item> remove( itype_id it_id );

        item get_and_remove_random_entry();


        item_components split( int batch_size, size_t offset, bool is_cooked = false );

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonValue &jv );
};

#endif
