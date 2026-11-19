#pragma once
#ifndef CATA_SRC_GENERIC_FACTORY_H
#define CATA_SRC_GENERIC_FACTORY_H

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "debug.h"
#include "demangle.h"
#include "enum_conversions.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "init.h"
#include "int_id.h"
#include "json.h"
#include "mod_tracker.h"
#include "string_formatter.h"
#include "string_id.h"
#include "units.h"

class quantity;

template <typename E> class enum_bitset;
template<typename T> struct weighted_int_list;
template <typename T, typename W> struct weighted_list;





























































































template<typename T>
class generic_factory
{
    public:
        virtual ~generic_factory() = default;

    private:
        DynamicDataLoader::deferred_json deferred;




        int64_t  version = 0;

        void inc_version() {
            do {
                version++;
            } while( version == INVALID_VERSION );
        }

    protected:
        std::vector<T> list;
        std::unordered_map<string_id<T>, int_id<T>> map;
        std::unordered_map<std::string, T> abstracts;

        std::string type_name;
        std::string id_member_name;

        bool find_id( const string_id<T> &id, int_id<T> &result ) const {
            if( id._version == version ) {
                result = int_id<T>( id._cid );
                return is_valid( result );
            }
            const auto iter = map.find( id );


            if( iter == map.end() ) {
                id.set_cid_version( INVALID_CID, version );
                return false;
            }
            result = iter->second;
            id.set_cid_version( result.to_i(), version );
            return true;
        }

        const T dummy_obj;

    public:
        const bool initialized;






        explicit generic_factory( const std::string &type_name, const std::string &id_member_name = "id" )
            : type_name( type_name ),
              id_member_name( id_member_name ),
              dummy_obj(),
              initialized( true ) {
        }






        template<typename U>
        using T_has_handle_inheritance_t = decltype(
            std::declval<U>().handle_inheritance(
                std::declval<const T &>(),
                std::declval<const std::unordered_map<std::string, T>&>() ) );

        template<typename U, typename=void>
        struct T_has_handle_inheritance : std::false_type {};
        template<typename U>
        struct T_has_handle_inheritance<U, std::void_t<T_has_handle_inheritance_t<U>>> : std::true_type {};
        template<typename U=T, std::enable_if_t<T_has_handle_inheritance<U>::value>* = nullptr>
        void handle_inheritance_on_T( T &def, const T &copy_from ) {
            def.handle_inheritance( copy_from, abstracts );
        }
        template<typename U=T, std::enable_if_t<!T_has_handle_inheritance<U>::value>* = nullptr>
        void handle_inheritance_on_T( T &def, const T &copy_from ) {
            def = copy_from;
        }



        template<typename U, typename = void>
        struct T_has_finalize : std::false_type {};


        template<typename U>
    struct T_has_finalize<U, std::void_t<decltype( std::declval<U &>().finalize() )>> :
        std::true_type {};











        bool handle_inheritance( T &def, const JsonObject &jo, const std::string &src ) {
            static const std::string copy_from_member_name( "copy-from" );
            static const std::string abstract_member_name( "abstract" );
            if( jo.has_string( copy_from_member_name ) ) {
                const std::string source = jo.get_string( copy_from_member_name );
                auto base = map.find( string_id<T>( source ) );

                if( base != map.end() ) {
                    const T &base_obj = obj( base->second );
                    handle_inheritance_on_T( def, base_obj );
                } else {
                    auto ab = abstracts.find( source );

                    if( ab != abstracts.end() ) {
                        const T &base_obj = ab->second;
                        handle_inheritance_on_T( def, base_obj );
                    } else {
                        def.was_loaded = false;
                        deferred.emplace_back( jo, src );
                        jo.allow_omitted_members();
                        return false;
                    }
                }
                def.was_loaded = true;
            }

            if( jo.has_string( abstract_member_name ) ) {
                if( jo.has_string( id_member_name ) ) {
                    jo.throw_error( string_format( "cannot specify both '%s' and '%s'",
                                                   abstract_member_name, id_member_name ) );
                }
                restore_on_out_of_scope restore_check_plural( check_plural );
                check_plural = check_plural_t::none;
                const std::string abstract_id =  jo.get_string( abstract_member_name );
                def.id = string_id<T>( abstract_id );

                // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
                def.load( jo, src );
                abstracts[abstract_id] = def;
            }
            return true;
        }

        template<typename LT, typename = std::void_t<>>
        struct load_is_bool : std::false_type { };

        template<typename LT>

    struct load_is_bool<LT, std::void_t<std::is_same<decltype( std::declval<LT &>().load() ), bool>>> :
        std:: true_type {};















        template<typename U = T, std::enable_if_t<load_is_bool<U>::value>* = nullptr>
        void load( const JsonObject &jo, const std::string &src ) {
            static const std::string abstract_member_name( "abstract" );

            T def;

            if( !handle_inheritance( def, jo, src ) ) {
                return;
            }
            if( jo.has_string( id_member_name ) ) {
                def.id = string_id<T>( jo.get_string( id_member_name ) );
                mod_tracker::assign_src( def, src );
                if( def.load( jo, src ) ) {
                    insert( def );
                } else {
                    def.was_loaded = false;
                    deferred.emplace_back( jo, src );
                    jo.allow_omitted_members();
                }

            } else if( jo.has_array( id_member_name ) ) {
                for( JsonValue e : jo.get_array( id_member_name ) ) {
                    T def;
                    if( !handle_inheritance( def, jo, src ) ) {
                        break;
                    }
                    def.id = string_id<T>( e );
                    mod_tracker::assign_src( def, src );
                    if( def.load( jo, src ) ) {
                        insert( def );
                    } else {
                        def.was_loaded = false;
                        deferred.emplace_back( jo, src );
                        jo.allow_omitted_members();
                    }
                }

            } else if( !jo.has_string( abstract_member_name ) ) {
                jo.throw_error( string_format( "must specify either '%s' or '%s'",
                                               abstract_member_name, id_member_name ) );
            }
        }

        template < typename U = T, std::enable_if_t < !load_is_bool<T>::value > * = nullptr >
        void load( const JsonObject &jo, const std::string &src ) {
            static const std::string abstract_member_name( "abstract" );

            T def;

            if( !handle_inheritance( def, jo, src ) ) {
                return;
            }
            if( jo.has_string( id_member_name ) ) {
                def.id = string_id<T>( jo.get_string( id_member_name ) );
                mod_tracker::assign_src( def, src );
                def.load( jo, src );
                insert( def );

            } else if( jo.has_array( id_member_name ) ) {
                for( JsonValue e : jo.get_array( id_member_name ) ) {
                    T def;
                    if( !handle_inheritance( def, jo, src ) ) {
                        break;
                    }
                    def.id = string_id<T>( e );
                    mod_tracker::assign_src( def, src );

                    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
                    def.load( jo, src );
                    insert( def );
                }

            } else if( !jo.has_string( abstract_member_name ) ) {
                jo.throw_error( string_format( "must specify either '%s' or '%s'",
                                               abstract_member_name, id_member_name ) );
            }
        }





