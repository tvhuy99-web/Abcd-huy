#include "a11y_quantity_selection.h"

namespace a11y
{
std::optional<std::size_t> selected_quantity( const std::string_view text,
        const std::size_t available )
{
    if( text.empty() ) {
        return std::nullopt;
    }
    std::size_t value = 0;
    for( const char c : text ) {
        if( c < '0' || c > '9' ) {
            return std::nullopt;
        }
        const std::size_t digit = static_cast<std::size_t>( c - '0' );
        if( digit > available || value > ( available - digit ) / 10 ) {
            return std::nullopt;
        }
        value = value * 10 + digit;
    }
    return value;
}

} // namespace a11y
