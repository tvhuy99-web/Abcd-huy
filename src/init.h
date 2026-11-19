#pragma once
#ifndef CATA_SRC_INIT_H
#define CATA_SRC_INIT_H

#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <string> // IWYU pragma: keep
#include <utility>
#include <vector>

#include "cata_path.h"
#include "memory_fast.h"

class JsonObject;
class JsonValue;





































class DynamicDataLoader
{
    public:




        using deferred_json = std::list<std::pair<JsonObject, std::string>>;

    private:
        bool finalized = false;

        struct cached_streams;

        std::unique_ptr<cached_streams> stream_cache;

    protected:




        using type_string = std::string;
        using t_type_function_map =
            std::map<type_string, std::function<void( const JsonObject &, const std::string &, const cata_path &, const cata_path & )>>;
        using str_vec = std::vector<std::string>;
        t_type_function_map type_function_map;
        void add( const std::string &type, const std::function<void( const JsonObject & )> &f );
        void add( const std::string &type,
                  const std::function<void( const JsonObject &, const std::string & )> &f );
        void add( const std::string &type,
                  const std::function<void( const JsonObject &, const std::string &, const std::string &, const std::string & )>
                  &f );
        void add( const std::string &type,
                  const std::function<void( const JsonObject &, const std::string &, const cata_path &, const cata_path & )>
                  &f );









        void load_all_from_json( const JsonValue &jsin, const std::string &src,
                                 const cata_path &base_path, const cata_path &full_path );






        void load_object( const JsonObject &jo, const std::string &src,
                          const cata_path &base_path = cata_path{},
                          const cata_path &full_path = cata_path{} );

        DynamicDataLoader();
        ~DynamicDataLoader();



        void initialize();





        void check_consistency();

    public:



        static DynamicDataLoader &get_instance();











        void load_data_from_path( const cata_path &path, const std::string &src );










        void load_mod_data_from_path( const cata_path &path, const std::string &src );







        void load_mod_interaction_files_from_path( const cata_path &path, const std::string &src );





        void unload_data();











        void finalize_loaded_data();





        void load_deferred( deferred_json &data );




        bool is_data_finalized() const {
            return finalized;
        }







        shared_ptr_fast<std::istream> get_cached_stream( const std::string &path );
};

#endif
