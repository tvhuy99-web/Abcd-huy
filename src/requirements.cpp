#include "requirements.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "debug_menu.h"
#include "enum_traits.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "item.h"
#include "item_components.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_tname.h"
#include "itype.h"
#include "json.h"
#include "localized_comparator.h"
#include "output.h"
#include "pocket_type.h"
#include "string_formatter.h"
#include "temp_crafting_inventory.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"

static const flag_id json_flag_UNRECOVERABLE( "UNRECOVERABLE" );

static const itype_id itype_UPS( "UPS" );
static const itype_id itype_char_forge( "char_forge" );
static const itype_id itype_crucible( "crucible" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_forge( "forge" );
static const itype_id itype_oxy_torch( "oxy_torch" );
static const itype_id itype_press( "press" );
static const itype_id itype_welder( "welder" );
static const itype_id itype_welder_crude( "welder_crude" );

static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_CUT( "CUT" );
static const quality_id qual_GLARE( "GLARE" );
static const quality_id qual_KNIT( "KNIT" );
static const quality_id qual_PULL( "PULL" );
static const quality_id qual_SAW_M_FINE( "SAW_M_FINE" );
static const quality_id qual_SEW( "SEW" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static std::map<requirement_id, requirement_data> requirements_all;

static bool a_satisfies_b( const quality_requirement &a, const quality_requirement &b );
static bool a_satisfies_b( const std::vector<quality_requirement> &a,
                           const std::vector<quality_requirement> &b );

int quality_for_crafter( const item &it, const quality_id &qual, const Character *who,
                         const bool strict_boiling )
{

    if( strict_boiling && qual == qual_BOIL && !it.empty_container() ) {
        return INT_MIN;
    }

    int result = INT_MIN;
    const auto qit = it.type->qualities.find( qual );
    if( qit != it.type->qualities.end() ) {
        result = qit->second.level;
    }
    if( !it.type->charged_qualities.empty() && it.ammo_sufficient( who ) ) {
        const auto cit = it.type->charged_qualities.find( qual );
        if( cit != it.type->charged_qualities.end() ) {
            result = std::max( result, cit->second.level );
        }
    }
    return result;
}

int provider_quality_level( const item &cand, const quality_id &id, const Character *who,
                            const bool strict_boiling )
{
    int best = quality_for_crafter( cand, id, who, strict_boiling );



    for( int pk = 0; pk < static_cast<int>( pocket_type::LAST ); ++pk ) {
        const pocket_type pk_type = static_cast<pocket_type>( pk );
        if( pk_type == pocket_type::CONTAINER || !cand.has_pocket_type( pk_type ) ) {
            continue;
        }
        for( const item *nested : cand.all_items_ptr( pk_type ) ) {
            best = std::max( best, quality_for_crafter( *nested, id, who, strict_boiling ) );
        }
    }

    return best;
}


template<>
bool string_id<requirement_data>::is_valid() const
{
    return requirements_all.count( *this );
}


template<>
const requirement_data &string_id<requirement_data>::obj() const
{
    const auto found = requirements_all.find( *this );
    if( found == requirements_all.end() ) {
        debugmsg( "Tried to get invalid requirements: %s", c_str() );
        static const requirement_data null_requirement{};
        return null_requirement;
    }
    return found->second;
}

std::vector<requirement_data> requirement_data::get_all()
{
    std::vector<requirement_data> ret;
    ret.reserve( requirements_all.size() );
    for( const std::pair<const requirement_id, requirement_data> &pair : requirements_all ) {
        ret.push_back( pair.second );
    }
    return ret;
}

namespace
{
generic_factory<quality> quality_factory( "tool quality" );
}

void quality::reset()
{
    quality_factory.reset();
}

void quality::load_static( const JsonObject &jo, const std::string &src )
{
    quality_factory.load( jo, src );
}

void quality::finalize_all()
{
    quality_factory.finalize();
}

void quality::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name );

    for( JsonArray levels : jo.get_array( "usages" ) ) {
        const int level = levels.get_int( 0 );
        for( const std::string line : levels.get_array( 1 ) ) {
            usages.emplace_back( level, line );
        }
    }
}


template<>
const quality &string_id<quality>::obj() const
{
    return quality_factory.obj( *this );
}


template<>
bool string_id<quality>::is_valid() const
{
    return quality_factory.is_valid( *this );
}

std::string quality_requirement::to_string( const int, const int ) const
{
    //~ %1$d: tool count, %2$s: quality requirement name, %3$d: quality level requirement
    return string_format( n_gettext( "%1$d tool with %2$s of %3$d or more.",
                                     "%1$d tools with %2$s of %3$d or more.", count ),
                          count, type.obj().name, level );
}

std::string quality_requirement::to_colored_string() const
{
    //~ %1$d: tool count, %2$s: quality requirement name, %3$d: quality level requirement
    return string_format( n_gettext( "%1$d tool with <info>%2$s of %3$d</info> or more",
                                     "%1$d tools with <info>%2$s of %3$d</info> or more", count ),
                          count, type.obj().name, level );
}

bool tool_comp::by_charges() const
{
    return count > 0;
}

std::string tool_comp::to_string( const int batch, const int ) const
{
    if( by_charges() ) {
        int charge_total = count * batch;
        //~ %1$s: tool name, %2$d: charge requirement
        return string_format( npgettext( "requirement", "%1$s (%2$d charge)", "%1$s (%2$d charges)",
                                         charge_total ),
                              item::tname( type, 1, tname::base_item_name ), charge_total );
    } else {
        return item::tname( type, std::abs( count ), tname::base_item_name );
    }
}

