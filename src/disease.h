#pragma once
#ifndef CATA_SRC_DISEASE_H
#define CATA_SRC_DISEASE_H

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "type_id.h"

class JsonObject;

class disease_type
{
    public:
        static void load_disease_type( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void reset();
        void load( const JsonObject &jo, std::string_view );
        static const std::vector<disease_type> &get_all();
        static void check_disease_consistency();
        bool was_loaded = false;

        diseasetype_id id;
        std::vector<std::pair<diseasetype_id, mod_id>> src;
        time_duration min_duration = 1_turns;
        time_duration max_duration = 1_turns;
        int min_intensity = 1;
        int max_intensity = 1;

        std::set<bodypart_str_id> affected_bodyparts;

        std::optional<int> health_threshold;

        efftype_id symptoms;

};
#endif
