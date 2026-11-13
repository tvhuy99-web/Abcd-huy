#pragma once
#ifndef CATA_SRC_A11Y_ITEM_TEXT_H
#define CATA_SRC_A11Y_ITEM_TEXT_H

#include <string>
#include <string_view>

class item;

namespace a11y
{
std::string spoken_item_name( const item &it );
void announce_full_detail_text( const std::string &full );
void announce_full_item_info( const item &it, std::string_view context = {} );
} // namespace a11y

#endif // CATA_SRC_A11Y_ITEM_TEXT_H
