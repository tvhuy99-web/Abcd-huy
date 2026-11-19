#pragma once
#ifndef CATA_SRC_INT_ID_H
#define CATA_SRC_INT_ID_H

#include <bitset>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_set>

template<typename T>
class string_id;







template<typename T>
class int_id
{
    public:
        using This = int_id<T>;





        explicit int_id( const int id )
            : _id( id ) {
        }




        template < typename S, typename std::enable_if_t < !std::is_same_v<S, T>, int > = 0 >
        int_id( const int_id<S> &id ) = delete;





        int_id()
            : _id( 0 ) {
        }




        // NOLINTNEXTLINE(google-explicit-constructor)
        int_id( const string_id<T> &id );






        template<typename S, class =
                 std::enable_if_t< std::is_convertible_v<S, std::string >> >
        explicit int_id( S && id ) : int_id( string_id<T>( std::forward<S>( id ) ) ) {}




        bool operator<( const This &rhs ) const {
            return _id < rhs._id;
        }



        bool operator==( const This &rhs ) const {
            return _id == rhs._id;
        }



        bool operator!=( const This &rhs ) const {
            return _id != rhs._id;
        }






        int to_i() const {
            return _id;
        }



        explicit operator int() const {
            return _id;
        }

        explicit operator bool() const {
            return _id != 0;
        }



        const string_id<T> &id() const;
        const T &obj() const;

        const T &operator*() const {
            return obj();
        }

        const T *operator->() const {
            return &obj();
        }




        bool is_valid() const;

    private:
        int _id;
};



template <typename T, std::size_t kFastSize>
class int_id_set
{
    public:
        template <typename... Ts>
        void emplace( Ts &&... ts ) {
            insert( int_id<T>( std::forward<Ts>( ts )... ) );
        }

        void insert( int_id<T> id ) {
            const std::size_t i = static_cast<std::size_t>( id.to_i() );
            if( i < fast_set_.size() ) {
                fast_set_[i] = true;
            } else {
                slow_set_.insert( id );
            }
        }

        void erase( int_id<T> id ) {
            const std::size_t i = static_cast<std::size_t>( id.to_i() );
            if( i < fast_set_.size() ) {
                fast_set_[i] = false;
            } else {
                slow_set_.erase( id );
            }
        }

        bool contains( int_id<T> id ) const {
            const std::size_t i = static_cast<std::size_t>( id.to_i() );
            if( i < fast_set_.size() ) {
                return fast_set_[i];
            } else {
                return slow_set_.count( id );
            }
        }

        void clear() {
            fast_set_.reset();
            slow_set_.clear();
        }

    private:
        std::bitset<kFastSize> fast_set_;
        std::unordered_set<int_id<T>> slow_set_;
};


template<typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<int_id<T>> {
    std::size_t operator()( const int_id<T> &v ) const noexcept {
        return hash<int>()( v.to_i() );
    }
};

#endif
