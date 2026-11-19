#include "mod_manager.h" // IWYU pragma: associated

#include <algorithm>
#include <exception>

#include "a11y_runtime.h"
#include "color.h"
#include "debug.h"
#include "dependency_tree.h"
#include "output.h"
#include "string_formatter.h"

mod_ui::mod_ui( mod_manager &mman )
    : active_manager( mman )
    , mm_tree( active_manager.get_tree() )
{
}

std::string mod_ui::get_information( const MOD_INFORMATION *mod )
{
    if( mod == nullptr ) {
        return "";
    }

    std::string info;

    if( !mod->authors.empty() ) {
        info += colorize( n_gettext( "Author", "Authors", mod->authors.size() ),
                          c_light_blue ) + ": " + enumerate_as_string( mod->authors );
        if( mod->maintainers.empty() ) {
            info += "\n";
        } else {
            info += "  ";
        }
    }

    if( !mod->maintainers.empty() ) {
        info += colorize( n_gettext( "Maintainer", "Maintainers", mod->maintainers.size() ),
                          c_light_blue ) + u8":\u00a0" + enumerate_as_string( mod->maintainers ) + "\n";
    }

    if( !mod->dependencies.empty() ) {
        const auto &deps = mod->dependencies;
        std::string str = enumerate_as_string( deps.begin(), deps.end(), [&]( const mod_id & e ) {
            if( e.is_valid() ) {
                return string_format( "[%s]", e->name() );
            } else {
                return string_format( "[<color_red>%s</color>]", e.c_str() );
            }
        } );
        info += colorize( n_gettext( "Dependency", "Dependencies", deps.size() ),
                          c_light_blue ) + ": " + str + "\n";
    }

    if( !mod->version.empty() ) {
        info += colorize( _( "Mod version" ), c_light_blue ) + ": " + mod->version + "\n";
    }

    if( !mod->description.empty() ) {
        info += mod->description + "\n";
    }

    std::string note = !mm_tree.is_available( mod->ident ) ? mm_tree.get_node(
                           mod->ident )->s_errors() : "";
    if( !note.empty() ) {
        info += colorize( note, c_red );
    }

    return info;
}

std::optional<mod_id> mod_ui::find_mod_conflict( const mod_id &checked_mod,
        const std::vector<mod_id> &active_list )
{
    for( const mod_id &some_active_mod : active_list ) {
        for( const mod_id &conflict_mod_id : some_active_mod->conflicts ) {
            if( conflict_mod_id == checked_mod ) {
                return some_active_mod;
            }
        }
    }
    if( checked_mod.is_valid() ) {
        for( const mod_id &conflict_mod_id : checked_mod->conflicts ) {
            std::vector<mod_id>::const_iterator it = std::find( active_list.begin(),
                    active_list.end(), conflict_mod_id );
            if( it != active_list.end() ) {
                return *it;
            }
        }
    }
    return std::nullopt;
}

void mod_ui::try_add( const mod_id &mod_to_add,
                      std::vector<mod_id> &active_list )
{
    if( std::find( active_list.begin(), active_list.end(), mod_to_add ) != active_list.end() ) {

        return;
    }
    if( !mod_to_add.is_valid() ) {
        debugmsg( "Unable to load mod \"%s\".", mod_to_add.c_str() );
        return;
    }
    std::optional<mod_id> conflict = find_mod_conflict( mod_to_add, active_list );
    if( conflict ) {
        popup( _( "Unable to add %s.  It conflicts with %s." ),
               mod_to_add->name(), ( *conflict )->name() );
        return;
    }

    const MOD_INFORMATION &mod = *mod_to_add;
    bool errs;
    try {
        dependency_node *checknode = mm_tree.get_node( mod.ident );
        if( !checknode ) {
            return;
        }
        errs = checknode->has_errors();
    } catch( std::exception & ) {
        errs = true;
    }

    if( errs ) {

        return;
    }

    std::vector<mod_id> dependencies = mm_tree.get_dependencies_of_X_as_strings( mod.ident );


    if( mod.core ) {

        if( !active_list.empty() && active_list[0]->core && active_list[0] != mod_to_add ) {

            const std::string old_core_name = active_list[0]->name();
            try_rem( 0, active_list );
            if( a11y::enabled() ) {


                popup( _( "Removed the previous core mod: %s." ), old_core_name );
            }
        }


        active_list.insert( active_list.begin(), mod_to_add );
    } else {

        std::vector<mod_id> mods_to_add;
        bool new_core = false;
        for( auto &i : dependencies ) {
            if( std::find( active_list.begin(), active_list.end(), i ) == active_list.end() ) {
                if( i->core ) {
                    mods_to_add.insert( mods_to_add.begin(), i );
                    new_core = true;
                } else {
                    mods_to_add.push_back( i );
                }
            }
        }

        if( new_core && !active_list.empty() ) {



            const bool had_core = active_list[0].is_valid() && active_list[0]->core;
            const std::string old_core_name = had_core ? active_list[0]->name() : std::string();
            try_rem( 0, active_list );
            if( a11y::enabled() && had_core ) {
                popup( _( "Removed the previous core mod: %s." ), old_core_name );
            }
            active_list.insert( active_list.begin(), mods_to_add[0] );
            mods_to_add.erase( mods_to_add.begin() );
        }

        for( auto &i : mods_to_add ) {
            active_list.push_back( i );
        }

        active_list.push_back( mod.ident );
    }
}