std::string item_comp::to_string( const int batch, const int avail ) const
{
    const int num_wanted = std::abs( count ) * batch;
    const item item_temp = item( type );
    if( item_temp.count_by_charges() ) {


        if( avail == item::INFINITE_CHARGES ) {
            //~ %1$s: item name, %2$s: charge requirement (number or weight or volume of item, depending on type)
            return string_format( npgettext( "requirement", "%2$s %1$s (have infinite)",
                                             "%2$s %1$s (have infinite)",
                                             num_wanted ),
                                  item_temp.tname( 1, tname::base_item_name ),
                                  type->item_measure_prefix( num_wanted ) );
        } else if( avail > 0 ) {
            //~ %1$s: item name, %2$s: charge requirement (number or weight or volume of item, depending on type), %3%s: available charges (number or weight or volume of item, depending on type)
            return string_format( npgettext( "requirement", "%2$s %1$s (have %3$s)",
                                             "%2$s %1$s (have %3$s)", num_wanted ),
                                  item_temp.tname( 1, tname::base_item_name ),
                                  type->item_measure_prefix( num_wanted ),
                                  type->item_measure_prefix( avail ) );
        } else {
            //~ %1$s: item name, %2$s: charge requirement (number or weight or volume of item, depending on type)
            return string_format( npgettext( "requirement", "%2$s %1$s", "%2$s %1$s", num_wanted ),
                                  item_temp.tname( 1, tname::base_item_name ), type->item_measure_prefix( num_wanted ) );
        }
    } else {
        if( avail == item::INFINITE_CHARGES ) {
            //~ %1$s: item name, %2$s: required count (number or weight or volume of item, depending on type)
            return string_format( npgettext( "requirement", "%2$s %1$s (have infinite)",
                                             "%2$s %1$s (have infinite)",
                                             num_wanted ),
                                  item_temp.tname( num_wanted, tname::base_item_name ),
                                  type->item_measure_prefix( num_wanted ) );
        } else if( avail > 0 ) {
            //~ %1$s: item name, %2$s: required count (number or weight or volume of item, depending on type), %3%d: available count (number or weight or volume of item, depending on type)
            return string_format( npgettext( "requirement", "%2$s %1$s (have %3$s)",
                                             "%2$s %1$s (have %3$s)", num_wanted ),
                                  item_temp.tname( num_wanted, tname::base_item_name ),
                                  type->item_measure_prefix( num_wanted ),
                                  type->item_measure_prefix( avail ) );
        } else {
            //~ %1$s: item name, %2$s: required count (number or weight or volume of item, depending on type)
            return string_format( npgettext( "requirement", "%2$s %1$s", "%2$s %1$s", num_wanted ),
                                  item_temp.tname( num_wanted, tname::base_item_name ),
                                  type->item_measure_prefix( num_wanted ) );
        }
    }
}

void quality_requirement::load( const JsonValue &value )
{
    const JsonObject quality_data = value.get_object();
    type = quality_id( quality_data.get_string( "id" ) );
    level = quality_data.get_int( "level", 1 );
    count = quality_data.get_int( "amount", 1 );
    if( count <= 0 ) {
        quality_data.throw_error_at( "amount", "quality amount must be a positive number" );
    }

}

void quality_requirement::dump( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "id", type );
    if( level != 1 ) {
        jsout.member( "level", level );
    }
    if( count != 1 ) {
        jsout.member( "amount", count );
    }
    jsout.end_object();
}

void tool_comp::load( const JsonValue &value )
{
    if( value.test_string() ) {

        value.read( type, true );
        count = -1;
    } else {
        JsonArray comp = value.get_array();
        comp.read( 0, type, true );
        count = comp.get_int( 1 );
        requirement = comp.size() > 2 && comp.get_string( 2 ) == "LIST";
        if( comp.size() > 2 && comp.get_string( 2 ) != "LIST" ) {
            value.throw_error( R"(expected "LIST")" );
        }
    }
    if( count == 0 ) {
        value.throw_error( "tool count must not be 0" );
    }

}

void tool_comp::dump( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( type );
    jsout.write( count );
    if( requirement ) {
        jsout.write( "LIST" );
    }
    jsout.end_array();
}

void item_comp::load( const JsonValue &value )
{
    JsonArray comp = value.get_array();
    comp.read( 0, type, true );
    count = comp.get_int( 1 );
    size_t handled = 2;
    while( comp.size() > handled ) {
        const std::string &flag = comp.get_string( handled++ );
        if( flag == "NO_RECOVER" ) {
            recoverable = false;
        } else if( flag == "LIST" ) {
            requirement = true;
        } else {
            value.throw_error( R"(expected "LIST" or "NO_RECOVER")" );
        }
    }
    if( count <= 0 ) {
        value.throw_error( "item count must be a positive number" );
    }
}

void item_comp::dump( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( type );
    jsout.write( count );
    if( !recoverable ) {
        jsout.write( "NO_RECOVER" );
    }
    if( requirement ) {
        jsout.write( "LIST" );
    }
    jsout.end_array();
}

template<typename T>
void requirement_data::load_obj_list( const JsonArray &jsarr, std::vector< std::vector<T> > &objs )
{
    for( const JsonValue entry : jsarr ) {
        if( entry.test_array() ) {
            std::vector<T> choices;
            for( const JsonValue subentry : entry.get_array() ) {
                choices.push_back( T() );
                choices.back().load( subentry );
            }
            if( !choices.empty() ) {
                objs.push_back( choices );
            }
        } else {


            objs.push_back( std::vector<T>( 1 ) );
            objs.back().back().load( entry );
        }
    }
}

requirement_data requirement_data::operator*( unsigned scalar ) const
{
    requirement_data res = *this;
    for( auto &group : res.components ) {
        for( item_comp &e : group ) {
            e.count = std::max( e.count * static_cast<int>( scalar ), -1 );
        }
    }



    for( auto &group : res.tools ) {
        for( tool_comp &e : group ) {
            e.count = std::max( e.count * static_cast<int>( scalar ), -1 );
        }
    }

    return res;
}

static bool a_satisfies_b( const quality_requirement &a, const quality_requirement &b )
{
    return a.type == b.type && a.level >= b.level
           && a.requirement == b.requirement && a.count == b.count;
}

static bool a_satisfies_b( const std::vector<quality_requirement> &a,
                           const std::vector<quality_requirement> &b )
{

    for( const quality_requirement &b_x : b ) {
        bool satisfied = false;
        for( const quality_requirement &a_x : a ) {
            if( a_satisfies_b( a_x, b_x ) ) {
                satisfied = true;
                break;
            }
        }
        if( !satisfied ) {
            return false;
        }
    }

    return true;
}

requirement_data requirement_data::operator+( const requirement_data &rhs ) const
{
    requirement_data res = *this;

    res.components.insert( res.components.end(), rhs.components.begin(), rhs.components.end() );
    res.tools.insert( res.tools.end(), rhs.tools.begin(), rhs.tools.end() );
















    for( const std::vector<quality_requirement> &new_quality : rhs.qualities ) {
        bool add = true;
        for( std::vector<quality_requirement> &old_quality : res.qualities ) {
            if( a_satisfies_b( new_quality, old_quality ) ) {
                add = false;
                old_quality = new_quality;
                break;
            } else if( a_satisfies_b( old_quality, new_quality ) ) {
                add = false;
                break;
            }
        }
        if( add ) {
            res.qualities.emplace_back( new_quality );
        }
    }


    res.id_ = requirement_id::NULL_ID();




    res.blacklisted |= rhs.blacklisted;

    return res;
}

requirement_data requirement_data::operator+(
    const std::pair<const requirement_id, int> &rhs ) const
{
    return *this + *rhs.first * rhs.second;
}

requirement_data requirement_data::operator+( const std::pair<requirement_id, int> &rhs ) const
{
    return *this + *rhs.first * rhs.second;
}

