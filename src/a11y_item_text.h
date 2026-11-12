#pragma once
#ifndef CATA_SRC_A11Y_ITEM_TEXT_H
#define CATA_SRC_A11Y_ITEM_TEXT_H

#include <algorithm>
#include <string>

#include "a11y_android.h"

#include "item.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"

namespace a11y
{
/**
 * Spoken item names must not contain the visual durability/burn glyph.
 * Keep all other meaningful item-name segments, and supply the underlying
 * condition explicitly from the item. This is shared by both ordinary
 * visible tile observations and the explicit unrestricted reader scan.
 */
inline std::string spoken_item_name( const item &it )
{
    std::string name = remove_color_tags( it.tname( 1, false ) );
    const int damage = it.damage_level();
    if( damage > 0 ) {
        name += ". " + string_format( _( "Damage level: %d" ), damage );
    } else if( damage < 0 ) {
        name += ". ";
        name += _( "Reinforced" );
    }
    if( it.burnt > 0 ) {
        name += ". ";
        name += _( "Burn damaged" );
    }
    if( it.is_favorite ) {
        name += ". ";
        name += _( "Favorite item." );
    }
    return name;
}
// Item information is assembled only for an explicit reader request.
// Keep list traversal fast; the item itself supplies the same complete
// description used by the native Examine window, regardless of item type.
// Send bounded speech chunks so unusually large item descriptions are not
// rejected by the native-to-Java announcement size limit.
inline void announce_full_item_info( const item &it )
{
    const std::string full = spoken_item_name( it ) + "\n" +
                             remove_color_tags( it.info( true ) );
    constexpr std::size_t max_chunk = 11000;
    std::size_t begin = 0;
    while( begin < full.size() ) {
        std::size_t end = std::min( begin + max_chunk, full.size() );
        if( end < full.size() ) {
            const std::size_t boundary = full.find_last_of( "\n ", end - 1 );
            if( boundary != std::string::npos && boundary > begin + max_chunk / 2 ) {
                end = boundary + 1;
            }
        }
        android::announce( "A11Y_DETAIL:" + full.substr( begin, end - begin ), 1 );
        begin = end;
    }
}
} // namespace a11y

#endif // CATA_SRC_A11Y_ITEM_TEXT_H
