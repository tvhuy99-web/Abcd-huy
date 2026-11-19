#pragma once
#ifndef CATA_SRC_PIMPL_H
#define CATA_SRC_PIMPL_H

#include <memory>
#include <type_traits>

class JsonOut;
class JsonValue;

template<typename T>
class pimpl;
template<typename ...T>
class is_pimpl_helper : public std::false_type
{
};
template<typename T>
class is_pimpl_helper<pimpl<T>> : public std::true_type
{
};
template<typename T>
class is_pimpl : public is_pimpl_helper<std::decay_t<T>>
{
};











template <typename T>
class pimpl : private std::unique_ptr<T>
{
    public:




        explicit pimpl() : std::unique_ptr<T>( new T() ) { }
        template < typename P, typename ...Args,
                   typename = std::enable_if_t < !is_pimpl<P>::value > >
        explicit pimpl( P && head, Args &&
                        ... args ) : std::unique_ptr<T>( new T( std::forward<P>( head ), std::forward<Args>( args )... ) ) { }

        pimpl( const pimpl<T> &rhs ) : std::unique_ptr<T>( new T( *rhs ) ) { }
        pimpl( pimpl<T> &&rhs ) noexcept : std::unique_ptr<T>( new T( std::move( *rhs ) ) ) { }

        pimpl<T> &operator=( const pimpl<T> &rhs ) {
            operator*() = *rhs;
            return *this;
        }
        pimpl<T> &operator=( pimpl<T> &&rhs ) noexcept {
            operator*() = std::move( *rhs );
            return *this;
        }

        bool operator==( const pimpl<T> &rhs ) const {
            return operator*() == *rhs;
        }

        using std::unique_ptr<T>::operator->;
        using std::unique_ptr<T>::operator*;


        void deserialize( const JsonValue &stream ) {
            operator*().deserialize( stream );
        }

        void serialize( JsonOut &stream ) const {
            operator*().serialize( stream );
        }
};

#endif