void requirement_data::load_requirement( const JsonObject &jsobj, const requirement_id &id,
        const bool check_extend, const bool is_abstract )
{
    requirement_data req;
    requirement_data ext;

    if( check_extend && jsobj.has_object( "extend" ) ) {
        JsonObject jext = jsobj.get_object( "extend" );
        if( jext.has_member( "components" ) ) {
            load_obj_list( jext.get_array( "components" ), ext.components );
        }
        if( jext.has_member( "qualities" ) ) {
            load_obj_list( jext.get_array( "qualities" ), ext.qualities );
        }
        if( jext.has_member( "tools" ) ) {
            load_obj_list( jext.get_array( "tools" ), ext.tools );
        }
    }

    if( ext.components.empty() || jsobj.has_member( "components" ) ) {
        load_obj_list( jsobj.get_array( "components" ), req.components );
        if( is_abstract && !req.components.empty() ) {
            debugmsg( "Abstract recipe %s has components, which cannot be inherited.  "
                      "This is probably an error.", id.str() );
        }
    }
    if( ext.qualities.empty() || jsobj.has_member( "qualities" ) ) {
        load_obj_list( jsobj.get_array( "qualities" ), req.qualities );
        if( is_abstract && !req.qualities.empty() ) {
            debugmsg( "Abstract recipe %s has qualities, which cannot be inherited.  "
                      "This is probably an error.", id.str() );
        }
    }
    if( ext.tools.empty() || jsobj.has_member( "tools" ) ) {
        load_obj_list( jsobj.get_array( "tools" ), req.tools );
        if( is_abstract && !req.tools.empty() ) {
            debugmsg( "Abstract recipe %s has tools, which cannot be inherited.  "
                      "This is probably an error.", id.str() );
        }
    }

    if( !id.is_null() ) {
        req.id_ = id;
    } else if( jsobj.has_string( "id" ) ) {
        req.id_ = requirement_id( jsobj.get_string( "id" ) );
    } else {
        jsobj.throw_error( "id was not specified for requirement" );
    }



    if( id.is_null() && jsobj.has_member( "name" ) ) {
        jsobj.read( "name", req.name_ );
    }

    save_requirement( req, string_id<requirement_data>::NULL_ID(), &ext );
}

void requirement_data::save_requirement( const requirement_data &req, const requirement_id &id,
        const requirement_data *extend )
{
    requirement_data dup = req;
    if( !id.is_null() ) {
        dup.id_ = id;
    }

    if( requirements_all.count( dup.id_ ) == 0 ) {
        requirements_all[ dup.id_ ] = dup;
    }

    requirement_data &r = requirements_all[ dup.id_ ];
    if( !dup.components.empty() ) {
        r.components.clear();
        r.components.insert( r.components.end(), dup.components.begin(), dup.components.end() );
    }
    if( !dup.tools.empty() ) {
        r.tools.clear();
        r.tools.insert( r.tools.end(), dup.tools.begin(), dup.tools.end() );
    }
    if( !dup.qualities.empty() ) {
        r.qualities.clear();
        r.qualities.insert( r.qualities.end(), dup.qualities.begin(), dup.qualities.end() );
    }

    if( !!extend ) {
        for( unsigned i = 0; i < r.components.size() && i < extend->components.size(); i++ ) {
            r.components[i].insert( r.components[i].end(), extend->components[i].begin(),
                                    extend->components[i].end() );
        }
        for( unsigned i = 0; i < r.tools.size() && i < extend->tools.size(); i++ ) {
            r.tools[i].insert( r.tools[i].end(), extend->tools[i].begin(), extend->tools[i].end() );
        }
        for( unsigned i = 0; i < r.qualities.size() && i < extend->qualities.size(); i++ ) {
            r.qualities[i].insert( r.qualities[i].end(), extend->qualities[i].begin(),
                                   extend->qualities[i].end() );
        }
    }
}

template<typename T>
bool requirement_data::any_marked_available( const std::vector<T> &comps )
{
    for( const auto &comp : comps ) {
        if( comp.available == available_status::a_true ) {
            return true;
        }
    }
    return false;
}

template<typename T>
std::string requirement_data::print_all_objs( const std::string &header,
        const std::vector< std::vector<T> > &objs )
{
    std::string buffer;
    for( const auto &list : objs ) {
        if( !buffer.empty() ) {
            buffer += std::string( "\n" ) + _( "and " );
        }
        std::vector<std::string> alternatives;
        std::transform( list.begin(), list.end(), std::back_inserter( alternatives ),
        []( const T & t ) {
            return t.to_string();
        } );
        std::sort( alternatives.begin(), alternatives.end(), localized_compare );
        buffer += string_join( alternatives, _( " or " ) );
    }
    if( buffer.empty() ) {
        return std::string();
    }
    return header + "\n" + buffer + "\n";
}

std::string requirement_data::list_all() const
{
    std::string buffer;
    buffer += print_all_objs( _( "These tools are required:" ), tools );
    buffer += print_all_objs( _( "These tools are required:" ), qualities );
    buffer += print_all_objs( _( "These components are required:" ), components );
    return buffer;
}

template<typename T>
std::string requirement_data::print_missing_objs( const std::string &header,
        const std::vector< std::vector<T> > &objs )
{
    std::string separator_and = _( "and " );
    std::string separator_or = _( " or " );
    std::string buffer;
    for( const auto &list : objs ) {
        if( any_marked_available( list ) ) {
            continue;
        }
        if( !buffer.empty() ) {
            buffer += std::string( "\n" ) + separator_and;
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            if( it != list.begin() ) {
                buffer += separator_or;
            }
            buffer += it->to_string();
        }
    }
    if( buffer.empty() ) {
        return std::string();
    }
    return header + "\n" + buffer + "\n";
}

std::string requirement_data::list_missing() const
{
    std::string buffer;
    buffer += print_missing_objs( _( "These tools are missing:" ), tools );
    buffer += print_missing_objs( _( "These tools are missing:" ), qualities );
    buffer += print_missing_objs( _( "These components are missing:" ), components );
    return buffer;
}

void quality_requirement::check_consistency( const std::string &display_name ) const
{
    if( !type.is_valid() ) {
        debugmsg( "Unknown quality %s in %s", type.c_str(), display_name );
    }
}

void component::check_consistency( const std::string &display_name ) const
{
    if( !item::type_is_defined( type ) ) {
        debugmsg( "%s in %s is not a valid item template", type.c_str(), display_name );
    }
}

template<typename T>
void requirement_data::check_consistency( const std::vector< std::vector<T> > &vec,
        const std::string &display_name )
{

    for( const auto &list : vec ) {
        bool requirement_was_empty = true;
        for( const auto &comp : list ) {
            requirement_was_empty = false;
            if( comp.requirement ) {
                debugmsg( "Finalization failed to inline %s in %s", comp.type.c_str(), display_name );
            }

            comp.check_consistency( display_name );
        }
        if( requirement_was_empty ) {
            debugmsg( "Requirement %s is empty, recipes using it have no valid results.  Some input must be invalid.",
                      display_name );
        }
    }
}

