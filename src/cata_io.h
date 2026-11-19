#pragma once
#ifndef CATA_SRC_CATA_IO_H
#define CATA_SRC_CATA_IO_H

#include <functional>
#include <string>
#include <type_traits>

#include "json.h"











































































namespace io
{









struct default_tag { };







struct empty_default_tag { };




struct required_tag { };









namespace detail
{

template<class T, class R = void>
struct enable_if_type {
    using type = R;
};






template<class T, class E = void>
struct has_archive_tag : std::false_type {
    template < typename S, typename TT = T,
               std::enable_if_t < std::is_enum_v<TT> &&std::is_same_v<S, TT> > * = nullptr >
    static void write( JsonOut &stream, const S &value ) {


        stream.write_as_string( value );
    }
    template < typename S, typename TT = T,
               std::enable_if_t < !std::is_enum_v<TT> &&std::is_same_v<S, TT> > * = nullptr >
    static void write( JsonOut &stream, const S &value ) {
        stream.write( value );
    }
    static bool read( const JsonObject &obj, const std::string &key, T &value ) {
        return obj.read( key, value );
    }
    static bool read( JsonArray &arr, T &value ) {
        return arr.read_next( value );
    }
};





template<class T>
struct has_archive_tag<T, typename enable_if_type<typename T::archive_type_tag>::type> :
    std::true_type {
    using InArchive = typename T::archive_type_tag::InputArchive;

    static void write( JsonOut &stream, const T &value ) {
        typename T::archive_type_tag::OutputArchive archive( stream );
        const_cast<T &>( value ).io( archive );
    }
    static bool read( const JsonObject &obj, const std::string_view key, T &value ) {
        if( !obj.has_member( key ) ) {
            return false;
        }
        InArchive archive( obj, key );
        value.io( archive );
        return true;
    }
    static bool read( JsonArray &arr, T &value ) {
        if( !arr.has_more() ) {
            return false;
        }
        InArchive archive( arr );
        value.io( archive );
        return true;
    }
};

}






class JsonObjectInputArchive : public JsonObject
{
    public:
        using is_input = std::true_type;

        explicit JsonObjectInputArchive( const JsonObject &jo )
            : JsonObject( jo ) {
        }

        explicit JsonObjectInputArchive( JsonArray & );

        JsonObjectInputArchive( const JsonObject &, std::string_view key );














        template<typename T>
        bool io( const std::string &name, T &value ) {
            return io::detail::has_archive_tag<T>::read( *this, name, value );
        }




        template<typename T>
        bool io( const std::string &name, T &value, const T &default_value ) {
            if( io( name, value ) ) {
                return true;
            }
            value = default_value;
            return false;
        }




        template<typename T>
        bool io( const std::string &name, T &value, default_tag ) {
            static const T default_value = T();
            return io( name, value, default_value );
        }




        template<typename T>
        bool io( const std::string &name, T &value, empty_default_tag ) {
            if( io( name, value ) ) {
                return true;
            }
            value.clear();
            return false;
        }































        template<typename T>
        bool io( const std::string &name, T *&pointer,
                 const std::function<void( const std::string & )> &load,
                 const std::function<std::string( const T & )> &save, bool required = false ) {

            ( void ) save;
            std::string ident;
            if( !io( name, ident ) ) {
                if( required ) {
                    JsonObject::throw_error( std::string( "required member is missing: " ) + name );
                }
                pointer = nullptr;
                return false;
            }
            load( ident );
            return true;
        }



        template<typename T, typename LoadFunc, typename SaveFunc>
        bool io( const std::string &name, T &pointer,
                 const LoadFunc &load,
                 const SaveFunc &save, bool required = false ) {

            ( void ) save;
            std::string ident;
            if( !io( name, ident ) ) {
                if( required ) {
                    JsonObject::throw_error( std::string( "required member is missing: " ) + name );
                }
                pointer = nullptr;
                return false;
            }
            load( ident );
            return true;
        }
        template<typename T>
        bool io( const std::string &name, T *&pointer,
                 const std::function<void( const std::string & )> &load,
                 const std::function<std::string( const T & )> &save, required_tag ) {
            return io<T>( name, pointer, load, save, true );
        }

};









class JsonArrayInputArchive : public JsonArray
{
    public:
        using is_input = std::true_type;

