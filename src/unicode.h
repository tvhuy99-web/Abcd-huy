#pragma once
#ifndef CATA_SRC_UNICODE_H
#define CATA_SRC_UNICODE_H

#include <cstdint>






bool is_cjk_or_emoji( uint32_t ch );










char32_t u32_to_lowercase( char32_t ch );




char32_t u32_to_uppercase( char32_t ch );



void remove_accent( char32_t &ch );

bool u32_isspace( char32_t ch );

bool u32_ispunct( char32_t ch );

#endif
