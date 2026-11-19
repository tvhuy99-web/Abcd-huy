#pragma once
#ifndef CATA_SRC_SUBBODYPART_H
#define CATA_SRC_SUBBODYPART_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "damage.h"
#include "translation.h"
#include "type_id.h"

class JsonObject;
struct body_part_type;
struct sub_body_part_type;
template <typename T> struct enum_traits;

using sub_bodypart_id = int_id<sub_body_part_type>;

enum class side : int {
    BOTH,
    LEFT,
    RIGHT,
    num_sides
};

template<>
struct enum_traits<side> {
    static constexpr side last = side::num_sides;
};

struct sub_body_part_type {

    sub_bodypart_str_id id;
    std::vector<std::pair<sub_bodypart_str_id, mod_id>> src;
    sub_bodypart_str_id opposite;

    bool was_loaded = false;


    translation name;

    translation name_multiple;

    side part_side = side::BOTH;


    string_id<body_part_type> parent;





    bool secondary = false;




    int max_coverage = 0;




    std::vector<sub_bodypart_str_id> locations_under;



    std::optional<sub_bodypart_str_id> similar_bodypart;

    damage_instance unarmed_damage;

    static void load_bp( const JsonObject &jo, const std::string &src );

    void load( const JsonObject &jo, std::string_view src );


    static std::vector<translation> consolidate( std::vector<sub_bodypart_id> &covered );

    std::vector<sub_bodypart_str_id> get_all_combined_similar_sub_bodyparts() const;


    static void reset();

    static void finalize_all();

    void finalize();
};

#endif
