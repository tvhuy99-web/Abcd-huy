#pragma once
#ifndef CATA_SRC_JSON_LOADER_H
#define CATA_SRC_JSON_LOADER_H

#include "path_info.h"
#include "flexbuffer_json.h"

class json_loader
{
    public:



        static JsonValue from_path( const cata_path &source_file ) noexcept( false );
        static JsonValue from_path_at_offset( const cata_path &source_file,
                                              size_t offset = 0 ) noexcept( false );


        static std::optional<JsonValue> from_path_opt( const cata_path &source_file ) noexcept( false );
        static std::optional<JsonValue> from_path_at_offset_opt( const cata_path &source_file,
                size_t offset = 0 ) noexcept( false );


        static JsonValue from_string( std::string data ) noexcept( false );
        static std::optional<JsonValue> from_string_opt( std::string const &data ) noexcept( false );

};

#endif
