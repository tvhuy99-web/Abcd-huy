#pragma once
#ifndef CATA_SRC_ITEM_FACTORY_H
#define CATA_SRC_ITEM_FACTORY_H

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "generic_factory.h"
#include "item.h"
#include "itype.h"
#include "iuse.h"
#include "type_id.h"
#include "units_fwd.h"

class Item_group;
class Item_spawn_data;
class relic;
class translation;

namespace cata
{
template <typename T> class value_ptr;
}





bool item_is_blacklisted( const itype_id &id );

using item_action_id = std::string;
using Item_list = std::vector<item>;

class Item_factory;
class JsonArray;
class JsonObject;

extern std::unique_ptr<Item_factory> item_controller;




class migration
{
    public:
        itype_id id;
        std::string variant;
        std::optional<std::string> from_variant;
        itype_id replace;
        std::set<std::string> flags;
        int charges = 0;


        bool reset_item_vars = false;

        class content
        {
            public:
                itype_id id;
                int count = 0;

                bool operator==( const content & ) const;
                void deserialize( const JsonObject &jsobj );
        };
        std::vector<content> contents;
        bool sealed = true;
};

struct item_blacklist_t {
    std::set<itype_id> blacklist;

    std::vector<std::pair<bool, std::set<itype_id>>> sub_blacklist;

    void clear();
};










class Item_factory
{
        friend class generic_factory<itype>;
        friend struct itype;
    public:
        generic_factory<itype> item_factory = generic_factory<itype>( "ITEM" );

        Item_factory();
        ~Item_factory();

        generic_factory<itype> &get_generic_factory() {
            return item_factory;
        }


        void init();



        void reset();





        void check_definitions() const;














        void load_item_group( const JsonObject &jsobj );












        void load_item_group( const JsonObject &jsobj, const item_group_id &group_id,
                              const std::string &subtype, std::string context = {} );









        void load_item_group_data( const JsonObject &jsobj, Item_group *ig,
                                   const std::string &subtype );


















        void load_item_group( const JsonArray &entries, const item_group_id &group_id,
                              bool is_collection, int ammo_chance, int magazine_chance,
                              std::string context = {} );



        Item_spawn_data *get_group( const item_group_id & );



        std::vector<item_group_id> get_all_group_names();





        void check_and_create_magazine_pockets( itype &def );



        void add_special_pockets( itype &def );


        void finalize();


        void load_migration( const JsonObject &jo );


        void add_migration( const migration &m );


        itype_id migrate_id( const itype_id &id );







        void migrate_item( const itype_id &id, item &obj );


        void migrate_item_from_variant( item &obj, const std::string &from_variant );







        const itype *add_runtime( const itype_id &id, translation name, translation description ) const;





        bool has_template( const itype_id &id ) const;







        const itype *find_template( const itype_id &id ) const;

        static inline std::map<item_action_id, use_function> iuse_function_list;
        static std::set<std::string> repair_actions;

        static use_function usage_from_string( const std::string &type );

        static use_function read_use_function( const JsonObject &jo, std::map<std::string, int> &ammo_scale,
                                               std::string &type );


        static void add_iuse( const std::string &type, use_function_pointer f );
        static void add_iuse( const std::string &type, use_function_pointer f,
                              const translation &info );
        static void add_actor( std::unique_ptr<iuse_actor> ptr );

        std::map<itype_id, std::vector<migration>> migrations;




        static bool has_iuse( const item_action_id &type );

        void load_item_blacklist( const JsonObject &json );


        const std::vector<const itype *> &all() const;


        static std::vector<const itype *> find( const std::function<bool( const itype & )> &func );





        std::pair<std::vector<item>::const_iterator, std::vector<item>::const_iterator>
        get_armor_containers( units::volume min_volume ) const;

        std::list<itype_id> subtype_replacement( const itype_id & ) const;

    private:

        bool frozen = false;

        mutable std::map<itype_id, std::unique_ptr<itype>> m_runtimes;

        mutable bool m_runtimes_dirty = true;
        mutable std::vector<const itype *> templates_all_cache;

        using GroupMap = std::map<item_group_id, std::unique_ptr<Item_spawn_data>>;
        GroupMap m_template_groups;

        std::unordered_map<itype_id, ammotype> migrated_ammo;
        std::unordered_map<itype_id, itype_id> migrated_magazines;







        mutable std::vector<item> armor_containers;
        mutable std::vector<units::volume> volumes;







        bool check_ammo_type( std::string &msg, const ammotype &ammo ) const;

        template<typename SlotType>
        static void load_slot( const JsonObject &jo, bool was_loaded,
                               cata::value_ptr<SlotType> &slotptr );

        void load( relic &slot, const JsonObject &jo, std::string_view src );


        void emplace_usage( std::map<std::string, use_function> &container,
                            const std::string &iuse_id );














        bool load_sub_ref( std::unique_ptr<Item_spawn_data> &ptr, const JsonObject &obj,
                           const std::string &name, const Item_group &parent );
        bool load_string( std::vector<std::string> &vec, const JsonObject &obj, std::string_view name );
        void add_entry( Item_group &ig, const JsonObject &obj, const std::string &context );


        static void npc_implied_flags( itype &item_template );
        static void set_allergy_flags( itype &item_template );

        void clear();

        void finalize_item_blacklist();


        void finalize_pre( itype &obj );

        void register_cached_uses( const itype &obj );
        using repair_materials_by_tool = std::map<itype_id, std::set<material_id>>;

        void finalize_post( itype &obj, const repair_materials_by_tool &materials_by_tool );

        void finalize_post_armor( itype &obj );





        std::map<itype_id, std::set<itype_id>> tool_subtypes;


        std::set<itype_id> repair_tools;


        std::set<itype_id> gun_tools;
};

namespace items
{

void load( const JsonObject &jo, const std::string &src );

void finalize_all();

void reset();

void check_consistency();
}
#endif
