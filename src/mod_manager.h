#pragma once
#ifndef CATA_SRC_MOD_MANAGER_H
#define CATA_SRC_MOD_MANAGER_H

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_path.h"
#include "output.h"
#include "pimpl.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class dependency_tree;
class mod_manager;
struct WORLD;

const std::vector<std::pair<std::string, translation>> &get_mod_list_categories();
const std::vector<std::pair<std::string, translation>> &get_mod_list_tabs();
const std::map<std::string, std::string> &get_mod_list_cat_tab();

mod_id get_mod_base_id_from_src( mod_id src );

struct MOD_INFORMATION {
    private:
        friend mod_manager;
        translation name_;

    public:
        std::string name() const;

        mod_id ident;


        cata_path path;


        std::set<std::string> authors;





        std::set<std::string> maintainers;


        std::set<std::string> loading_images;


        bool disable_other_loading_screens;

        translation description;
        std::string version;


        std::vector<mod_id> dependencies;


        std::vector<mod_id> conflicts;


        bool core = false;


        bool obsolete = false;

        std::pair<int, translation> category = { -1, translation() };
};


template<typename src_id>
std::string get_origin( const std::vector<std::pair<src_id, mod_id>> &src )
{
    std::string origin_str = enumerate_as_string( src.begin(),
    src.end(), []( const std::pair<src_id, mod_id> &source ) {
        return string_format( "'%s'", source.second->name() );
    }, enumeration_conjunction::arrow );
    return string_format( _( "Origin: %s" ), origin_str );
}

struct mod_migrations {
    static void load( const JsonObject &jo );
    static void check();
    static void reset();
};

class mod_manager
{
    public:
        using t_mod_list = std::vector<mod_id>;

        mod_manager();
        ~mod_manager();




        void refresh_mod_list();

        std::vector<mod_id> all_mods() const;





        dependency_tree &get_tree();



        void clear();










        bool copy_mod_contents( const t_mod_list &mods_to_copy, const cata_path &output_base_path );





        void save_mods_list( const WORLD *world ) const;




        void load_mods_list( WORLD *world ) const;




        void check_mods_list( WORLD *world ) const;
        const t_mod_list &get_default_mods() const;
        bool set_default_mods( const t_mod_list &mods );
        const std::vector<mod_id> &get_usable_mods() const {
            return usable_mods;
        }

    private:

        friend class mod_ui;
        friend class worldfactory;
        friend mod_id;




        static cata_path get_mods_list_file( const WORLD *world );






        void load_mods_from( const cata_path &path );




        void load_mod_info( const cata_path &info_file_path );





        void load_modfile( const JsonObject &jo, const cata_path &path );

        bool set_default_mods( const mod_id &ident );

        pimpl<dependency_tree> tree;




        std::map<mod_id, MOD_INFORMATION> mod_map;
        t_mod_list default_mods;

        std::vector<mod_id> usable_mods;

        void set_usable_mods();
};

class mod_ui
{
    public:
        explicit mod_ui( mod_manager &mman );

        std::string get_information( const MOD_INFORMATION *mod );
        mod_manager &active_manager;
        dependency_tree &mm_tree;

        void try_add( const mod_id &mod_to_add,
                      std::vector<mod_id> &active_list );
        void try_rem( size_t selection, std::vector<mod_id> &active_list );
        void try_shift( char direction, size_t &selection, std::vector<mod_id> &active_list );

        std::optional<mod_id> find_mod_conflict( const mod_id &checked_mod,
                const std::vector<mod_id> &active_list );

        bool can_shift_up( size_t selection, const std::vector<mod_id> &active_list );
        bool can_shift_down( size_t selection, const std::vector<mod_id> &active_list );
};

#endif
