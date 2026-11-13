#include "a11y_item_text.h"

#include <algorithm>
#include <cstddef>

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
std::string spoken_item_name( const item &it )
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
// Long original descriptions must not be precomputed while scrolling a
// list. Announce only on a validated reader intent. A single Java event
// cannot exceed the JNI speech limit; multi-part data is framed so Java can
// reassemble it BEFORE placing it in the bounded TTS queue.
void announce_full_detail_text( const std::string &full )
{
    if( full.empty() ) {
        return;
    }
    constexpr std::size_t max_chunk = 11000;
    if( full.size() <= max_chunk ) {
        android::announce( "A11Y_DETAIL:" + full, 1 );
        return;
    }
    std::size_t begin = 0;
    bool first = true;
    while( begin < full.size() ) {
        std::size_t end = std::min( begin + max_chunk, full.size() );
        if( end < full.size() ) {
            const std::size_t boundary = full.find_last_of( "\n ", end - 1 );
            if( boundary != std::string::npos && boundary > begin + max_chunk / 2 ) {
                end = boundary + 1;
            } else {
                // Never split a Vietnamese UTF-8 character across JNI
                // strings when there is no whitespace in the interval.
                while( end > begin &&
                       ( static_cast<unsigned char>( full[end] ) & 0xc0 ) == 0x80 ) {
                    --end;
                }
            }
        }
        const bool last = end == full.size();
        android::announce( ( first ? "A11Y_DETAIL_BEGIN:" :
                             last ? "A11Y_DETAIL_END:" : "A11Y_DETAIL_PART:" ) +
                           full.substr( begin, end - begin ), 1 );
        begin = end;
        first = false;
    }
}

// The actual item, not a guessed type, supplies its full facts: melee and
// ranged damage, magazine, ammunition, armor, food, books, tools, etc.
// Context is the already visible location/status, never an extra action.
void announce_full_item_info( const item &it, const std::string_view context )
{
    const std::string full = spoken_item_name( it ) +
                             ( context.empty() ? "" : "\n" + remove_color_tags( context ) ) +
                             "\n" + remove_color_tags( it.info( true ) );
    announce_full_detail_text( full );
}
} // namespace a11y
