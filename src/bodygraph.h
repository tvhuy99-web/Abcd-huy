#pragma once
#ifndef CATA_SRC_BODYGRAPH_H
#define CATA_SRC_BODYGRAPH_H

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.h"
#include "damage.h"
#include "effect.h"
#include "type_id.h"

class Character;
class JsonObject;

struct bodygraph_part {

    std::vector<bodypart_id> bodyparts;

    std::vector<sub_bodypart_id> sub_bodyparts;

    bodygraph_id nested_graph;

    nc_color sel_color;

    std::string sym;
};


struct bodygraph_info {
    std::vector<std::string> worn_names;
    resistances worst_case;
    resistances median_case;
    resistances best_case;
    std::string parent_bp_name;
    std::vector<effect> effects;
    std::pair<int, nc_color> temperature;
    std::string temp_approx;
    int avg_coverage = 0;
    int avg_env_protection = 0;
    int total_encumbrance = 0;
    int part_hp_cur = 0;
    int part_hp_max = 0;
    float wetness = 0;
    bool specific_sublimb = false;
};

struct bodygraph {
    bodygraph_id id;
    std::optional<bodypart_id> parent_bp;
    std::optional<bodygraph_id> mirror;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::vector<std::string>> fill_rows;
    std::map<std::string, bodygraph_part> parts;
    std::string label_fill;
    std::string fill_sym;
    nc_color fill_color = c_white;
    bool was_loaded = false;

    static void load_bodygraphs( const JsonObject &jo, const std::string &src );
    static void finalize_all();
    static void check_all();
    static void reset();
    static const std::vector<bodygraph> &get_all();
    void load( const JsonObject &jo, std::string_view src );
    void finalize();
    void check() const;
};


void display_bodygraph( const Character &u, const bodygraph_id &id = bodygraph_id::NULL_ID() );

using bodygraph_callback =
    std::function<std::string( const bodygraph_part *, std::string )>;










std::vector<std::string> get_bodygraph_lines( const Character &u,
        const bodygraph_callback &fragment_cb, const bodygraph_id &id = bodygraph_id::NULL_ID(),
        int width = 0, int height = 0, std::string_view label = "" );

#endif
