#pragma once
#ifndef CATA_SRC_CATA_VIEWS_H
#define CATA_SRC_CATA_VIEWS_H

#include <functional>

namespace cata
{
namespace views
{
























template<typename A, typename B>
class transform
{
    private:
        using fun_t = std::function<B( const typename A::value_type & )>;

        fun_t fun;
        typename A::const_iterator it_begin, it_end;

        class iterator
        {
            private:
                typename A::const_iterator it;
                const fun_t &fun;
            public:
                explicit iterator( const typename A::const_iterator &it, const fun_t &fun )
                    : it( it ), fun( fun ) {}
                bool operator != ( const iterator &rhs ) const {
                    return this->it != rhs.it;
                }
                const iterator &operator++() {
                    ++it;
                    return *this;
                }
                B operator*() const {
                    return fun( *it );
                }

                using iterator_category = std::input_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = B;
                using pointer = value_type*;
                using reference = value_type&;
        };
    public:
        explicit transform( const A &container, const fun_t &fun ) : fun( fun ),
            it_begin( std::begin( container ) ), it_end( std::end( container ) ) {}
        explicit transform( const typename A::const_iterator &it_begin,
                            const typename A::const_iterator &it_end,
                            const fun_t &fun )
            : fun( fun ), it_begin( it_begin ), it_end( it_end ) {}
        iterator begin() {
            return iterator( it_begin, fun );
        }
        iterator end() {
            return iterator( it_end, fun );
        }
};
}
}

#endif
