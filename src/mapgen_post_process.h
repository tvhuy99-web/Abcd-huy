#pragma once
#ifndef CATA_SRC_MAPGEN_POST_PROCESS_H
#define CATA_SRC_MAPGEN_POST_PROCESS_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class JsonValue;
class map;
template<typename E> struct enum_traits;
template<typename T> class generic_factory;


enum class sub_generator_type : int {
    bash_damage = 0,
    move_items,
    add_fire,
    pre_burn,
    place_blood,
    aftershock_ruin,
    modify_items,
    ter_furn_transform,
    last
};

template<>
struct enum_traits<sub_generator_type> {
    static constexpr sub_generator_type last = sub_generator_type::last;
};






enum class pp_sub_generator_scope : int {
    omt = 0,
    overmap_special,
    last
};

template<>
struct enum_traits<pp_sub_generator_scope> {
    static constexpr pp_sub_generator_scope last = pp_sub_generator_scope::last;
};

struct generator_item_filter {
    std::optional<item_category_id> category;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &obj ) {
        load( obj );
    }
};


struct generator_item_modification {

    int delete_chance = 100;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &obj ) {
        load( obj );
    }
};



struct pp_sub_generator {
    int attempts = 0;
    int chance = 0;


    int min_intensity = 0;
    int max_intensity = 0;
    int scaling_days_start = 0;
    int scaling_days_end = 0;
    sub_generator_type type = sub_generator_type::bash_damage;
    pp_sub_generator_scope scope = pp_sub_generator_scope::omt;


    std::optional<generator_item_filter> filter;
    generator_item_modification mod;


    std::optional<ter_furn_transform_id> ter_furn_transform_used;

    void load( const JsonObject &jo );
    void check( const std::string &ctx ) const;
};







struct pp_sub_decision {
    enum class status : uint8_t { not_evaluated = 0, applied, skipped };

    sub_generator_type type = sub_generator_type::last;
    uint8_t ordinal = 0;
    status st = status::not_evaluated;
    uint32_t seed = 0;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &jv );
};


struct pp_resolved_generator {
    pp_generator_id generator;
    std::vector<pp_sub_decision> sub_decisions;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &jv );
};



class pp_generator
{
    public:
        pp_generator_id id = pp_generator_id::NULL_ID();

        const std::vector<pp_sub_generator> &sub_generators() const {
            return sub_generators_;
        }





        void execute( map &md, const tripoint_abs_omt &p,
                      std::vector<pp_sub_decision> *decisions = nullptr ) const;

    private:
        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        bool was_loaded = false;
        friend class generic_factory<pp_generator>;

        std::vector<pp_sub_generator> sub_generators_;
};

namespace pp_generators
{
void load( const JsonObject &jo, const std::string &src );
void reset();
void finalize();
void check_consistency();
}

#endif
