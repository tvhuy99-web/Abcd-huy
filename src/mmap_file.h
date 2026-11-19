#pragma once
#ifndef CATA_SRC_MMAP_FILE_H
#define CATA_SRC_MMAP_FILE_H

#include <memory>
#include <string>

#include <filesystem>



class mmap_file
{
    public:




        static std::shared_ptr<const mmap_file> map_file( const std::filesystem::path &file_path );







        static std::unique_ptr<mmap_file> map_writeable_file( const std::filesystem::path &file_path );






        static std::unique_ptr<mmap_file> map_writeable_memory( size_t initial_size );







        bool resize_file( size_t desired_size );

        ~mmap_file();




        void *base();
        void const *base() const;




        size_t len() const;





        void flush();





        void flush( size_t offset, size_t len );


        struct impl;

    private:
        mmap_file();

        static std::unique_ptr<mmap_file> map_file_generic(
            const std::filesystem::path &file_path,
            bool writeable );

        std::shared_ptr<impl> pimpl;
};

#endif
