#pragma once
#ifndef CATA_SRC_STRING_ID_H
#define CATA_SRC_STRING_ID_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

constexpr int64_t INVALID_VERSION = -1;
constexpr int INVALID_CID = -1;

template<typename T>
class generic_factory;
template<typename T>
class int_id;
template<typename T>
struct lexicographic;

































































template<typename T>
struct string_id_params {
    static constexpr bool dynamic = false;
};




class Item_spawn_data;

template<> struct string_id_params<Item_spawn_data> {
    static constexpr bool dynamic = true;
};
class Trait_group;

template<> struct string_id_params<Trait_group> {
    static constexpr bool dynamic = true;
};

#if !defined(RELEASE) && !defined(CATA_STRING_ID_DEBUGGING)
#define CATA_STRING_ID_DEBUGGING
#endif






class string_identity_static
{
    private:
        int _id;
#ifdef CATA_STRING_ID_DEBUGGING



        const char *_string_id = nullptr;
#endif
        string_identity_static()
            : _id( empty_interned_string() )
#ifdef CATA_STRING_ID_DEBUGGING
            , _string_id( "" )
#endif
        {}

        template<typename S, class = std::enable_if_t<std::is_convertible_v<S, std::string>>>
                 explicit string_identity_static( S && id )
                     : _id( string_id_intern( std::forward<S>( id ) ) )
#ifdef CATA_STRING_ID_DEBUGGING
            , _string_id( str().c_str() )
#endif
        {}

        inline const std::string &str() const {
            return get_interned_string( _id );
        }

        inline bool is_empty() const {
            return _id == empty_interned_string();
        }


        static int string_id_intern( std::string &&s );
        static int string_id_intern( std::string &s );
        static int string_id_intern( const std::string &s );


        static const std::string &get_interned_string( int id );

        static int empty_interned_string();

        template<typename T>
        friend class string_id;

        template<typename T>
        friend struct std::hash; // NOLINT(cert-dcl58-cpp)
};








class string_identity_dynamic
{
    private:
        std::string _id;

        string_identity_dynamic() = default;

        template<typename S, class = std::enable_if_t<std::is_convertible_v<S, std::string>>>
                 explicit string_identity_dynamic( S && id ) : _id( std::forward<S>( id ) )  {}

        inline const std::string &str() const {
            return _id;
        }

        inline bool is_empty() const {
            return _id.empty();
        }

        template<typename T>
        friend class string_id;

        template<typename T>
        friend struct std::hash; // NOLINT(cert-dcl58-cpp)
};

template<typename T>
class string_id
{
public:
    using value_type = T;
    using This = string_id<T>;

    using Identity = std::conditional_t<string_id_params<T>::dynamic,
          string_identity_dynamic, string_identity_static>;

    using LexCmp = lexicographic<T>;








    template<typename S, class = std::enable_if_t<std::is_convertible_v<S, std::string>>>
             explicit string_id( S && id ) : _id( std::forward<S>( id ) ) {}



    explicit string_id( const std::string_view id ) : string_id( std::string( id ) ) {}





    string_id() : _id() {} // NOLINT(clang-analyzer-optin.cplusplus.UninitializedObject)





    bool operator<( const This &rhs ) const {
        return _id._id < rhs._id._id;
    }



    bool operator==( const This &rhs ) const {
        return _id._id == rhs._id._id;
    }



    bool operator!=( const This &rhs ) const {
        return ! operator==( rhs );
    }





    const char *c_str() const {
        return _id.str().c_str();
    }





    const std::string &str() const {
        return _id.str();
    }

    explicit operator std::string() const {
        return _id.str();
    }







    int_id<T> id() const;





    int_id<T> id_or( const int_id<T> &fallback ) const;



    const T &obj() const;

    const T &operator*() const {
        return obj();

    }
    const T *operator->() const {
        return &obj();
    }




    bool is_valid() const;





    bool is_empty() const {
        return _id.is_empty();
    }




    static const string_id<T> &NULL_ID();











    bool is_null() const {
        return operator==( NULL_ID() );
    }













    explicit operator bool() const {
        return !is_null();
    }

    friend std::ostream &operator<<( std::ostream &os, const string_id &s ) {
        os << s.str();
        return os;
    }

private:

    mutable int64_t _version = INVALID_VERSION;

    mutable int _cid = INVALID_CID;

    Identity _id;

    inline void set_cid_version( int cid, int64_t version ) const {
        _cid = cid;
        _version = version;
    }

    friend class generic_factory<T>;
    friend struct std::hash<string_id<T>>;
};


template<typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<string_id<T>> {
    std::size_t operator()( const string_id<T> &v ) const noexcept {
        using IdType = decltype( v._id._id );
        return std::hash<IdType>()( v._id._id );
    }
};


template<typename T>
struct lexicographic {
    bool operator()( const string_id<T> &x, const string_id<T> &y ) const {

        // NOLINTNEXTLINE cata-use-localized-sorting
        return x.str() < y.str();
    }
};

#endif