        T &insert( const T &obj ) {





            inc_version();
            const auto iter = map.find( obj.id );
            if( iter != map.end() ) {
                mod_tracker::check_duplicate_entries( obj, list[iter->second.to_i()] );
                T &result = list[iter->second.to_i()];
                result = obj;
                result.id.set_cid_version( iter->second.to_i(), version );
                return result;
            }

            const int_id<T> cid( list.size() );
            list.push_back( obj );

            T &result = list.back();
            result.id.set_cid_version( cid.to_i(), version );
            map[result.id] = cid;
            return result;
        }


        virtual void finalize() {
            DynamicDataLoader::get_instance().load_deferred( deferred );
            abstracts.clear();

            inc_version();
            for( size_t i = 0; i < list.size(); i++ ) {
                list[i].id.set_cid_version( static_cast<int>( i ), version );
                if constexpr( T_has_finalize<T>::value ) {
                    list[i].finalize();
                }
            }
        }




        void check() const {
            for( const T &obj : list ) {
                obj.check();
            }
        }



        size_t size() const {
            return list.size();
        }



        bool empty() const {
            return list.empty();
        }




        void reset() {

            for( std::pair<JsonObject, std::string> &deferred_json : deferred ) {
                deferred_json.first.allow_omitted_members();
            }
            deferred.clear();
            list.clear();
            map.clear();
            inc_version();
        }



        const std::vector<T> &get_all() const {
            return list;
        }




        std::vector<T> &get_all_mod() {
            return list;
        }















        const T &obj( const int_id<T> &id ) const {
            if( !is_valid( id ) ) {
                debugmsg( "invalid %s id \"%d\"", type_name, id.to_i() );
                return dummy_obj;
            }
            return list[id.to_i()];
        }







        const T &obj( const string_id<T> &id ) const {
            int_id<T> i_id;
            if( !find_id( id, i_id ) ) {
                debugmsg( "invalid %s id \"%s\"", type_name, id.c_str() );
                return dummy_obj;
            }
            return list[i_id.to_i()];
        }




        bool is_valid( const int_id<T> &id ) const {
            return static_cast<size_t>( id.to_i() ) < list.size();
        }




        bool is_valid( const string_id<T> &id ) const {
            int_id<T> dummy;
            return find_id( id, dummy );
        }




        int_id<T> convert( const string_id<T> &id, const int_id<T> &null_id,
                           const bool warn = true ) const {
            int_id<T> result;
            if( find_id( id, result ) ) {
                return result;
            }
            if( warn ) {
                debugmsg( "invalid %s id \"%s\"", type_name, id.c_str() );
            }
            return null_id;
        }



        const string_id<T> &convert( const int_id<T> &id ) const {
            return obj( id ).id;
        }







        class Version
        {
                friend generic_factory<T>;
            public:
                Version() = default;
            private:
                explicit Version( int64_t version ) : version( version ) {}
                int64_t  version = -1;
            public:
                bool operator==( const Version &rhs ) const {
                    return version == rhs.version;
                }
                bool operator!=( const Version &rhs ) const {
                    return !( rhs == *this );
                }
        };


        Version get_version() {
            return Version( version );
        }


        bool is_valid( const Version &v ) {
            return v.version == version;
        }
};










































































void warn_disabled_feature( const JsonObject &jo, std::string_view feature,
                            std::string_view member, std::string_view reason );



template<typename MemberType>
inline void mandatory( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                       MemberType &member )
{
    warn_disabled_feature( jo, "extend", name, "disabled for mandatory" );
    warn_disabled_feature( jo, "delete", name, "disabled for mandatory" );
    warn_disabled_feature( jo, "relative", name, "disabled for mandatory" );
    warn_disabled_feature( jo, "proportional", name, "disabled for mandatory" );
    if( !jo.read( name, member ) ) {
        if( !was_loaded ) {
            if( jo.has_member( name ) ) {
                jo.throw_error( str_cat( "object beginning at next line failed to read mandatory member \"", name,
                                         "\"" ) );
            } else {
                jo.throw_error( str_cat( "object beginning at next line missing mandatory member \"", name,
                                         "\"" ) );
            }
        }
    }
}
template<typename MemberType, typename ReaderType>
inline void mandatory( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                       MemberType &member, const ReaderType &reader )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            if( jo.has_member( name ) ) {
                jo.throw_error( str_cat( "object beginning at next line failed to read mandatory member \"", name,
                                         "\"" ) );
            } else {
                jo.throw_error( str_cat( "object beginning at next line missing mandatory member \"", name,
                                         "\"" ) );
            }
        }
    }
}












template<typename T, typename = std::void_t<>>
struct supports_proportional : std::false_type { };

template<typename T>
struct supports_proportional<T, std::void_t<decltype( std::declval<T &>() *= std::declval<float>() )
>> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_proportional_handler : std::false_type {};

template<typename T>
struct supports_proportional_handler<T, std::void_t<decltype( &T::handle_proportional )
>> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_relative : std::false_type { };

template<typename T>
struct supports_relative < T, std::void_t < decltype( std::declval<T &>() += std::declval<T &>() )
>> : std::true_type {};

template<typename T, typename = void>
struct supports_extend_handler : std::false_type {};

template<typename T>
struct supports_extend_handler<T, std::void_t<decltype( &T::handle_extend )>> : std::true_type {};

template<typename T, typename = void>
struct supports_delete_handler : std::false_type {};

template<typename T>
struct supports_delete_handler<T, std::void_t<decltype( &T::handle_delete )>> : std::true_type {};



template<>
struct supports_proportional<bool> : std::false_type {};

template<>
struct supports_relative<bool> : std::false_type {};

template<>
struct supports_relative<std::string> : std::false_type {};


static_assert( supports_relative<units::energy>::value, "units should support relative" );
static_assert( supports_proportional<units::energy>::value, "units should support proportional" );

static_assert( supports_relative<int>::value, "ints should support relative" );
static_assert( supports_proportional<int>::value, "ints should support proportional" );