        explicit JsonArrayInputArchive( const JsonArray &jo )
            : JsonArray( jo ) {
        }

        explicit JsonArrayInputArchive( JsonArray & );

        JsonArrayInputArchive( const JsonObject &, std::string_view key );

        template<typename T>
        bool io( T &value ) {
            return io::detail::has_archive_tag<T>::read( *this, value );
        }
};

inline JsonArrayInputArchive::JsonArrayInputArchive( JsonArray &arr )
    : JsonArray( arr.next_array() ) { }
inline JsonArrayInputArchive::JsonArrayInputArchive( const JsonObject &obj,
        const std::string_view key )
    : JsonArray( obj.get_array( key ) ) { }

inline JsonObjectInputArchive::JsonObjectInputArchive( JsonArray &arr )
    : JsonObject( arr.next_object() ) { }
inline JsonObjectInputArchive::JsonObjectInputArchive( const JsonObject &obj,
        const std::string_view key )
    : JsonObject( obj.get_object( key ) ) { }









class JsonObjectOutputArchive
{
    public:
        using is_input = std::false_type;

        JsonOut &stream;

        explicit JsonObjectOutputArchive( JsonOut &stream )
            : stream( stream ) {
            stream.start_object();
        }
        ~JsonObjectOutputArchive() {
            stream.end_object();
        }













        template<typename T>
        bool io( const std::string_view name, const T &value ) {
            stream.member( name );
            io::detail::has_archive_tag<T>::write( stream, value );
            return false;
        }
        template<typename T>
        bool io( const std::string &name, const T &value, const T &default_value ) {
            if( value == default_value ) {
                return false;
            }
            return io( name, value );
        }
        template<typename T>
        bool io( const std::string &name, const T &value, default_tag ) {
            static const T default_value = T();
            return io<T>( name, value, default_value );
        }
        template<typename T>
        bool io( const std::string &name, const T &value, empty_default_tag ) {
            if( !value.empty() ) {
                io<T>( name, value );
            }
            return false;
        }










        template<typename T>
        bool io( const std::string &name, const T *pointer,
                 const std::function<void( const std::string & )> &,
                 const std::function<std::string( const T & )> &save, bool required = false ) {
            if( pointer == nullptr ) {
                if( required ) {
                    throw JsonError( "a required member is null: " + name );
                }
                return false;
            }
            return io( name, save( *pointer ) );
        }



        template<typename T, typename LoadFunc, typename SaveFunc>
        bool io( const std::string &name, const T &pointer,
                 const LoadFunc &,
                 const SaveFunc &save, bool required = false ) {
            if( pointer == nullptr ) {
                if( required ) {
                    throw JsonError( "a required member is null: " + name );
                }
                return false;
            }
            return io( name, save( pointer ) );
        }
        template<typename T>
        bool io( const std::string &name, const T *pointer,
                 const std::function<void( const std::string & )> &load,
                 const std::function<std::string( const T & )> &save, required_tag ) {
            return io<T>( name, pointer, load, save, true );
        }





        template<typename T>
        bool read( const std::string_view, T & ) {
            return false;
        }
};







class JsonArrayOutputArchive
{
    public:
        using is_input = std::false_type;

        JsonOut &stream;

        explicit JsonArrayOutputArchive( JsonOut &stream )
            : stream( stream ) {
            stream.start_array();
        }
        ~JsonArrayOutputArchive() {
            stream.end_array();
        }

        template<typename T>
        bool io( const T &value ) {
            io::detail::has_archive_tag<T>::write( stream, value );
            return false;
        }
};

}

#endif
