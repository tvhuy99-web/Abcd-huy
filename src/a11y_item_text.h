#pragma once
#ifndef CATA_SRC_A11Y_ITEM_TEXT_H
#define CATA_SRC_A11Y_ITEM_TEXT_H

#include <string>

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
        name += ". " + _( "Reinforced" );
    }
    if( it.burnt > 0 ) {
        name += ". " + _( "Burn damaged" );
    }
    return name;
}
} // namespace a11y

#endif // CATA_SRC_A11Y_ITEM_TEXT_H
