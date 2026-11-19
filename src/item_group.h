#pragma once
#ifndef CATA_SRC_ITEM_GROUP_H
#define CATA_SRC_ITEM_GROUP_H

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "enums.h"
#include "global_vars.h"
#include "item.h"
#include "relic.h"
#include "type_id.h"
#include "value_ptr.h"

class Item_spawn_data;
class JsonObject;
class JsonValue;
class time_point;
struct itype;
template <typename E> struct enum_traits;

enum class spawn_flags {
    none = 0,
    maximized = 1,
    use_spawn_rate = 2,
};

template<>
struct enum_traits<spawn_flags> {
    static constexpr bool is_flag_enum = true;
};

namespace item_group
{






item item_from( const item_group_id &group_id, const time_point &birthday );



item item_from( const item_group_id &group_id );

std::string potential_items( const item_group_id &group_id );

using ItemList = std::vector<item>;















ItemList items_from( const item_group_id &group_id, const time_point &birthday,
                     spawn_flags flags = spawn_flags::none );



ItemList items_from( const item_group_id &group_id );



bool group_contains_item( const item_group_id &group_id, const itype_id & );



std::set<const itype *> every_possible_item_from( const item_group_id &group_id );





bool group_is_defined( const item_group_id &group_id );



Item_spawn_data *spawn_data_from_group( const item_group_id &group_id );



void load_item_group( const JsonObject &jsobj, const item_group_id &group_id,
                      const std::string &subtype );






















item_group_id load_item_group( const JsonValue &value, const std::string &default_subtype,
                               const std::string &context );
}





class Item_spawn_data
{
    public:
        using ItemList = std::vector<item>;
        using RecursionList = std::vector<item_group_id>;

        enum class overflow_behaviour {
            none,
            spill,
            discard,
            last
        };

        Item_spawn_data( int _probability, const std::string &context, holiday _event = holiday::none ) :
            probability( _probability ), context_( context ), event( _event ) { }
        virtual ~Item_spawn_data() = default;









        virtual std::size_t create( ItemList &list, const time_point &birthday, RecursionList &rec,
                                    spawn_flags = spawn_flags::none ) const = 0;
        std::size_t create( ItemList &list, const time_point &birthday,
                            spawn_flags = spawn_flags::none ) const;




        virtual item create_single( const time_point &birthday, RecursionList &rec ) const = 0;
        item create_single( const time_point &birthday ) const;




        virtual void check_consistency( bool actually_spawn ) const;




        virtual bool remove_item( const itype_id &itemid ) = 0;
        virtual void replace_items( const std::unordered_map<itype_id, itype_id> &replacements ) = 0;
        virtual bool has_item( const itype_id &itemid ) const = 0;

        virtual std::set<const itype *> every_item() const = 0;
        virtual std::map<const itype *, std::pair<int, int>> every_item_min_max() const = 0;

        const std::string &context() const {
            return context_;
        }

        int get_probability( bool skip_event_check ) const;
        void set_probability( int prob ) {
            probability = prob;
        }
        bool is_event_based() const {
            return event != holiday::none;
        }




        std::optional<itype_id> container_item;
        std::optional<std::string> container_item_variant;
        overflow_behaviour on_overflow = overflow_behaviour::none;



        std::optional<std::vector<itype_id>> components_items;
        bool sealed = true;
        std::optional<bool> active = std::nullopt;

        struct relic_generator {
            relic_procgen_data::generation_rules rules;
            relic_procgen_id id;

            relic generate_relic( const itype_id &it_id ) const;

            bool was_loaded = false;
            void load( const JsonObject &jo );
        };

        cata::value_ptr<relic_generator> artifact;

    protected:

        int probability;


        std::string context_;

        holiday event = holiday::none;
};

template<>
struct enum_traits<Item_spawn_data::overflow_behaviour> {
    static constexpr Item_spawn_data::overflow_behaviour last =
        Item_spawn_data::overflow_behaviour::last;
};







class Item_modifier
{
    public:
        std::pair<int, int> damage;
        std::pair<int, int> count;




        std::pair<int, int> dirt;
        std::pair<int, int> charges;




        std::unique_ptr<Item_spawn_data> ammo;









        std::unique_ptr<Item_spawn_data> container;



        std::unique_ptr<Item_spawn_data> contents;
        bool sealed = true;



        std::vector<flag_id> custom_flags;




        std::string variant;




        std::vector<std::pair<fault_id, int>> faults;




        global_variables::impl_t item_vars;




        std::vector<snippet_id> snippets;

        Item_modifier();
        Item_modifier( Item_modifier && ) = default;

        void modify( item &new_item, const std::string &context ) const;
        void check_consistency( const std::string &context ) const;
        bool remove_item( const itype_id &itemid );
        void replace_items( const std::unordered_map<itype_id, itype_id> &replacements ) const;




        int with_ammo;

        int with_magazine;
};














class Single_item_creator : public Item_spawn_data
{
    public:
        enum Type {
            S_ITEM_GROUP,
            S_ITEM,
            S_NONE,
        };

        Single_item_creator( const std::string &id, Type type, int probability,
                             const std::string &context, holiday event = holiday::none );
        ~Single_item_creator() override = default;




        std::string id;
        Type type;
        std::optional<Item_modifier> modifier;

        void inherit_ammo_mag_chances( int ammo, int mag );

        std::size_t create( ItemList &list, const time_point &birthday, RecursionList &rec,
                            spawn_flags ) const override;
        item create_single( const time_point &birthday, RecursionList &rec ) const override;
        item create_single_without_container( const time_point &birthday, RecursionList &rec ) const;
        void check_consistency( bool actually_spawn ) const override;
        bool remove_item( const itype_id &itemid ) override;
        void replace_items( const std::unordered_map<itype_id, itype_id> &replacements ) override;

        bool has_item( const itype_id &itemid ) const override;
        std::set<const itype *> every_item() const override;
        std::map<const itype *, std::pair<int, int>> every_item_min_max() const override;
};





class Item_group : public Item_spawn_data
{
    public:
        using ItemList = std::vector<item>;
        enum Type {
            G_COLLECTION,
            G_DISTRIBUTION
        };

        Item_group( Type type, int probability, int ammo_chance, int magazine_chance,
                    const std::string &context, holiday event = holiday::none );
        ~Item_group() override = default;

        const Type type;








        using prop_list = std::vector<std::unique_ptr<Item_spawn_data> >;

        void add_item_entry( const itype_id &itemid, int probability,
                             const std::string &variant = "" );
        void add_group_entry( const item_group_id &groupid, int probability );





        void add_entry( std::unique_ptr<Item_spawn_data> ptr );
        std::size_t create( ItemList &list, const time_point &birthday, RecursionList &rec,
                            spawn_flags ) const override;
        item create_single( const time_point &birthday, RecursionList &rec ) const override;
        void check_consistency( bool actually_spawn ) const override;
        bool remove_item( const itype_id &itemid ) override;
        void replace_items( const std::unordered_map<itype_id, itype_id> &replacements ) override;
        bool has_item( const itype_id &itemid ) const override;
        std::set<const itype *> every_item() const override;
        std::map<const itype *, std::pair<int, int>> every_item_min_max() const override;







        const int with_ammo;

        const int with_magazine;

    protected:





        const Item_spawn_data *pick_distribution_entry() const;




        int sum_prob;



        prop_list items;




        mutable std::vector<int> cached_cum_prob;
};

#endif
