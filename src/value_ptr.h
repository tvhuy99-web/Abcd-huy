#pragma once
#ifndef CATA_SRC_VALUE_PTR_H
#define CATA_SRC_VALUE_PTR_H

#include <memory>

class JsonOut;
class JsonValue;

namespace cata
{





template <class T>
class value_ptr : public std::unique_ptr<T>
{
    public:
        value_ptr() = default;
        value_ptr( value_ptr && ) noexcept = default;
        // NOLINTNEXTLINE(google-explicit-constructor)
        value_ptr( std::nullptr_t ) {}
        explicit value_ptr( T *value ) : std::unique_ptr<T>( value ) {}
        value_ptr( const value_ptr<T> &other ) :
            std::unique_ptr<T>( other ? new T( *other ) : nullptr ) {}
        value_ptr &operator=( value_ptr<T> other ) noexcept {
            std::unique_ptr<T>::operator=( std::move( other ) );
            return *this;
        }

        template<typename Stream = JsonOut>
        void serialize( Stream &jsout ) const {
            if( this->get() ) {
                this->get()->serialize( jsout );
            } else {
                jsout.write_null();
            }
        }
        template<typename Value = JsonValue, std::enable_if_t<std::is_same_v<std::decay_t<Value>, JsonValue>>* = nullptr>
        void deserialize( const Value &jsin ) {
            if( jsin.test_null() ) {
                this->reset();
            } else {
                this->reset( new T() );
                this->get()->deserialize( jsin );
            }
        }
};

template <class T, class... Args>
value_ptr<T> make_value( Args &&...args )
{
    return value_ptr<T>( new T( std::forward<Args>( args )... ) );
}

template <class T>
bool value_ptr_equals( const value_ptr<T> &lhs, const value_ptr<T> &rhs )
{
    return ( !lhs && !rhs ) || ( lhs && rhs && *lhs == *rhs );
}










template <class T>
struct heap {
    private:
        std::unique_ptr<T> heaped_;

    public:
        template<typename ...Args>
        // NOLINTNEXTLINE(google-explicit-constructor)
        heap( Args &&...args ) : heaped_{ new T{ std::forward<Args>( args )... } } {}



        heap( heap && ) noexcept = default;
        heap &operator=( heap &&other ) noexcept = default;


        heap( heap const &rhs ) {
            *this = rhs;
        }
        heap &operator=( heap const &rhs ) {
            if( this == &rhs ) {
                return *this;
            }

            if( rhs.heaped_ ) {
                heaped_.reset( new T{ *rhs.heaped_ } );
            } else {
                heaped_.reset( new T{} );
            }
            return *this;
        }

        heap &operator=( T const &t ) {
            heaped_.reset( new T{ t } );
            return *this;
        }

        heap &operator=( T &&t ) {
            heaped_.reset( new T{ std::move( t ) } );
            return *this;
        }


        // NOLINTNEXTLINE(google-explicit-constructor)
        operator T &() & {
            return *heaped_;
        }
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator T const &() const & {
            return *heaped_;
        }

        // NOLINTNEXTLINE(google-explicit-constructor)
        operator T() && {
            return std::move( *heaped_ );
        }






        T &operator*() & {
            return *heaped_;
        }
        T const &operator*() const & {
            return *heaped_;
        }

        T operator*() && {
            return std::move( *heaped_ );
        }

    private:

        T &val() {
            return *heaped_;
        }
        T const &val() const {
            return *heaped_;
        }
    public:


#pragma push_macro("PROXY")
#define PROXY(func) \
    template<typename ...Us> \
    auto func( Us&& ...us ) -> decltype( val().func( std::forward<Us>( us )... ) ) { \
        return val().func( std::forward<Us>( us )... ); \
    }
#pragma push_macro("PROXY_CONST")
#define PROXY_CONST(func) \
    template<typename ...Us> \
    auto func( Us&& ...us ) const -> decltype( val().func( std::forward<Us>( us )... ) ) { \
        return val().func( std::forward<Us>( us )... ); \
    }


        auto operator==( heap const &rhs ) const -> decltype( val() == val() ) {
            const T *lhsp = heaped_.get();
            const T *rhsp = rhs.heaped_.get();

            return rhsp && lhsp && *rhsp == *lhsp;
        }

        auto operator!=( heap const &rhs ) const -> decltype( val() != val() ) {
            const T *lhsp = heaped_.get();
            const T *rhsp = rhs.heaped_.get();

            return rhsp && lhsp && *rhsp != *lhsp;
        }



        PROXY_CONST( empty )
        PROXY_CONST( count )
        PROXY_CONST( size )
        PROXY( clear )


        PROXY( begin )
        PROXY_CONST( begin )
        PROXY( end )
        PROXY_CONST( end )


        template<typename U>
        auto operator[]( U &&u ) -> decltype( val()[std::forward<U>( u )] ) {
            return val()[std::forward<U>( u )];
        }

        PROXY( find )
        PROXY_CONST( find )

        PROXY( erase )
        PROXY( insert )
        PROXY( emplace )


        template<typename Stream = JsonOut>
        void serialize( Stream &jsout ) const {
            jsout.write( val() );
        }

        template<typename Value = JsonValue, std::enable_if_t<std::is_same_v<std::decay_t<Value>, JsonValue>>* = nullptr>
        void deserialize( const Value &jsin ) {
            jsin.read( val() );
        }
#pragma pop_macro("PROXY")
#pragma pop_macro("PROXY_CONST")
};

}

#endif