static_assert( !supports_relative<bool>::value, "bools should not support relative" );
static_assert( !supports_proportional<bool>::value, "bools should not support proportional" );



static_assert( !supports_relative<string_id<int>>::value,
               "string ids should not support relative" );
static_assert( !supports_proportional<string_id<int>>::value,
               "string ids should not support proportional" );



static_assert( !supports_relative<int_id<int>>::value,
               "int ids should not support relative" );
static_assert( !supports_proportional<int_id<int>>::value,
               "int ids should not support proportional" );

static_assert( !supports_relative<std::string>::value, "strings should not support relative" );
static_assert( !supports_proportional<std::string>::value,
               "strings should not support proportional" );


static_assert( !supports_relative<DebugOutput>::value, "enum classes should not support relative" );
static_assert( !supports_proportional<DebugOutput>::value,
               "enum classes should not support proportional" );


static_assert( !supports_relative<DebugLevel>::value, "enums should not support relative" );
static_assert( !supports_proportional<DebugLevel>::value, "enums should not support relative" );




template < typename MemberType, std::enable_if_t < !supports_proportional<MemberType>::value &&
           !supports_proportional_handler<MemberType>::value > * = nullptr >
inline bool handle_proportional( const JsonObject &jo, const std::string_view name, MemberType & )
{
    if( jo.has_object( "proportional" ) ) {
        JsonObject proportional = jo.get_object( "proportional" );
        proportional.allow_omitted_members();
        if( proportional.has_member( name ) ) {
            debugmsg( "Member %s of type %s does not support proportional", name,
                      demangle( typeid( MemberType ).name() ) );
        }
    }
    return false;
}






template<typename MemberType, std::enable_if_t<supports_proportional<MemberType>::value>* = nullptr>
inline bool handle_proportional( const JsonObject &jo, const std::string_view name,
                                 MemberType &member )
{
    if( jo.has_object( "proportional" ) ) {
        JsonObject proportional = jo.get_object( "proportional" );
        proportional.allow_omitted_members();

        if( !proportional.has_member( name ) ) {
            return false;
        }
        if( proportional.has_float( name ) ) {
            double scalar = proportional.get_float( name );
            if( scalar <= 0 || scalar == 1 ) {
                debugmsg( "Invalid scalar %g for %s", scalar, name );
                return false;
            }
            member *= scalar;
            return true;
        } else {
            jo.throw_error_at( name, str_cat( "Invalid scalar for ", name ) );
        }
    }
    return false;
}


template<typename MemberType, std::enable_if_t<supports_proportional_handler<MemberType>::value>* = nullptr>
inline bool handle_proportional( const JsonObject &jo, const std::string_view name,
                                 MemberType &member )
{
    if( jo.has_object( "proportional" ) ) {
        JsonObject proportional = jo.get_object( "proportional" );
        proportional.allow_omitted_members();

        if( !proportional.has_member( name ) ) {
            return false;
        }
        bool handled = member.handle_proportional( proportional.get_member( name ) );
        if( !handled ) {
            jo.throw_error_at( name, str_cat( "Invalid scalar for ", name ) );
        }
        return handled;
    }
    return false;
}




template < typename MemberType,
           std::enable_if_t < !supports_relative<MemberType>::value > * = nullptr
           >
inline bool handle_relative( const JsonObject &jo, const std::string_view name, MemberType & )
{
    if( jo.has_object( "relative" ) ) {
        JsonObject relative = jo.get_object( "relative" );
        relative.allow_omitted_members();
        if( !relative.has_member( name ) ) {
            return false;
        }
        debugmsg( "Member %s of type %s does not support relative", name,
                  demangle( typeid( MemberType ).name() ) );
    }
    return false;
}





template<typename MemberType, std::enable_if_t<supports_relative<MemberType>::value>* = nullptr>
inline bool handle_relative( const JsonObject &jo, const std::string_view name, MemberType &member )
{
    if( jo.has_object( "relative" ) ) {
        JsonObject relative = jo.get_object( "relative" );
        relative.allow_omitted_members();

        if( !relative.has_member( name ) ) {
            return false;
        }
        MemberType adder;
        if( relative.read( name, adder ) ) {
            member += adder;
            return true;
        } else {
            jo.throw_error_at( name, str_cat( "Invalid adder for ", name ) );
        }
    }
    return false;
}

template<typename MemberType>
void handle_extend( const JsonObject &jo, const std::string_view name, MemberType &member )
{
    if constexpr( supports_extend_handler<MemberType>::value ) {
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            if( tmp.has_member( name ) ) {
                member.handle_extend( tmp.get_member( name ) );
            }
        }
    } else {



        warn_disabled_feature( jo, "extend", name, "does not use reader" );
    }
}

template<typename MemberType>
void handle_delete( const JsonObject &jo, const std::string_view name, MemberType &member )
{
    if constexpr( supports_delete_handler<MemberType>::value ) {
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            if( tmp.has_member( name ) ) {
                member.handle_delete( tmp.get_member( name ) );
            }
        }
    } else {



        warn_disabled_feature( jo, "delete", name, "does not use reader" );
    }
}


template<typename MemberType>
inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                      MemberType &member )
{
    if( !was_loaded ) {
        warn_disabled_feature( jo, "relative", name, "no copy-from" );
        warn_disabled_feature( jo, "proportional", name, "no copy-from" );
        warn_disabled_feature( jo, "extend", name, "no copy-from" );
        warn_disabled_feature( jo, "delete", name, "no copy-from" );
    }
    if( !jo.read( name, member ) && !handle_proportional( jo, name, member ) &&
        !handle_relative( jo, name, member ) ) {
        if( !was_loaded ) {
            member = MemberType();
        }
    }
    handle_extend( jo, name, member );
    handle_delete( jo, name, member );
}










template<typename MemberType, typename DefaultType = MemberType,
         typename = std::enable_if_t<std::is_constructible_v<MemberType, const DefaultType &>>>
                                     inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                                             MemberType &member, const DefaultType &default_value )
{
    if( !was_loaded ) {
        warn_disabled_feature( jo, "relative", name, "no copy-from" );
        warn_disabled_feature( jo, "proportional", name, "no copy-from" );
        warn_disabled_feature( jo, "extend", name, "no copy-from" );
        warn_disabled_feature( jo, "delete", name, "no copy-from" );
    }
    if( !jo.read( name, member ) && !handle_proportional( jo, name, member ) &&
        !handle_relative( jo, name, member ) ) {
        if( !was_loaded ) {
            member = default_value;
        }
    }
    handle_extend( jo, name, member );
    handle_delete( jo, name, member );
}
template < typename MemberType, typename ReaderType, typename DefaultType = MemberType,
           typename = std::enable_if_t <
               !std::is_constructible_v<MemberType, const ReaderType &> > >
inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                      MemberType &member, const ReaderType &reader )
{

    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            member = MemberType();
        }
    }
}
template<typename MemberType, typename ReaderType, typename DefaultType = MemberType>
inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                      MemberType &member, const ReaderType &reader, const DefaultType &default_value )
{

    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            member = default_value;
        }
    }
}






bool one_char_symbol_reader( const JsonObject &jo, std::string_view member_name, int &sym,
                             bool );





bool unicode_codepoint_from_symbol_reader(
    const JsonObject &jo, std::string_view member_name, uint32_t &member, bool );




bool unicode_symbol_reader( const JsonObject &jo, std::string_view member_name, std::string &member,
                            bool was_loaded );


float read_proportional_entry( const JsonObject &jo, std::string_view key );

namespace reader_detail
{

template<typename T, typename = std::void_t<>>
struct stringify : std::false_type {};

template<typename T, typename = std::void_t<>>
struct has_to_string : std::false_type {};

template<typename T>
struct has_to_string<T, std::void_t<decltype( std::declval<T &>().to_string() )>> :
std::true_type {};

template<typename T, typename = std::void_t<>>
struct has_to_string_writable : std::false_type {};

template<typename T>
struct has_to_string_writable<T, std::void_t<decltype( std::declval<T &>().to_string_writable() )>> :
std::true_type {};

template<typename T, typename = std::void_t<>>
struct has_to_string_call : std::false_type {};

template<typename T>
struct has_to_string_call<T, std::void_t<decltype( to_string( std::declval<T &>() ) )>> :
std::true_type {};

template<typename T, typename = std::void_t<>>
struct has_str : std::false_type {};

template<typename T>
struct has_str<T, std::void_t<decltype( std::declval<T &>().str() )> > : std::true_type {};

template<typename T>
struct stringify < T, std::enable_if_t < has_to_string<T>::value ||
                   has_to_string_writable<T>::value ||
                   has_to_string_call<T>::value ||
                   has_str<T>::value >> : std::true_type {
    static std::string string( const T &data ) {
        if constexpr( has_to_string<T>::value ) {
            return data.to_string();
        }
        if constexpr( has_to_string_writable<T>::value ) {
            return data.to_string_writable();
        }
        if constexpr( has_to_string_call<T>::value ) {
            return to_string( data );
        }
        if constexpr( has_str<T>::value ) {
            return data.str();
        }
    }
};

template<typename T, std::enable_if_t<stringify<T>::value>* = nullptr>
static std::string data_string( const T &data )
{
    return stringify<T>::string( data );
}

template < typename T, std::enable_if_t < !stringify<T>::value > * = nullptr >
static std::string data_string( const T & )
{
    return string_format( "type '%s'", demangle( typeid( T ).name() ) );
}


template<typename T>
struct handler {
    static constexpr bool is_container = false;
};

template<typename T>
struct handler<cata::flat_set<T>> {
    void clear( cata::flat_set<T> &container ) const {
        container.clear();
    }
    bool insert( cata::flat_set<T> &container, const T &data ) const {
        container.insert( data );
        return true;
    }
    bool relative( cata::flat_set<T> &, const T & ) const {
        return false;
    }
    bool erase( cata::flat_set<T> &container, const T &data ) const {
        if( container.erase( data ) < 1 ) {
            debugmsg( "Did not remove %s in delete", data_string( data ) );
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename T>
struct handler<std::set<T>> {
    void clear( std::set<T> &container ) const {
        container.clear();
    }
    bool insert( std::set<T> &container, const T &data ) const {
        container.insert( data );
        return true;
    }
    bool relative( std::set<T> &, const T & ) const {
        return false;
    }
    bool erase( std::set<T> &container, const T &data ) const {
        if( container.erase( data ) < 1 ) {
            debugmsg( "Did not remove %s in delete", data_string( data ) );
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename T>
struct handler<std::unordered_set<T>> {
    void clear( std::unordered_set<T> &container ) const {
        container.clear();
    }
    bool insert( std::unordered_set<T> &container, const T &data ) const {
        container.insert( data );
        return true;
    }
    bool relative( std::unordered_set<T> &, const T & ) const {
        return false;
    }
    bool erase( std::unordered_set<T> &container, const T &data ) const {
        if( container.erase( data ) < 1 ) {
            debugmsg( "Did not remove %s in delete", data_string( data ) );
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<size_t N>
struct handler<std::bitset<N>> {
    void clear( std::bitset<N> &container ) const {
        container.reset();
    }
    template<typename T>
    bool insert( std::bitset<N> &container, const T &data ) const {
        container.set( data );
        return true;
    }
    template<typename T>
    bool relative( std::bitset<N> &, const T & ) const {
        return false;
    }
    template<typename T>
    bool erase( std::bitset<N> &container, const T &data ) const {
        container.reset( data );
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename E>
struct handler<enum_bitset<E>> {
    void clear( enum_bitset<E> &container ) const {
        container.reset();
    }
    template<typename T>
    bool insert( enum_bitset<E> &container, const T &data ) const {
        container.set( data );
        return true;
    }
    template<typename T>
    bool relative( enum_bitset<E> &, const T & ) const {
        return false;
    }
    template<typename T>
    bool erase( enum_bitset<E> &container, const T &data ) const {
        container.reset( data );
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename T>
struct handler<std::vector<T>> {
    void clear( std::vector<T> &container ) const {
        container.clear();
    }
    bool insert( std::vector<T> &container, const T &data ) const {
        container.push_back( data );
        return true;
    }
    bool relative( std::vector<T> &, const T & ) const {
        return false;
    }
    template<typename E>
    bool erase( std::vector<T> &container, const E &data ) const {
        const auto pred = [&data]( const T & e ) {
            return e == data;
        };
        if( !erase_if( container, pred ) ) {
            debugmsg( "Did not remove %s in delete", data_string( data ) );
            return false;
        }
        return true;
    }
    template<typename P>
    bool erase_if( std::vector<T> &container, const P &predicate ) const {
        const auto iter = std::find_if( container.begin(), container.end(), predicate );
        if( iter != container.end() ) {
            container.erase( iter );
        } else {
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename T>
struct handler<weighted_int_list<T>> {
    void clear( weighted_int_list<T> &container ) const {
        container.clear();
    }
    bool insert( weighted_int_list<T> &container, const std::pair<T, int> &data ) const {
        return !!container.add_or_replace( data.first, data.second );
    }
    bool relative( weighted_int_list<T> &, const std::pair<T, int> & ) const {
        return false;
    }
    template<typename E>
    bool erase( weighted_int_list<T> &container, const E &data ) const {
        const auto pred = [&data]( const std::pair<T, int> &e ) {
            return e.first == data.first;
        };
        if( !erase_if( container, pred ) ) {
            debugmsg( "Did not remove %s in delete", data_string( data ) );
            return false;
        }
        return true;
    }
    template<typename P>
    bool erase_if( weighted_int_list<T> &container, P &predicate ) const {
        const auto iter = std::find_if( container.begin(), container.end(), predicate );
        if( iter != container.end() ) {
            container.remove( iter->first );
        } else {
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename T, typename W>
struct handler<weighted_list<T, W>> {
    void clear( weighted_list<T, W> &container ) const {
        container.clear();
    }
    bool insert( weighted_list<T, W> &container, const std::pair<T, W> &data ) const {
        return !!container.add_or_replace( data.first, data.second );
    }
    bool relative( weighted_list<T, W> &, const std::pair<T, W> & ) const {
        return false;
    }
    template<typename E>
    bool erase( weighted_list<T, W> &container, const E &data ) const {
        const auto pred = [&data]( const T & e ) {
            return e == data;
        };
        if( !erase_if( container, pred ) ) {
            debugmsg( "Did not remove %s in delete", data_string( data ) );
            return false;
        }
        return true;
    }
    template<typename P>
    bool erase_if( weighted_list<T, W> &container, P &predicate ) const {
        const auto iter = std::find_if( container.begin(), container.end(), predicate );
        if( iter != container.end() ) {
            container.remove( *iter.obj );
        } else {
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename Key, typename Val>
struct handler<std::map<Key, Val>> {
    void clear( std::map<Key, Val> &container ) const {
        container.clear();
    }
    bool insert( std::map<Key, Val> &container, const std::pair<Key, Val> &data ) const {

        if( !container.emplace( data ).second ) {
            debugmsg( "Insert of <%s, %s> failed, key already exists",
                      data_string( data.first ), data_string( data.second ) );
            return false;
        }
        return true;
    }
    bool relative( std::map<Key, Val> &container, const std::pair<Key, Val> &data ) const {
        if constexpr( !supports_relative<Val>::value ) {
            debugmsg( "relative not supported by type %s", demangle( typeid( Val ).name() ) );
            return false;
        } else {
            const auto iter = container.find( data.first );
            if( iter == container.end() ) {
                debugmsg( "No %s to perform relative of %s on",
                          data_string( data.first ),  data_string( data.second ) );
                return false;
            }
            iter->second += data.second;
        }
        return true;
    }
    bool erase( std::map<Key, Val> &container, const std::pair<Key, Val> &data ) const {
        const auto iter = container.find( data.first );
        if( iter != container.end() ) {
            container.erase( iter );
        } else {
            debugmsg( "Did not remove <%s, %s> in delete", data_string( data.first ),
                      data_string( data.second ) );
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};

template<typename Key, typename Val>
struct handler<std::unordered_map<Key, Val>> {
    void clear( std::unordered_map<Key, Val> &container ) const {
        container.clear();
    }
    bool insert( std::unordered_map<Key, Val> &container, const std::pair<Key, Val> &data ) const {

        if( !container.emplace( data ).second ) {
            debugmsg( "Insert of <%s, %s> failed, key already exists",
                      data_string( data.first ), data_string( data.second ) );
            return false;
        }
        return true;
    }
    bool relative( std::unordered_map<Key, Val> &container, const std::pair<Key, Val> &data ) const {
        if constexpr( !supports_relative<Val>::value ) {
            debugmsg( "relative not supported by type %s", demangle( typeid( Val ).name() ) );
            return false;
        } else {
            const auto iter = container.find( data.first );
            if( iter == container.end() ) {
                debugmsg( "No %s to perform relative of %s on",
                          data_string( data.first ),  data_string( data.second ) );
                return false;
            }
            iter->second += data.second;
        }
        return true;
    }
    bool erase( std::unordered_map<Key, Val> &container, const std::pair<Key, Val> &data ) const {
        const auto iter = container.find( data.first );
        if( iter != container.end() ) {
            container.erase( iter );
        } else {
            debugmsg( "Did not remove <%s, %s> in delete", data_string( data.first ),
                      data_string( data.second ) );
            return false;
        }
        return true;
    }
    static constexpr bool is_container = true;
};
}

template<typename T, typename = void>
struct T_has_do_extend : std::false_type {};

template<typename T>
struct T_has_do_extend<T, std::void_t<decltype( &T::do_extend )>> : std::true_type {};

template<typename T, typename = void>
struct T_has_do_delete : std::false_type {};

template<typename T>
struct T_has_do_delete<T, std::void_t<decltype( &T::do_delete )>> : std::true_type {};



































template<typename Derived>
class generic_typed_reader
{
public:
    static constexpr bool read_objects = false;
    static constexpr bool check_extend_delete_copy_from = true;

    template<typename C, typename Fn>

    void apply_all_values( JsonValue &jv, C &container, Fn apply ) const {
        if constexpr( Derived::read_objects ) {
            if( jv.test_array() ) {
                for( JsonValue jav : jv.get_array() ) {
                    apply( jav, container );
                }
            } else if( jv.test_object() ) {
                for( JsonMember jam : jv.get_object() ) {
                    apply( jam, container );
                }
            } else {
                apply( jv, container );
            }
        } else {
            if( jv.test_array() ) {
                for( JsonValue jav : jv.get_array() ) {
                    apply( jav, container );
                }
            } else {
                apply( jv, container );
            }
        }
    }



    template<typename C>
    void insert_values_from( const JsonObject &jo, const std::string_view member_name,
                             C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !jo.has_member( member_name ) ) {
            return;
        }
        JsonValue jv = jo.get_member( member_name );
        apply_all_values( jv, container, [&derived]( JsonValue & val, C & container ) {
            derived.insert_next( val, container );
        } );
    }

    template<typename C>
    void insert_next( JsonValue &jv, C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !reader_detail::handler<C>().insert( container, derived.get_next( jv ) ) ) {
            jv.throw_error( "insert failed" );
        }
    }

    template<typename C>
    void erase_values_from( const JsonObject &jo, const std::string_view member_name,
                            C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !jo.has_member( member_name ) ) {
            return;
        }
        JsonValue jv = jo.get_member( member_name );
        apply_all_values( jv, container, [&derived]( JsonValue & val, C & container ) {
            derived.erase_next( val, container );
        } );
    }
    template<typename C>
    void erase_next( JsonValue &jv, C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !reader_detail::handler<C>().erase( container, derived.get_next( jv ) ) ) {
            jv.throw_error( "no value to delete" );
        }
    }
    template<typename C>
    void relative_values_from( const JsonObject &jo, const std::string_view member_name,
                               C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !jo.has_member( member_name ) ) {
            return;
        }
        JsonValue jv = jo.get_member( member_name );
        apply_all_values( jv, container, [&derived]( JsonValue & val, C & container ) {
            derived.relative_next( val, container );
        } );
    }
    template<typename C>
    void relative_next( JsonValue &jv, C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !reader_detail::handler<C>().relative( container, derived.get_next( jv ) ) ) {
            jv.throw_error( "relative not supported or no value to modify" );
        }
    }








    template<typename C, std::enable_if_t<reader_detail::handler<C>::is_container, int> = 0>
    bool operator()( const JsonObject &jo, const std::string_view member_name,
                     C &container, bool was_loaded ) const {
        const Derived &derived = static_cast<const Derived &>( *this );



        if( jo.has_member( member_name ) ) {
            reader_detail::handler<C>().clear( container );
            derived.insert_values_from( jo, member_name, container );
            return true;
        } else if( !was_loaded ) {
            warn_disabled_feature( jo, "relative", member_name, "no copy-from" );
            warn_disabled_feature( jo, "proportional", member_name, "no copy-from" );
            warn_disabled_feature( jo, "extend", member_name, "no copy-from" );
            warn_disabled_feature( jo, "delete", member_name, "no copy-from" );
            return false;
        } else {
            warn_disabled_feature( jo, "proportional", member_name, "not implemented" );
            if( jo.has_object( "relative" ) ) {
                JsonObject tmp = jo.get_object( "relative" );
                tmp.allow_omitted_members();
                derived.relative_values_from( tmp, member_name, container );
            }
            if( jo.has_object( "extend" ) ) {
                JsonObject tmp = jo.get_object( "extend" );
                tmp.allow_omitted_members();
                derived.insert_values_from( tmp, member_name, container );
            }
            if( jo.has_object( "delete" ) ) {
                JsonObject tmp = jo.get_object( "delete" );
                tmp.allow_omitted_members();
                derived.erase_values_from( tmp, member_name, container );
            }
            return true;
        }
    }








    template < typename C, std::enable_if_t < !reader_detail::handler<C>::is_container,
               int > = 0,
               std::enable_if_t < !supports_relative<C>::value > * = nullptr
               >
    bool do_relative( const JsonObject &jo, const std::string_view name, C & ) const {
        if( jo.has_object( "relative" ) ) {
            JsonObject relative = jo.get_object( "relative" );
            relative.allow_omitted_members();
            if( !relative.has_member( name ) ) {
                return false;
            }
            debugmsg( "Member %s of type %s does not support relative",
                      name, demangle( typeid( C ).name() ) );
        }
        return false;
    }


    template < typename C, std::enable_if_t < !reader_detail::handler<C>::is_container,
               int > = 0, std::enable_if_t<supports_relative<C>::value> * = nullptr >
    bool do_relative( const JsonObject &jo, const std::string_view name, C &member ) const {
        if( jo.has_object( "relative" ) ) {
            JsonObject relative = jo.get_object( "relative" );
            relative.allow_omitted_members();
            const Derived &derived = static_cast<const Derived &>( *this );

            if( !relative.has_member( name ) ) {
                return false;
            }
            C adder = derived.get_next( relative.get_member( name ) );
            member += adder;
            return true;
        }
        return false;
    }


    template<typename T>
    bool do_extend( const JsonObject &jo, const std::string_view name, T &member ) const {
        if( !jo.has_member( name ) ) {
            return false;
        }
        if constexpr( T_has_do_extend<Derived>::value ) {
            const Derived &derived = static_cast<const Derived &>( *this );
            return derived.do_extend( jo, name, member );
        } else {
            jo.throw_error( string_format( "%s (reader %s) does not support extend outside of a container",
                                           name, demangle( typeid( Derived ).name() ) ) );
        }
        return false;
    }

    template<typename T>
    bool do_delete( const JsonObject &jo, const std::string_view name, T &member ) const {
        if( !jo.has_member( name ) ) {
            return false;
        }
        if constexpr( T_has_do_delete<Derived>::value ) {
            const Derived &derived = static_cast<const Derived &>( *this );
            return derived.do_delete( jo, name, member );
        } else {
            jo.throw_error( string_format( "%s (reader %s) does not support delete outside of a container",
                                           name, demangle( typeid( Derived ).name() ) ) );
        }
        return false;
    }

    template<typename C>
    bool read_normal( const JsonObject &jo, const std::string_view name, C &member ) const {
        if( jo.has_member( name ) ) {
            const Derived &derived = static_cast<const Derived &>( *this );
            member = derived.get_next( jo.get_member( name ) );
            return true;
        }
        return false;
    }






    template < typename C, std::enable_if_t < !reader_detail::handler<C>::is_container,
               int > = 0 >
    bool operator()( const JsonObject &jo, const std::string_view member_name,
                     C &member, bool was_loaded ) const {
        const Derived &derived = static_cast<const Derived &>( *this );

        if( !was_loaded ) {

            if( Derived::check_extend_delete_copy_from ) {
                warn_disabled_feature( jo, "extend", member_name, "no copy-from" );
                warn_disabled_feature( jo, "delete", member_name, "no copy-from" );
            }
            warn_disabled_feature( jo, "relative", member_name, "no copy-from" );
            warn_disabled_feature( jo, "proportional", member_name, "no copy-from" );
        }
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            do_extend( tmp, member_name, member );
        }
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            do_delete( tmp, member_name, member );
        }
        return derived.read_normal( jo, member_name, member ) ||

               handle_proportional( jo, member_name, member ) ||

               derived.do_relative( jo, member_name, member );
    }
};









template<typename T>
class json_read_reader : public generic_typed_reader<json_read_reader<T>>
{
public:
    static constexpr bool read_objects = true;

    T get_next( const JsonValue &jv ) const {
        T ret;
        if( !jv.read( ret ) ) {
            jv.throw_error( string_format( "Couldn't read %s", demangle( typeid( T ).name() ) ) );
        }
        return ret;
    }

    bool do_extend( const JsonObject &jo, const std::string_view name, T &member ) const {
        if( !jo.has_member( name ) ) {
            return false;
        }
        if constexpr( supports_extend_handler<T>::value ) {
            return member.handle_extend( jo.get_member( name ) );
        } else {
            jo.throw_error( string_format( "%s (type %s) does not implement handle_extend", name,
                                           demangle( typeid( T ).name() ) ) );
        }
        return false;
    }

    bool do_delete( const JsonObject &jo, const std::string_view name, T &member ) const {
        if( !jo.has_member( name ) ) {
            return false;
        }
        if constexpr( supports_delete_handler<T>::value ) {
            return member.handle_delete( jo.get_member( name ) );
        } else {
            jo.throw_error( string_format( "%s (type %s) does not implement handle_delete", name,
                                           demangle( typeid( T ).name() ) ) );
        }
        return false;
    }
};













template<typename FlagType = std::string>
class auto_flags_reader : public generic_typed_reader<auto_flags_reader<FlagType>>
{
public:
    FlagType get_next( std::string &&str ) const {
        return FlagType( std::move( str ) );
    }
};

using string_reader = auto_flags_reader<>;

class volume_reader : public generic_typed_reader<volume_reader>
{
    public:
        units::volume get_next( const JsonValue &jv ) const {
            return read_from_json_string<units::volume>( jv, units::volume_units );
        }
};

class mass_reader : public generic_typed_reader<mass_reader>
{
    public:
        units::mass get_next( const JsonValue &jv ) const {
            return read_from_json_string<units::mass>( jv, units::mass_units );
        }
};

class money_reader : public generic_typed_reader<money_reader>
{
    public:
        units::money get_next( const JsonValue &jv ) const {
            return read_from_json_string<units::money>( jv, units::money_units );
        }
};














template<typename T>
class typed_flag_reader : public generic_typed_reader<typed_flag_reader<T>>
{
private:
    using map_t = std::unordered_map<std::string, T>;

    const map_t &flag_map;
    const std::string flag_type;

public:
    typed_flag_reader( const map_t &flag_map, const std::string_view flag_type )
        : flag_map( flag_map )
        , flag_type( flag_type ) {
    }

    explicit typed_flag_reader( const std::string_view flag_type )
        : flag_map( io::get_enum_lookup_map<T>() )
        , flag_type( flag_type ) {
    }

    T get_next( const JsonValue &jv ) const {
        const std::string flag = jv;
        const auto iter = flag_map.find( flag );

        if( iter == flag_map.cend() ) {
            jv.throw_error( string_format( "invalid %s: \"%s\"", flag_type, flag ) );
        }

        return iter->second;
    }
};

template<typename T>
typed_flag_reader<T> make_flag_reader( const std::unordered_map<std::string, T> &m,
                                       const std::string_view e )
{
    return typed_flag_reader<T>( m, e );
}




template<typename E>
class enum_flags_reader : public generic_typed_reader<enum_flags_reader<E>>
{
private:
    const std::string flag_type;

public:
    explicit enum_flags_reader( const std::string &flag_type ) : flag_type( flag_type ) {
    }

    E get_next( const JsonValue &jv ) const {
        const std::string flag = jv.get_string();
        try {
            return io::string_to_enum<E>( flag );
        } catch( const io::InvalidEnumString & ) {
            jv.throw_error( string_format( "invalid %s: \"%s\"", flag_type, flag ) );
            throw;
        }
    }
};




template<typename T>
class string_id_reader : public generic_typed_reader<string_id_reader<T>>
{
public:
    string_id<T> get_next( std::string &&str ) const {
        return string_id<T>( std::move( str ) );
    }
};
















template<typename K, typename V>
class weighted_string_id_reader : public generic_typed_reader<weighted_string_id_reader<K, V>>
{
public:
    static constexpr bool read_objects = true;

    std::optional<V> default_weight;
    explicit weighted_string_id_reader( std::optional<V> default_weight ) : default_weight(
            default_weight ) {};

    std::pair<K, V> get_next( const JsonValue &val ) const {
        if( val.is_member() ) {
            const JsonMember &jm = dynamic_cast<const JsonMember &>( val );
            return std::pair<K, V>( jm.name(), static_cast<V>( val.get_float() ) );
        } else if( val.test_object() ) {
            JsonObject inline_pair = val.get_object();
            if( !( ( inline_pair.size() == 1 && default_weight.has_value() ) || inline_pair.size() == 2 ) ) {
                inline_pair.throw_error( "weighted_string_id_reader failed to read object" );
            }
            K pair_key;
            V pair_val = default_weight.value_or( V() );
            for( JsonMember mem : inline_pair ) {
                if( mem.test_string() ) {
                    pair_key = K( std::move( mem.get_string() ) );
                } else if( mem.test_float() ) {
                    pair_val = static_cast<V>( mem.get_float() );
                } else {
                    inline_pair.throw_error( "weighted_string_id_reader found unexpected value in object" );
                }
            }
            return std::pair<K, V>( pair_key, pair_val );
        } else if( val.test_array() ) {
            JsonArray arr = val.get_array();
            if( arr.size() != 2 ) {
                arr.throw_error( "weighted_string_id_reader read array without exactly two entries" );
            }
            return std::pair<K, V>(
                       K( std::move( arr[0].get_string() ) ),
                       static_cast<V>( arr[1].get_float() ) );
        } else if( default_weight.has_value() ) {
            if( val.test_string() ) {
                return std::pair<K, V>(
                           K( std::move( val.get_string() ) ), *default_weight );
            }
            val.throw_error( "weighted_string_id_reader provided with invalid string_id" );
        } else {
            val.throw_error( "weighted_string_id_reader requires two entries" );
        }
    }
};




template<typename K, typename V>
class generic_map_reader : public generic_typed_reader<generic_map_reader<K, V>>
{
public:
    static constexpr bool read_objects = true;

    static_assert( key_from_json_string<K>::valid, "Type K must have a known conversion from string" );

    std::pair<K, V> get_next( const JsonValue &jv ) const {
        const JsonMember *jm = dynamic_cast<const JsonMember *>( &jv );
        if( jm == nullptr ) {
            jv.throw_error( "not part of a JsonObject" );
        }
        K key = key_from_json_string<K>()( jm->name() );
        V value;
        jv.read( value, true );
        return std::pair<K, V>( key, value );
    }
};











template<typename T, typename U = T>
class pair_reader : public generic_typed_reader<pair_reader<T, U>>
{
public:
    std::pair<T, U> get_next( const JsonValue &jv ) const {

        T a;
        U b;

        if( jv.test_array() ) {
            JsonArray ja = jv.get_array();
            if( ja.size() != 2 ) {
                ja.throw_error( "Must have 2 elements" );
            }
            ja[0].read( a, true );
            ja[1].read( b, true );
            return std::make_pair( a, b );
        }

        if constexpr( key_from_json_string<T>::valid ) {
            if( jv.test_object() ) {
                JsonObject jo = jv.get_object();
                if( jo.size() != 1 ) {
                    jo.throw_error( "bad object pair, has too many elements" );
                }

                for( JsonMember jm : jo ) {
                    a = key_from_json_string<T>()( jm.name() );
                    jm.read( b, true );
                    return std::make_pair( a, b );
                }
            }
        }

        if constexpr( std::is_same_v<T, U> ) {
            jv.read( a, true );
            b = a;
            return std::make_pair( a, b );
        }
        jv.throw_error( "bad pair" );
        return std::make_pair( a, b );
    }
};




template<typename T1, typename T2>
class named_pair_reader : public generic_typed_reader<named_pair_reader<T1, T2>>
{
public:
    std::string_view key1;
    std::string_view key2;
    T2 default_value2;

    named_pair_reader( std::string_view _key1, std::string_view _key2,
                       T2 _default = T2() ) : key1( _key1 ), key2( _key2 ), default_value2( _default ) {
        cata_assert( !key1.empty() );
        cata_assert( !key2.empty() );
        cata_assert( key1 != key2 );
    }

    std::pair<T1, T2> get_next( const JsonValue &jv ) const {
        std::pair<T1, T2> ret;
        if( jv.test_object() ) {
            JsonObject jo = jv.get_object();
            jo.read( key1, ret.first, true );
            jo.read( key2, ret.second, true );
            return ret;
        }

        if( jv.test_array() ) {
            jv.throw_error( "invalid format" );
        }
        jv.read( ret.first, true );
        ret.second = default_value2;
        return ret;
    }
};

class nc_color;
class nc_color_reader : public generic_typed_reader<nc_color_reader>
{
    public:
        nc_color get_next( const JsonValue &jv ) const;
};

template<typename T>
static T bound_check( T low, T high, const JsonValue &jv, T read_value )
{
    if( read_value < low ) {
        jv.throw_error( string_format( "value below bound_reader's defined low bound" ) );
        return low;
    } else if( read_value > high ) {
        jv.throw_error( string_format( "value above bound_reader's defined high bound" ) );
        return high;
    }
    return read_value;
}

template<typename T, typename = std::void_t<>>
struct supports_units : std::false_type {};

template<typename T>
struct supports_units < T, std::void_t < decltype( std::is_base_of<quantity, T>() )
                        >> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_time : std::false_type {};

template<typename T>
struct supports_time < T, std::void_t < decltype( std::is_base_of<time_duration, T>() )
                                        >> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_primitives : std::false_type {};

template<typename T>
struct supports_primitives < T, std::void_t < decltype( std::is_arithmetic_v<T> )
                             >> : std::true_type {};









template <typename T>
class bound_reader : public generic_typed_reader<bound_reader<T>>
{
public:
    T low;
    T high;

    bound_reader() = default;

    bool do_relative( const JsonObject &jo, const std::string_view name, T &member ) const {
        if( jo.has_object( "relative" ) ) {
            JsonObject relative = jo.get_object( "relative" );
            relative.allow_omitted_members();

            if( !relative.has_member( name ) ) {
                return false;
            }
            JsonValue jv = relative.get_member( name );
            T adder = read( jv );
            member += adder;

            member = bound_check( low, high, jv, member );
            return true;
        }
        return false;
    }


    T read( const JsonValue &jv ) const {
        T read_member;
        if( !jv.read( read_member ) ) {
            jv.throw_error( "bound_reader failed to read value" );
        }
        return read_member;
    }

    T get_next( const JsonValue &jv ) const {
        return bound_check<T>( low, high, jv, read( jv ) );
    }
};


template<typename T, std::enable_if_t<supports_primitives<T>::value >* = nullptr >
class numeric_bound_reader : public bound_reader<T>
{
public:
    explicit numeric_bound_reader( T low = std::numeric_limits<T>::lowest(),
                                   T high = std::numeric_limits<T>::max() ) {
        bound_reader<T>::low = low;
        bound_reader<T>::high = high;
    };
};


template<typename T, std::enable_if_t<supports_units<T>::value >* = nullptr >
class units_bound_reader : public bound_reader<T>
{
public:
    explicit units_bound_reader( T low = T::min(), T high = T::max() ) {
        bound_reader<T>::low = low;
        bound_reader<T>::high = high;
    };
};


template <typename T = time_duration, std::enable_if_t<supports_time<T>::value >* = nullptr >
class time_bound_reader : public bound_reader<T>
{
public:
    explicit time_bound_reader( T low = 0_seconds,
                                T high = calendar::INDEFINITELY_LONG_DURATION ) {
        bound_reader<T>::low = low;
        bound_reader<T>::high = high;
    };
};

struct percentile_reader : public generic_typed_reader<percentile_reader> {
    double lower;
    double upper;

    explicit percentile_reader( double l = std::numeric_limits<double>::max(),
                                double h = std::numeric_limits<double>::max() ) : lower( l ), upper( h ) {}

    double get_next( const JsonValue &jv ) const {
        double ret = jv.get_float() / 100.0;
        return bound_check( lower, upper, jv, ret );
    }
};

struct weakpoints;

struct weakpoints_reader : generic_typed_reader<weakpoints_reader> {
    static constexpr bool check_extend_delete_copy_from = false;

    std::set<std::string> &deleted;

    explicit weakpoints_reader( std::set<std::string> &del ) : deleted( del ) {}

    weakpoints get_next( const JsonValue &jv ) const;
    bool do_extend( const JsonObject &jo, std::string_view name, weakpoints &member ) const;
    bool do_delete( const JsonObject &jo, std::string_view name, weakpoints &member ) const;
};





class text_style_check_reader : public generic_typed_reader<text_style_check_reader>
{
    public:
        enum class allow_object : int {
            no,
            yes,
        };

        explicit text_style_check_reader( allow_object object_allowed = allow_object::yes );

        std::string get_next( const JsonValue &jv ) const;

    private:
        allow_object object_allowed;
};

struct time_duration_as_moves_reader : public generic_typed_reader<time_duration_as_moves_reader> {
    int64_t get_next( const JsonValue &jv ) const {
        time_duration ret;
        jv.read( ret );
        return to_moves<int64_t>( ret );
    }
};

class activity_level_reader : public generic_typed_reader<activity_level_reader>
{
    public:
        float get_next( const JsonValue &jv ) const;
};

#endif
