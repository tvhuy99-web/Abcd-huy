#pragma once
#ifndef CATA_SRC_OFSTREAM_WRAPPER_H
#define CATA_SRC_OFSTREAM_WRAPPER_H

#include "filesystem.h"
















class ofstream_wrapper
{
    private:
        std::ofstream file_stream;
        std::filesystem::path path;
        std::filesystem::path temp_path;

        void open( std::ios::openmode mode );

    public:
        ofstream_wrapper( const std::filesystem::path &path, std::ios::openmode mode );
        ~ofstream_wrapper();

        std::ostream &stream() {
            return file_stream;
        }
        explicit operator std::ostream &() {
            return file_stream;
        }

        void close();
};

#endif