const std::map<requirement_id, requirement_data> &requirement_data::all()
{
    return requirements_all;
}

void requirement_data::check_consistency()
{
    for( const auto &r : all() ) {
        check_consistency( r.second.tools, r.first.str() );
        check_consistency( r.second.components, r.first.str() );
        check_consistency( r.second.qualities, r.first.str() );
    }
}

template <typename T>
static void inline_requirements( std::vector<std::vector<T>> &list,
                                 const std::function<const std::vector<std::vector<T>> & ( const requirement_data & )> &getter )
{

    const auto add_component = []( const T & comp, std::vector<T> &accum ) {
        auto iter = std::find_if( accum.begin(), accum.end(), [&]( const T & req ) {
            return !req.requirement && req.type == comp.type;
        } );
        if( iter == accum.end() ) {
            accum.push_back( comp );
        } else {


            iter->count = std::min( comp.count, iter->count );
        }
    };



    const auto inline_one = [&]( const T & comp, std::vector<T> &accum ) {
        std::set<requirement_id> already_nested;
        std::list<requirement_id> stack;

        std::function<void( const T &comp )> rec;
        rec = [&]( const T & comp ) {

            if( !comp.requirement ) {
                add_component( comp, accum );
                return;
            }

            const requirement_id r( comp.type.str() );
            if( !r.is_valid() ) {
                debugmsg( "Tried to inline unknown requirement %s", r.c_str() );
                return;
            }

            stack.push_back( r );
            if( already_nested.count( r ) ) {

                debugmsg( "Tried to inline requirement %s which forms a cycle: %s",
                r.c_str(),  debug_menu::iterable_to_string( stack, " -> ", []( const requirement_id & r ) {
                    return r.str();
                } ) );
                stack.pop_back();
                return;
            }
            already_nested.insert( r );

            const requirement_data &req = r.obj();
            const requirement_data multiplied = req * comp.count;

            const std::vector<std::vector<T>> &to_inline = getter( multiplied );
            if( !to_inline.empty() ) {
                const std::vector<T> &nested = to_inline.front();
                for( const T &nested_comp : nested ) {
                    rec( nested_comp );
                }
            }
            already_nested.erase( r );
            stack.pop_back();
        };

        rec( comp );
    };

    for( std::vector<T> &vec : list ) {
        const std::vector<T> vec_copy = vec;
        vec.clear();
        for( const T &comp  : vec_copy ) {
            inline_one( comp, vec );
        }
    }
}

void requirement_data::finalize()
{
    for( auto &r : const_cast<std::map<requirement_id, requirement_data> &>( all() ) ) {
        inline_requirements<tool_comp>( r.second.tools, []( const requirement_data & d ) -> const auto & {
            return d.get_tools();
        } );
        inline_requirements<item_comp>( r.second.components,
        []( const requirement_data & d ) -> const auto & {
            return d.get_components();
        } );
        requirement_data::alter_tool_comp_vector &vec = r.second.tools;
        for( auto &list : vec ) {
            if( list.empty() ) {
                debugmsg( "Requirements data %s tools vector contains impossible requirements.  Recipes using this requirement set will be impossible to make.",
                          r.first.c_str() );
            }
            std::vector<tool_comp> new_list;
            for( tool_comp &comp : list ) {
                const auto replacements = item_controller->subtype_replacement( comp.type );
                for( const auto &replaced_type : replacements ) {
                    new_list.emplace_back( replaced_type, comp.count );
                }
            }

            list = new_list;
        }
    }
}

void requirement_data::reset()
{
    requirements_all.clear();
}

std::vector<std::string> requirement_data::get_folded_components_list( const Character *actor,
        int width, nc_color col,
        const temp_crafting_inventory &crafting_inv, const std::function<bool( const item & )> &filter,
        int batch,
        std::string_view hilite, requirement_display_flags flags ) const
{
    std::vector<std::string> out_buffer;
    if( components.empty() ) {
        return out_buffer;
    }
    out_buffer.push_back( colorize( _( "Components required:" ), col ) );

    std::vector<std::string> folded_buffer =
        get_folded_list( actor, width, crafting_inv, filter, components, batch, hilite, flags );
    out_buffer.insert( out_buffer.end(), folded_buffer.begin(), folded_buffer.end() );

    return out_buffer;
}

template<typename T>
std::vector<std::string> requirement_data::get_folded_list( const Character *actor, int width,
        const temp_crafting_inventory &crafting_inv, const std::function<bool( const item & )> &filter,
        const std::vector< std::vector<T> > &objs, int batch, std::string_view hilite,
        requirement_display_flags flags ) const
{

    can_make_with_inventory( actor, crafting_inv, filter );

    const bool no_unavailable =
        static_cast<bool>( flags & requirement_display_flags::no_unavailable );

    std::vector<std::string> out_buffer;
    for( const auto &comp_list : objs ) {
        const bool has_one = any_marked_available( comp_list );
        std::vector<std::string> list_as_string;
        std::vector<std::string> list_as_string_unavailable;
        std::vector<std::string> buffer_has;
        for( const T &component : comp_list ) {
            nc_color color = component.get_color( actor, has_one, crafting_inv, filter, batch );
            const std::string color_tag = get_tag_from_color( color );
            int qty = 0;
            if( component.get_component_type() == component_type::ITEM ) {
                const itype_id item_id = itype_id( component.type.str() );
                if( item::count_by_charges( item_id ) ) {
                    qty = crafting_inv.charges_of( item_id, INT_MAX, filter );
                } else {
                    qty = crafting_inv.amount_of( item_id, false, INT_MAX, filter );
                }
            }
            const std::string text = component.to_string( batch, qty );

            if( std::find( buffer_has.begin(), buffer_has.end(), text + color_tag ) != buffer_has.end() ) {
                continue;
            }

            if( !hilite.empty() && lcmatch( text, hilite ) ) {
                color = yellow_background( color );
            }

            if( component.has( actor, crafting_inv, filter, batch ) ) {
                list_as_string.push_back( colorize( text, color ) );
            } else if( !no_unavailable ) {
                list_as_string_unavailable.push_back( colorize( text, color ) );
            }
            buffer_has.push_back( text + color_tag );
        }

        std::sort( list_as_string.begin(), list_as_string.end(), localized_compare );
        std::sort( list_as_string_unavailable.begin(), list_as_string_unavailable.end(),
                   localized_compare );
        list_as_string.insert( list_as_string.end(), list_as_string_unavailable.begin(),
                               list_as_string_unavailable.end() );

        const std::string separator = colorize( _( " OR " ), c_white );
        const std::string unfolded = string_join( list_as_string, separator );
        std::vector<std::string> folded = foldstring( unfolded, width - 2 );

        for( size_t i = 0; i < folded.size(); i++ ) {
            if( i == 0 ) {
                out_buffer.push_back( std::string( "> " ).append( folded[i] ) );
            } else {
                out_buffer.push_back( std::string( "  " ).append( folded[i] ) );
            }
        }
    }
    return out_buffer;
}

