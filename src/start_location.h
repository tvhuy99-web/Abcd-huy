#pragma once
#ifndef CATA_SRC_START_LOCATION_H
#define CATA_SRC_START_LOCATION_H

#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common_types.h"
#include "coords_fwd.h"
#include "translation.h"
#include "type_id.h"

class JsonObject;
class JsonValue;
class avatar;
class tinymap;
enum class ot_match_type : int;
struct city;

struct start_location_placement_constraints {
    numeric_interval<int> city_size;
    numeric_interval<int> city_distance;
    numeric_interval<int> allowed_z_levels;
};

struct omt_types_parameters {
    std::string omt;
    ot_match_type omt_type;
    std::unordered_map<std::string, std::string> parameters;

    void deserialize( const JsonValue &jv );
};

class start_location
{
    public:
        start_location_id id;
        std::vector<std::pair<start_location_id, mod_id>> src;
        bool was_loaded = false;
        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;

        std::string name() const;
        int targets_count() const;
        omt_types_parameters random_target() const;
        const std::set<std::string> &flags() const;







        std::pair<tripoint_abs_omt, std::unordered_map<std::string, std::string>>
                find_player_initial_location( const point_abs_om &origin ) const;






        std::pair<tripoint_abs_omt, std::unordered_map<std::string, std::string>>
                find_player_initial_location( const city &origin ) const;



        void set_parameters( const tripoint_abs_omt &omtstart,
                             const std::unordered_map<std::string, std::string> &parameters_to_set ) const;




        void prepare_map( const tripoint_abs_omt &omtstart ) const;



        void place_player( avatar &you, const tripoint_abs_omt &omtstart ) const;







        void burn( const tripoint_abs_omt &omtstart, size_t count, int rad ) const;



        void add_map_extra( const tripoint_abs_omt &omtstart, const map_extra_id &map_extra ) const;

        void handle_heli_crash( avatar &you ) const;





        void surround_with_monsters( const tripoint_abs_omt &omtstart, const mongroup_id &type,
                                     float expected_points ) const;
        const start_location_placement_constraints &get_constraints() const {
            return constraints_;
        }

        bool requires_city() const;

        bool can_belong_to_city( const tripoint_om_omt &p, const city &cit ) const;
    private:
        translation _name;
        std::vector<omt_types_parameters> _locations;
        std::set<std::string> _flags;
        start_location_placement_constraints constraints_;

        void prepare_map( tinymap &m ) const;
};

namespace start_locations
{

void load( const JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<start_location> &get_all();

}

#endif
