#pragma once
#ifndef CATA_SRC_ROTATABLE_SYMBOLS_H
#define CATA_SRC_ROTATABLE_SYMBOLS_H

#include <cstdint>
#include <string>

class JsonObject;

namespace rotatable_symbols
{

void load( const JsonObject &jo, const std::string &src );
void reset();





uint32_t get( const uint32_t &symbol, int n );

}

#endif
