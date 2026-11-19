#pragma once
#ifndef CATA_SRC_VITAMIN_H
#define CATA_SRC_VITAMIN_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "stomach.h"
#include "translation.h"
#include "type_id.h"

class JsonObject;
template <typename T> struct enum_traits;

enum class vitamin_type : int {
    VITAMIN,
    TOXIN,
    DRUG,
    COUNTER,
    num_vitamin_types
};

template<>
struct enum_traits<vitamin_type> {
    static constexpr vitamin_type last = vitamin_type::num_vitamin_types;
};

class vitamin
{
    public:
        vitamin() : id_( vitamin_id( "null" ) ), rate_( 1_hours ) {}

        const vitamin_id &id() const {
            return id_;
        }

        const vitamin_type &type() const {
            return type_;
        }

        bool is_null() const {
            return id_ == vitamin_id( "null" );
        }

        std::string name() const {
            return name_.translated();
        }

        bool has_flag( const std::string &flag ) const {
            return flags_.count( flag ) > 0;
        }


        const efftype_id &deficiency() const {
            return deficiency_;
        }


        const efftype_id &excess() const {
            return excess_;
        }


        int min() const {
            return min_;
        }


        int max() const {
            return max_;
        }





        time_duration rate() const {
            return rate_;
        }


        std::vector<std::pair<vitamin_id, int>> decays_into() const {
            return decays_into_;
        }


        int severity( int qty ) const;


        static void load_vitamin( const JsonObject &jo );


        static const std::map<vitamin_id, vitamin> &all();


        static void check_consistency();


        static void reset();





        float RDA_to_default( int percent ) const;


        int units_absorption_per_day() const;

        int units_from_mass( vitamin_units::mass val ) const;

        std::pair<std::string, std::string> mass_str_from_units( int units ) const;

    private:
        vitamin_id id_;
        vitamin_type type_ = vitamin_type::num_vitamin_types;
        translation name_;
        std::optional<vitamin_units::mass> weight_per_unit;
        efftype_id deficiency_;
        efftype_id excess_;
        int min_ = 0;
        int max_ = 0;
        time_duration rate_ = 0_turns;
        std::vector<std::pair<int, int>> disease_;
        std::vector<std::pair<int, int>> disease_excess_;
        std::vector<std::pair<vitamin_id, int>> decays_into_;
        std::set<std::string> flags_;
};

#endif
