#include "safe_reference.h"

#include <type_traits>

static_assert( std::is_nothrow_move_constructible_v<safe_reference_anchor> );
static_assert( std::is_nothrow_move_assignable_v<safe_reference_anchor> );

safe_reference_anchor::safe_reference_anchor()
{
    impl = std::make_shared<empty>();
}

safe_reference_anchor::safe_reference_anchor( const safe_reference_anchor & ) :
    safe_reference_anchor()
{}




safe_reference_anchor::safe_reference_anchor( safe_reference_anchor && ) noexcept :
    safe_reference_anchor()
{}

safe_reference_anchor &safe_reference_anchor::operator=( const safe_reference_anchor &other )
{
    if( this != &other ) {
        impl = std::make_shared<empty>();
    }
    return *this;
}

safe_reference_anchor &safe_reference_anchor::operator=( safe_reference_anchor && ) noexcept
{
    impl = std::make_shared<empty>();
    return *this;
}