std::vector<std::string> requirement_data::get_folded_tools_list( const Character *actor, int width,
        nc_color col,
        const temp_crafting_inventory &crafting_inv, int batch ) const
{
    std::vector<std::string> output_buffer;
    output_buffer.reserve( 2 );
    output_buffer.push_back( colorize( _( "Tools required:" ), col ) );
    if( tools.empty() && qualities.empty() ) {
        output_buffer.push_back( colorize( "> ", col ) + colorize( _( "NONE" ), c_green ) );
        return output_buffer;
    }

    std::vector<std::string> folded_qualities = get_folded_list( actor, width, crafting_inv,
            return_true<item>, qualities );
    output_buffer.insert( output_buffer.end(), folded_qualities.begin(), folded_qualities.end() );

    std::vector<std::string> folded_tools = get_folded_list( actor, width, crafting_inv,
                                            return_true<item>, tools, batch );
    output_buffer.insert( output_buffer.end(), folded_tools.begin(), folded_tools.end() );
    return output_buffer;
}

bool requirement_data::can_make_with_inventory( const Character *actor,
        const temp_crafting_inventory &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch, craft_flags flags,
        bool restrict_volume ) const
{

    if( actor != nullptr && actor->has_trait( trait_DEBUG_HS ) ) {
        return true;
    }

    bool retval = true;

    if( !has_comps( actor, crafting_inv, qualities, return_true<item> ) ) {
        retval = false;
    }
    if( !has_comps( actor, crafting_inv, tools, return_true<item>, batch, flags ) ) {
        retval = false;
    }
    if( !has_comps( actor, crafting_inv, components, filter, batch ) ) {
        retval = false;
    }
    if( !check_enough_materials( actor, crafting_inv, filter, batch, restrict_volume ) ) {
        retval = false;
    }
    return retval;
}

template<typename T>
bool requirement_data::has_comps( const Character *actor,
                                  const temp_crafting_inventory &crafting_inv,
                                  const std::vector< std::vector<T> > &vec,
                                  const std::function<bool( const item & )> &filter,
                                  int batch, craft_flags flags )
{
    bool retval = true;
    int total_UPS_charges_used = 0;
    for( const std::vector<T> &set_of_tools : vec ) {
        bool has_tool_in_set = false;
        int UPS_charges_used = std::numeric_limits<int>::max();
        const std::function<void( int )> use_ups = [ &UPS_charges_used ]( int charges ) {
            UPS_charges_used = std::min( UPS_charges_used, charges );
        };

        for( const T &tool : set_of_tools ) {
            if( tool.has( actor, crafting_inv, filter, batch, flags, use_ups ) ) {
                tool.available = available_status::a_true;
            } else {




                tool.available = available_status::a_false;
            }
            has_tool_in_set = has_tool_in_set || tool.available == available_status::a_true;
        }
        if( !has_tool_in_set ) {
            retval = false;
        }
        if( UPS_charges_used != std::numeric_limits<int>::max() ) {
            total_UPS_charges_used += UPS_charges_used;
        }
    }

    if( total_UPS_charges_used > 0 &&
        total_UPS_charges_used > crafting_inv.charges_of( itype_UPS ) ) {
        return false;
    }
    return retval;
}

bool quality_requirement::has(
    const Character *actor,
    const temp_crafting_inventory &crafting_inv, const std::function<bool( const item & )> &, int,
    craft_flags, const std::function<void( int )> & ) const
{
    if( actor != nullptr && actor->has_trait( trait_DEBUG_HS ) ) {
        return true;
    }



    return crafting_inv.has_provider_quality( type, level, count, actor ) ||
           ( actor != nullptr && actor->has_intrinsic_quality( type, level, count ) );
}

nc_color quality_requirement::get_color( const Character *actor, bool has_one,
        const temp_crafting_inventory &,
        const std::function<bool( const item & )> &, int ) const
{
    if( ( actor != nullptr && actor->has_trait( trait_DEBUG_HS ) ) ||
        available == available_status::a_true ) {
        return c_green;
    }
    return has_one ? c_dark_gray : c_red;
}