void mod_ui::try_rem( size_t selection, std::vector<mod_id> &active_list )
{

    if( selection >= active_list.size() ) {

        return;
    }
    const mod_id sel_string = active_list[selection];

    const MOD_INFORMATION &mod = *active_list[selection];

    std::vector<mod_id> dependents = mm_tree.get_dependents_of_X_as_strings( mod.ident );


    std::vector<std::string> removed_dependents;
    for( auto &i : dependents ) {
        auto rem = std::find( active_list.begin(), active_list.end(), i );
        if( rem != active_list.end() ) {
            removed_dependents.push_back( i.is_valid() ? i->name() : i.str() );
            active_list.erase( rem );
        }
    }
    std::vector<mod_id>::iterator rem = std::find( active_list.begin(), active_list.end(),
                                        sel_string );
    if( rem != active_list.end() ) {
        active_list.erase( rem );
    }
    if( a11y::enabled() && !removed_dependents.empty() ) {




        popup( _( "Also removed from the load order: %s." ),
               enumerate_as_string( removed_dependents ) );
    }
}

void mod_ui::try_shift( char direction, size_t &selection, std::vector<mod_id> &active_list )
{

    if( selection >= active_list.size() ) {
        return;
    }


    size_t newsel = 0;
    size_t oldsel = 0;
    int selshift = 0;


    if( direction == '-' && can_shift_up( selection, active_list ) ) {

        newsel = selection - 1;
        oldsel = selection;

        selshift = -1;
    }

    else if( direction == '+' && can_shift_down( selection, active_list ) ) {
        newsel = selection;
        oldsel = selection + 1;

        selshift = +1;
    } else {
        return;
    }

    mod_id modstring = active_list[newsel];
    mod_id selstring = active_list[oldsel];



    active_list[newsel] = selstring;
    active_list[oldsel] = modstring;

    selection += selshift;
}

bool mod_ui::can_shift_up( size_t selection, const std::vector<mod_id> &active_list )
{

    if( selection >= active_list.size() ) {
        return false;
    }

    std::vector<mod_id> dependencies = mm_tree.get_dependencies_of_X_as_strings(
                                           active_list[selection] );


    if( selection == 0 ) {

        return false;
    }

    int newsel = selection - 1;

    mod_id newsel_id = active_list[newsel];
    bool newsel_is_dependency =
        std::find( dependencies.begin(), dependencies.end(), newsel_id ) != dependencies.end();

    return !newsel_id->core && !newsel_is_dependency;
}

bool mod_ui::can_shift_down( size_t selection, const std::vector<mod_id> &active_list )
{

    if( selection >= active_list.size() ) {
        return false;
    }
    std::vector<mod_id> dependents = mm_tree.get_dependents_of_X_as_strings(
                                         active_list[selection] );


    if( selection == active_list.size() - 1 ) {

        return false;
    }
    int newsel = selection;
    int oldsel = selection + 1;

    mod_id modstring = active_list[newsel];
    mod_id selstring = active_list[oldsel];
    bool sel_is_dependency =
        std::find( dependents.begin(), dependents.end(), selstring ) != dependents.end();

    return !modstring->core && !sel_is_dependency;
}
