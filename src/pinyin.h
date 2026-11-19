#pragma once
#ifndef CATA_SRC_PINYIN_H
#define CATA_SRC_PINYIN_H

#include <string_view>

namespace pinyin
{









bool pinyin_match( std::u32string_view str, std::u32string_view qry );

}
#endif