bool tool_comp::has(
    const Character *actor, const temp_crafting_inventory &crafting_inv,
    const std::function<bool( const item & )> &filter,
    int batch,
    craft_flags flags, const std::function<void( int )> &visitor ) const
{
    if( actor != nullptr && actor->has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    if( !by_charges() ) {
        return crafting_inv.has_tools( type, std::abs( count ), filter );
    } else {
        int charges_required = count * batch * item::find_type( type )->charge_factor();



        if( ( flags & craft_flags::start_only ) != craft_flags::none && type->tool ) {






            charges_required = std::min( charges_required, charges_required / 20 + 19 );
        }

        int charges_found = crafting_inv.charges_of( type, charges_required, filter, visitor );
        return charges_found == charges_required;
    }
}

nc_color tool_comp::get_color( const Character *actor, bool has_one,
                               const temp_crafting_inventory &crafting_inv,
                               const std::function<bool( const item & )> &filter, int batch ) const
{
    if( available == available_status::a_insufficient ) {
        return c_brown;
    } else if( has( actor, crafting_inv, filter, batch ) ) {
        return c_green;
    }
    return has_one ? c_dark_gray : c_red;
}

bool item_comp::has(
    const Character *actor, const temp_crafting_inventory &crafting_inv,
    const std::function<bool( const item & )> &filter,
    int batch,
    craft_flags, const std::function<void( int )> & ) const
{
    if( actor != nullptr && actor->has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    const int cnt = std::abs( count ) * batch;
    if( item::count_by_charges( type ) ) {
        return crafting_inv.has_charges( type, cnt, filter );
    } else {
        return crafting_inv.has_components( type, cnt, filter );
    }
}

nc_color item_comp::get_color( const Character *actor, bool has_one,
                               const temp_crafting_inventory &crafting_inv,
                               const std::function<bool( const item & )> &filter, int batch ) const
{
    if( available == available_status::a_insufficient ) {
        return c_brown;
    } else if( has( actor, crafting_inv, filter, batch ) ) {

        if( std::any_of( type->pockets.begin(), type->pockets.end(), []( const pocket_data & d ) {
        return d.type == pocket_type::CONTAINER && d.watertight;
    } ) &&  crafting_inv.must_use_liq_container( type, count * batch ) ) {
            return c_magenta;
        }
        if( crafting_inv.must_use_hallu_poison( type, count * batch ) ) {
            return c_magenta;
        }

        if( !has( actor, crafting_inv, [&filter]( const item & it ) {
        return filter( it ) && !it.is_favorite;
        }, batch ) ) {
            return c_pink;
        }

        return c_green;
    }
    return has_one ? c_dark_gray  : c_red;
}

template<typename T, typename ID>
const T *requirement_data::find_by_type( const std::vector< std::vector<T> > &vec,
        const ID &type )
{
    for( const auto &list : vec ) {
        for( const auto &comp : list ) {
            if( comp.type == type ) {
                return &comp;
            }
        }
    }
    return nullptr;
}

bool requirement_data::check_enough_materials( const Character *actor,
        const temp_crafting_inventory &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch, bool restrict_volume ) const
{
    bool retval = true;
    units::volume total_component_volume = 0_ml;
    for( const auto &component_choices : components ) {
        bool atleast_one_available = false;
        units::volume max_volume_of_this_comp_choice = 0_ml;
        for( const item_comp &comp : component_choices ) {
            if( check_enough_materials( actor, comp, crafting_inv, filter, batch ) ) {

                const double relative_amount = comp.type->count_by_charges() ?
                                               static_cast<double>( comp.count ) / static_cast<double>( comp.type->stack_size ) : comp.count;
                const units::volume comp_volume = comp.type->volume * relative_amount * batch;

                max_volume_of_this_comp_choice = std::max( max_volume_of_this_comp_choice, comp_volume );
                atleast_one_available = true;
            }
        }
        total_component_volume += max_volume_of_this_comp_choice;
        if( !atleast_one_available ) {
            retval = false;
        }
    }



    if( restrict_volume && total_component_volume > MAX_ITEM_VOLUME ) {
        retval = false;
    }

    return retval;
}

bool requirement_data::check_enough_materials( const Character *actor, const item_comp &comp,
        const temp_crafting_inventory &crafting_inv,
        const std::function<bool( const item & )> &filter, int batch ) const
{
    if( comp.available != available_status::a_true ) {
        return false;
    }
    const int cnt = std::abs( comp.count ) * batch;
    const tool_comp *tq = find_by_type( tools, comp.type );
    if( tq != nullptr && tq->available == available_status::a_true ) {


        const int tc = tq->by_charges() ? 1 : std::abs( tq->count );













        const item_comp i_tmp( comp.type, cnt + tc );
        const tool_comp t_tmp( comp.type, -( cnt + tc ) );

        if( !i_tmp.has( actor, crafting_inv, filter, 1 ) &&
            !t_tmp.has( actor, crafting_inv, filter, 1 ) ) {
            comp.available = available_status::a_insufficient;
        }
    }
    const itype *it = item::find_type( comp.type );
    for( const auto &ql : it->qualities ) {
        const quality_requirement *qr = find_by_type( qualities, ql.first );
        if( qr == nullptr || qr->level > ql.second.level ) {
            continue;
        }







        const quality_count mode = qr->count == 1
                                   ? quality_count::units
                                   : quality_count::providers;
        if( !crafting_inv.has_provider_quality( qr->type, qr->level, qr->count + cnt, actor,
                                                mode ) ) {
            comp.available = available_status::a_insufficient;
        }
    }
    return comp.available == available_status::a_true;
}

template <typename T>
static bool apply_blacklist( std::vector<std::vector<T>> &vec, const itype_id &id )
{

    for( auto &opts : vec ) {
        opts.erase( std::remove_if( opts.begin(), opts.end(), [&id]( const T & e ) {
            return e.type == id;
        } ), opts.end() );
    }


    const bool blacklisted = std::any_of( vec.begin(), vec.end(), []( const std::vector<T> &e ) {
        return e.empty();
    } );


    vec.erase( std::remove_if( vec.begin(), vec.end(), []( const std::vector<T> &e ) {
        return e.empty();
    } ), vec.end() );

    return blacklisted;
}

void requirement_data::blacklist_item( const itype_id &id )
{
    blacklisted |= apply_blacklist( tools, id );
    blacklisted |= apply_blacklist( components, id );
}

template <typename T>
static void apply_replacements( std::vector<std::vector<T>> &vec,
                                const std::unordered_map<itype_id, itype_id> &replacements )
{
    for( std::vector<T> &opts : vec ) {
        for( typename std::vector<T>::iterator iiter = opts.begin(); iiter != opts.end(); ) {
            auto riter = replacements.find( iiter->type );
            if( riter != replacements.end() ) {
                itype_id to_id = riter->second;


                bool has_duplicates = std::count_if( opts.cbegin(), opts.cend(), [&to_id]( const T & item ) {
                    return item.type == to_id;
                } ) != 0;
                if( has_duplicates ) {
                    iiter = opts.erase( iiter );
                    continue;
                } else {
                    iiter->type = to_id;
                }
            }
            ++iiter;
        }
    }
}

void requirement_data::replace_items( const std::unordered_map<itype_id, itype_id> &replacements )
{
    apply_replacements( tools, replacements );
    apply_replacements( components, replacements );
}

const std::string &requirement_data::display_name() const
{
    static const std::string empty;
    return name_.empty() ? empty : name_.translated();
}

bool requirement_data::has_display_name() const
{
    return !name_.empty();
}

const requirement_data::alter_tool_comp_vector &requirement_data::get_tools() const
{
    return tools;
}

const requirement_data::alter_quali_req_vector &requirement_data::get_qualities() const
{
    return qualities;
}

const requirement_data::alter_item_comp_vector &requirement_data::get_components() const
{
    return components;
}

requirement_data::alter_item_comp_vector &requirement_data::get_components()
{
    return components;
}

requirement_data requirement_data::disassembly_requirements() const
{





    requirement_data ret = *this;
    auto new_qualities = std::vector<quality_requirement>();
    bool remove_fire = false;
    for( auto &it : ret.tools ) {
        bool replaced = false;
        for( const tool_comp &tool : it ) {
            const itype_id &type = tool.type;


            if( type == itype_welder || type == itype_welder_crude || type == itype_oxy_torch ||
                type == itype_forge || type == itype_char_forge ) {
                new_qualities.emplace_back( qual_SAW_M_FINE, 1, 1 );
                replaced = true;
                break;
            }
            if( type == itype_crucible ) {
                replaced = true;
                break;
            }

            if( type == itype_press ) {
                replaced = true;
                remove_fire = true;
                new_qualities.emplace_back( qual_PULL, 1, 1 );
                break;
            }
            if( type == itype_fire && remove_fire ) {
                replaced = true;
                break;
            }
        }

        if( replaced ) {


            it.clear();
        }
    }




    if( ret.qualities.empty() ) {
        ret.qualities.resize( 1 );
    } else {


        for( auto &it : ret.qualities ) {
            bool replaced = false;
            for( const quality_requirement &quality : it ) {
                if( quality.type == qual_SEW ) {
                    replaced = true;
                    new_qualities.emplace_back( qual_CUT, 1, quality.level );
                    break;
                }
                if( quality.type == qual_GLARE ) {
                    replaced = true;



                    break;
                }
                if( quality.type == qual_KNIT ) {
                    replaced = true;

                    break;
                }
            }
            if( replaced ) {
                it.clear();
            }
        }
    }

    auto &qualities = ret.qualities[0];
    qualities.insert( qualities.end(), new_qualities.begin(), new_qualities.end() );

    {
        const auto itr = std::unique( qualities.begin(), qualities.end(),
        []( const quality_requirement & a, const quality_requirement & b ) {
            return a.type == b.type;
        } );
        qualities.resize( std::distance( qualities.begin(), itr ) );
    }


    ret.tools.erase( std::remove_if( ret.tools.begin(), ret.tools.end(),
    []( const std::vector<tool_comp> &tcv ) {
        return tcv.empty();
    } ), ret.tools.end() );

    ret.components.erase( std::remove_if( ret.components.begin(), ret.components.end(),
    []( std::vector<item_comp> &cov ) {
        cov.erase( std::remove_if( cov.begin(), cov.end(),
        []( const item_comp & comp ) {
            return !comp.recoverable || item( comp.type ).has_flag( json_flag_UNRECOVERABLE );
        } ), cov.end() );
        return cov.empty();
    } ), ret.components.end() );

    return ret;
}

requirement_data requirement_data::continue_requirements( const std::vector<item_comp>
        &required_comps, const item_components &remaining_comps )
{

    requirement_data ret;


    for( const item_comp &it : required_comps ) {
        ret.components.emplace_back( std::vector<item_comp>( {it} ) );
    }

    temp_crafting_inventory craft_components;
    for( const item_components::type_vector_pair &tvp : remaining_comps ) {
        for( const item &inner : tvp.second ) {
            craft_components.add_item_copy( inner );
        }
    }





    ret.components.erase( std::remove_if( ret.components.begin(), ret.components.end(),
    [&craft_components]( std::vector<item_comp> &comps ) {
        item_comp &comp = comps.front();
        if( item::count_by_charges( comp.type ) && comp.count > 0 ) {
            int qty = craft_components.charges_of( comp.type, comp.count );
            comp.count -= qty;

            std::vector<item_location> del;
            craft_components.visit_items( [&comp, &qty, &del]( item_location e ) {
                std::list<item> used;
                if( e->use_charges( e, comp.type, qty, used, tripoint_bub_ms::zero ) ) {
                    del.push_back( e );
                }
                return qty > 0 ? VisitResponse::SKIP : VisitResponse::ABORT;
            } );
            for( item_location &loc : del ) {
                loc.remove_item();
            }
        } else {
            int amount = craft_components.amount_of( comp.type, comp.count );
            comp.count -= amount;
            craft_components.remove_items_with(
            [&comp]( const item & it ) {
                return it.typeId() == comp.type;
            }
            , amount
            );
        }
        return comp.count <= 0;
    } ), ret.components.end() );

    return ret;
}

template<typename T, typename Accum>
static std::vector<std::vector<T>> consolidate( std::vector<std::vector<T>> old_vec,
                                const Accum &accum )
{
    const auto type_lt = []( const T & lhs, const T & rhs ) -> bool {

        return std::forward_as_tuple( lhs.type.str(), lhs.requirement )
        < std::forward_as_tuple( rhs.type.str(), rhs.requirement );
    };





    for( std::vector<T> &old_inner : old_vec ) {
        std::sort( old_inner.begin(), old_inner.end(), type_lt );
    }
    std::sort( old_vec.begin(), old_vec.end(),
    [&type_lt]( const std::vector<T> &lhs, const std::vector<T> &rhs ) -> bool {
        return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                             type_lt );
    } );

    std::vector<std::vector<T>> new_vec;
    for( std::vector<T> &old_inner : old_vec ) {
        bool match = false;
        for( std::vector<T> &new_inner : new_vec ) {














            if( std::includes( new_inner.begin(), new_inner.end(),
                               old_inner.begin(), old_inner.end(),
                               type_lt ) ) {

                match = true;
                std::swap( old_inner, new_inner );
            } else if( std::includes( old_inner.begin(), old_inner.end(),
                                      new_inner.begin(), new_inner.end(),
                                      type_lt ) ) {

                match = true;
            }
            if( match ) {
                for( auto it1 = new_inner.begin(), it2 = old_inner.begin();
                     it1 < new_inner.end(); ++it2 ) {
                    if( !type_lt( *it2, *it1 ) ) {


                        *it1 = accum( *it1, *it2 );
                        ++it1;
                    }
                }
                break;
            }
        }
        if( !match ) {
            new_vec.emplace_back( old_inner );
        }
    }
    return new_vec;
}

void requirement_data::consolidate()
{
    qualities = ::consolidate( qualities,
    []( const quality_requirement & lhs, const quality_requirement & rhs ) {
        quality_requirement ret = lhs;
        ret.count = std::max( ret.count, rhs.count );
        ret.level = std::max( ret.level, rhs.level );
        return ret;
    } );

    tools = ::consolidate( tools,
    []( const tool_comp & lhs, const tool_comp & rhs ) {
        tool_comp ret = lhs;
        if( ret.count < 0 && rhs.count < 0 ) {
            ret.count = std::min( ret.count, rhs.count );
        } else if( ret.count > 0 && rhs.count > 0 ) {
            ret.count += rhs.count;
        } else {
            debugmsg( "required counts of the same tool have different signs" );
        }
        return ret;
    } );

    components = ::consolidate( components,
    []( const item_comp & lhs, const item_comp & rhs ) {
        item_comp ret = lhs;
        ret.count += rhs.count;
        return ret;
    } );
}

template<typename T>
static bool sorted_equal( std::vector<std::vector<T>> lhs, std::vector<std::vector<T>> rhs )
{
    if( lhs.size() != rhs.size() ) {
        return false;
    }
    for( auto &inner : lhs ) {
        std::sort( inner.begin(), inner.end() );
    }
    for( auto &inner : rhs ) {
        std::sort( inner.begin(), inner.end() );
    }
    std::sort( lhs.begin(), lhs.end() );
    std::sort( rhs.begin(), rhs.end() );
    return lhs == rhs;
}

bool requirement_data::has_same_requirements_as( const requirement_data &that ) const
{
    return sorted_equal( tools, that.tools ) && sorted_equal( qualities, that.qualities )
           && sorted_equal( components, that.components );
}

template<typename T>
static void dump_req_vec( const std::vector<std::vector<T>> &vec, JsonOut &jsout )
{
    jsout.start_array( !vec.empty() );
    for( const auto &inner : vec ) {
        jsout.start_array();
        for( const auto &val : inner ) {
            val.dump( jsout );
        }
        jsout.end_array();
    }
    jsout.end_array();
}

void requirement_data::dump( JsonOut &jsout ) const
{
    jsout.start_object( true );

    jsout.member( "tools" );
    dump_req_vec( tools, jsout );

    jsout.member( "qualities" );
    dump_req_vec( qualities, jsout );

    jsout.member( "components" );
    dump_req_vec( components, jsout );

    jsout.end_object();
}

uint64_t requirement_data::make_hash() const
{
    std::ostringstream stream;
    JsonOut json( stream );
    dump( json );
    std::hash<std::string> hasher;
    return hasher( stream.str() );
}





























static void expand_item_in_reqs(
    const item_comp &leftover, requirement_data::alter_item_comp_vector req_prefix,
    const requirement_data::alter_item_comp_vector &to_expand, size_t orig_index, size_t index,
    std::vector<requirement_data::alter_item_comp_vector> &result )
{
    cata_assert( req_prefix.size() >= orig_index );
    cata_assert( orig_index < index );

    if( index == to_expand.size() ) {


        req_prefix.insert( req_prefix.begin() + orig_index, { leftover } );
        result.push_back( req_prefix );
        return;
    }

    std::vector<item_comp> this_requirement = to_expand[index];
    auto duplicate = std::find_if( this_requirement.begin(), this_requirement.end(),
    [&]( const item_comp & c ) {
        return c.type == leftover.type;
    } );
    if( duplicate == this_requirement.end() ) {

        req_prefix.push_back( this_requirement );
        expand_item_in_reqs( leftover, req_prefix, to_expand, orig_index, index + 1, result );
        return;
    }


    requirement_data::alter_item_comp_vector req = req_prefix;
    req.push_back( { item_comp( leftover.type, leftover.count + duplicate->count ) } );
    req.insert( req.end(), to_expand.begin() + index + 1, to_expand.end() );
    result.push_back( req );




    this_requirement.erase( duplicate );
    if( !this_requirement.empty() ) {
        req_prefix.push_back( this_requirement );
        expand_item_in_reqs( leftover, req_prefix, to_expand, orig_index, index + 1, result );
    }
}

deduped_requirement_data::deduped_requirement_data( const requirement_data &in,
        const recipe_id &context )
{







    struct to_check {
        alter_item_comp_vector components;
        size_t index;
    };
    std::stack<to_check, std::vector<to_check>> pending;
    pending.push( { in.get_components(), 0 } );


    requirement_data::alter_tool_comp_vector tools_unique;
    for( auto tools : in.get_tools() ) {
        std::sort( tools.begin(), tools.end() );
        tools.erase( std::unique( tools.begin(), tools.end() ), tools.end() );
        tools_unique.push_back( tools );
    }

    while( !pending.empty() ) {
        to_check next = pending.top();
        pending.pop();

        if( next.index == next.components.size() ) {
            alternatives_.emplace_back( tools_unique, in.get_qualities(), next.components );
            continue;
        }



        std::unordered_set<itype_id> later_itypes;
        for( size_t i = next.index + 1; i != next.components.size(); ++i ) {
            std::transform( next.components[i].begin(), next.components[i].end(),
                            std::inserter( later_itypes, later_itypes.end() ),
            []( const item_comp & c ) {
                return c.type;
            } );
        }

        std::vector<item_comp> this_requirement = next.components[next.index];

        auto first_duplicated = std::stable_partition(
                                    this_requirement.begin(), this_requirement.end(),
        [&]( const item_comp & c ) {
            return !later_itypes.count( c.type );
        }
                                );

        for( auto comp_it = first_duplicated; comp_it != this_requirement.end(); ++comp_it ) {


            alter_item_comp_vector req_prefix( next.components.begin(),
                                               next.components.begin() + next.index );
            std::vector<alter_item_comp_vector> result;
            expand_item_in_reqs( *comp_it, req_prefix, next.components, next.index, next.index + 1,
                                 result );
            for( const alter_item_comp_vector &v : result ) {


                size_t index_inc = v.size() == next.components.size() ? 1 : 0;
                pending.push( { v, next.index + index_inc } );
            }
        }


        this_requirement.erase( first_duplicated, this_requirement.end() );
        if( !this_requirement.empty() ) {
            alter_item_comp_vector without_dupes = next.components;
            without_dupes[next.index] = this_requirement;
            pending.push( { without_dupes, next.index + 1 } );
        }

        if( alternatives_.empty() && pending.empty() ) {
            debugmsg( "Recipe definition %s somehow has no valid recipes!", context.str() );
        }





        static constexpr size_t max_alternatives = 115;
        if( alternatives_.size() + pending.size() > max_alternatives ) {
            debugmsg( "Construction of deduped_requirement_data generated too many alternatives.  "
                      "The recipe %1s should be simplified.  See the Recipe section in "
                      "doc/JSON/JSON_INFO.md for more details.  It has %2s alternatives.", context.str(),
                      alternatives_.size() + pending.size() );
            is_too_complex_ = true;
            alternatives_ = { in };
            return;
        }
    }
}

bool deduped_requirement_data::can_make_with_inventory(
    const Character *actor,
    const temp_crafting_inventory &crafting_inv, const std::function<bool( const item & )> &filter,
    int batch, craft_flags flags ) const
{
    return std::any_of( alternatives().begin(), alternatives().end(),
    [&]( const requirement_data & alt ) {
        return alt.can_make_with_inventory( actor, crafting_inv, filter, batch, flags );
    } );
}

std::vector<const requirement_data *> deduped_requirement_data::feasible_alternatives(
    const Character *actor, const temp_crafting_inventory &crafting_inv,
    const std::function<bool( const item & )> &filter,
    int batch, craft_flags flags ) const
{
    std::vector<const requirement_data *> result;
    for( const requirement_data &req : alternatives() ) {
        if( req.can_make_with_inventory( actor, crafting_inv, filter, batch, flags ) ) {
            result.push_back( &req );
        }
    }
    return result;
}

const requirement_data *deduped_requirement_data::select_alternative(
    Character &crafter, const std::function<bool( const item & )> &filter, int batch,
    craft_flags flags ) const
{
    return select_alternative( crafter, crafter.crafting_inventory(), filter, batch, flags );
}

const requirement_data *deduped_requirement_data::select_alternative(
    Character &crafter, const temp_crafting_inventory &inv,
    const std::function<bool( const item & )> &filter,
    int batch, craft_flags flags ) const
{
    const std::vector<const requirement_data *> all_reqs =
        feasible_alternatives( &crafter, inv, filter, batch, flags );
    return crafter.select_requirements( all_reqs, 1, inv, filter );
}
