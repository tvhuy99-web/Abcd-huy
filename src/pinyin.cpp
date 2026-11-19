#include "pinyin.h"

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "third-party/pinyin/pinyin_data.hpp"

namespace pinyin
{
bool pinyin_match( const std::u32string_view str, const std::u32string_view qry )
{


    static std::unordered_map<char32_t, std::vector<std::u32string>> indexed_pinyin_map;


    if( indexed_pinyin_map.empty() ) {
        for( auto const &entry : pinyin_data ) {

            for( const char32_t current_char : entry.second ) {

                if( indexed_pinyin_map.find( current_char ) == indexed_pinyin_map.end() ) {
                    std::vector<std::u32string> pinyin_of_this_char;
                    pinyin_of_this_char.push_back( entry.first );
                    indexed_pinyin_map.insert( std::make_pair( current_char, pinyin_of_this_char ) );
                } else {

                    if( std::find( indexed_pinyin_map.at( current_char ).begin(),
                                   indexed_pinyin_map.at( current_char ).end(),
                                   entry.first ) == indexed_pinyin_map.at( current_char ).end() ) {
                        indexed_pinyin_map.at( current_char ).push_back( entry.first );
                    }
                }
            }
        }
    }

    int combination_index = 0;
    bool all_combinations_tested = false;
    while( !all_combinations_tested ) {
        std::u32string current_combination;

        current_combination.reserve( str.length() * 6 );

        int combination = combination_index;
        int total_combination = 1;

        for( const char32_t ch : str ) {

            if( indexed_pinyin_map.find( ch ) == indexed_pinyin_map.end() ) {

                current_combination += ch;
                continue;
            }
            const std::vector<std::u32string> &cur_char_pinyin_list = indexed_pinyin_map.at( ch );







            current_combination += cur_char_pinyin_list.at( combination % cur_char_pinyin_list.size() );
            combination /= cur_char_pinyin_list.size();


            total_combination *= cur_char_pinyin_list.size();
        }

        if( current_combination.find( qry ) != std::u32string::npos ) {
            return true;
        }

        combination_index++;
        all_combinations_tested = ( combination_index >= total_combination );
    }

    return false;
}
}
