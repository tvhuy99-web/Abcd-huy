#pragma once
#ifndef CATA_SRC_A11Y_QUANTITY_SELECTION_H
#define CATA_SRC_A11Y_QUANTITY_SELECTION_H

#include <cstddef>
#include <optional>
#include <string_view>

namespace a11y
{

// A quantity choice is complete when the reader selects a number.  Parse
// strictly against the *live* available count instead of trusting a cached
// Android snapshot; 0 is a deliberate deselection.  This helper is shared by
// the full inventory selectors and the accessible one-tile pickup menu.
inline std::optional<std::size_t> selected_quantity( const std::string_view text,
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
#endif // CATA_SRC_A11Y_QUANTITY_SELECTION_H
